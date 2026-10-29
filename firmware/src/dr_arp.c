/* SPDX-License-Identifier: GPL-3.0-only
 * Double Rainbow's arp: a generative arpeggiator for track 1, the character (dr.c turns it on, draws it and turns
 * its knobs; seq.c runs it on the sequencer's clock, so it lands on the beat and follows the tempo, tapped or not).
 *
 * Where the notes come from (SOURCE): KEYS, the keys held; HOLD, the last keys held, kept after they are let go;
 * AUTO, the key's own scale with nothing held (it plays by itself), the keys held when there are some.
 * How it moves through them (SHAPE): up, down, up and back, a random walk (mostly a step, sometimes two), or a
 * jump anywhere. RATE: eighths, eighth triplets, sixteenths, sixteenth triplets or thirty-seconds. RANGE: 1 to 4
 * octaves. GATE: how long each note is held.
 * The rhythm: HITS spread as evenly as they go over STEPS (a Euclidean rhythm: 3 in 8 is the tresillo), CHANCE that
 * a hit plays at all, BURST (DRA_RATCHET: a ratchet) the chance it plays as a fast double or triple.
 * Change: SPICE lets passing notes in (AUTO: the scale's other notes among the chord's; KEYS: a step off the note
 * held, in the key), LOOP repeats the last 4 to 16 notes chosen, and EVOLVE is the chance that one of them is
 * chosen again each time round (0: the loop is locked).
 * Feel: SWING pushes every other step late (all the way: a shuffle), HUMAN nudges each note a little late and a little
 * louder or softer at random, as hands do, ACCENT leans on the bar's first step and softens the rest, and LEAP is the
 * chance a note jumps an octave up or down.
 * Runs in the audio ISR (seq_advance); dr.c writes p[] (bytes) from the main loop. */
enum { DRA_SHAPE, DRA_RATE, DRA_RANGE, DRA_GATE, DRA_STEPS, DRA_HITS, DRA_CHANCE, DRA_RATCHET,
       DRA_SOURCE, DRA_SPICE, DRA_LOOP, DRA_EVOLVE, DRA_SWING, DRA_HUMAN, DRA_ACCENT, DRA_LEAP, DRA_N };
enum { DRA_UP, DRA_DOWN, DRA_BOUNCE, DRA_WALK, DRA_JUMP };
enum { DRA_KEYS, DRA_HOLD, DRA_AUTO };
static const uint8_t DRA_SPB[5] = {2, 3, 4, 6, 8};      /* RATE: steps a beat */
static struct {
    volatile uint8_t on;
    uint8_t p[DRA_N];
    uint8_t held[16], nheld, phys;           /* the notes held, low to high; the keys down (HOLD keeps the notes) */
    uint8_t note, rat, rnote, cyc;           /* sounding (0 none); ratchet notes to come, and theirs; the step */
    int8_t dir;                              /* BOUNCE: the way it is going */
    int16_t i;                               /* the place in the pool */
    uint8_t lp[16], lpos;                    /* LOOP: the places chosen, and where it is */
    uint16_t lfull;                          /* .. which of them are chosen yet */
    volatile uint8_t step;                   /* the step just played (dr.c lights it), + 1; 0 none yet */
    uint8_t pend, pvel;                      /* a note waiting to start (0 none: SWING, HUMAN), and how loud */
    uint32_t last, off, rat_len, rat_at, gate, pend_at;
    volatile uint32_t count;                 /* notes played (the tests count them) */
} dra;

static void dra_add(uint32_t note)
{
    uint32_t i, j;
    if (dra.p[DRA_SOURCE] == DRA_HOLD && !dra.phys)
        dra.nheld = 0;                                   /* (HOLD: a new chord replaces the kept one) */
    dra.phys++;
    for (i = 0; i < dra.nheld && dra.held[i] < note; i++)
        ;
    if ((i < dra.nheld && dra.held[i] == note) || dra.nheld >= 16u)
        return;
    for (j = dra.nheld; j > i; j--)
        dra.held[j] = dra.held[j - 1u];
    dra.held[i] = (uint8_t)note;
    dra.nheld++;
}

static void dra_remove(uint32_t note)
{
    uint32_t i, k = 0;
    if (dra.phys)
        dra.phys--;
    if (dra.p[DRA_SOURCE] == DRA_HOLD)
        return;
    for (i = 0; i < dra.nheld; i++)
        if (dra.held[i] != note)
            dra.held[k++] = dra.held[i];
    dra.nheld = (uint8_t)k;
}

/* the pool, low to high: the held notes over RANGE octaves, or (AUTO, nothing held) the key's major scale from the
 * tonic under middle C; chord[] marks the current chord's notes (WEAVE and CAST: the one picked; SOLO: the key's I) */
static uint32_t dra_pool(uint8_t *pool, uint8_t *chord)
{
    static const uint8_t MAJ[7] = {0, 2, 4, 5, 7, 9, 11};
    uint32_t n = 0, o, k, root, ct, oct = dra.p[DRA_RANGE];
    int8_t iv[4];
    uint32_t m = dr_chord_tones(dr_kmode == DRK_SOLO ? 3u : dr_chord, &root, iv);
    for (ct = 0, k = 0; k < m; k++)
        ct |= 1u << ((root + (uint32_t)iv[k]) % 12u);
    if (dra.nheld) {
        for (o = 0; o < oct; o++)
            for (k = 0; k < dra.nheld && n < 48u; k++, n++) {
                pool[n] = (uint8_t)clamp(dra.held[k] + 12 * (int32_t)o, 0, 127);
                chord[n] = 1;
            }
    } else if (dra.p[DRA_SOURCE] == DRA_AUTO)
        for (o = 0; o < oct; o++)
            for (k = 0; k < 7u && n < 48u; k++, n++) {
                uint32_t pc = (MAJ[k] + dr_key) % 12u;
                pool[n] = (uint8_t)clamp(48 + dr_key % 12u + MAJ[k] + 12 * (int32_t)o + 12 * song.octave, 0, 127);
                chord[n] = (uint8_t)((ct >> pc) & 1u);
            }
    return n;
}

static void dra_note_off(void)
{
    if (dra.note)
        trk_note_off(&trk[0], dra.note);
    dra.note = 0;
}

static void dra_note_on(uint32_t note, uint32_t vel, uint32_t gate)
{
    dra_note_off();
    dra.note = (uint8_t)note;
    dra.off = gate > 64u ? gate : 64u;
    dra.count++;
    trk_note_on(&trk[0], note, vel);
}

/* the next place in the pool of n, by SHAPE */
static int32_t dra_next(int32_t i, int32_t n)
{
    uint32_t r;
    switch (dra.p[DRA_SHAPE]) {
    case DRA_DOWN:
        return (i + n - 1) % n;
    case DRA_BOUNCE:
        if (n < 2)
            return 0;
        if (dra.dir >= 0 ? i >= n - 1 : i <= 0)
            dra.dir = (int8_t)(dra.dir >= 0 ? -1 : 1);
        return clamp(i + (dra.dir >= 0 ? 1 : -1), 0, n - 1);
    case DRA_WALK:                                       /* a step mostly, now and then two; off the end, back in */
        r = rng() % 8u;
        i += r == 0u ? -2 : r < 4u ? -1 : r < 7u ? 1 : 2;
        if (i < 0)
            i = -i;
        if (i >= n)
            i = 2 * (n - 1) - i;
        return clamp(i, 0, n - 1);
    case DRA_JUMP:
        return (int32_t)(rng() % (uint32_t)n);
    default:
        return (i + 1) % n;
    }
}

/* one step of the grid: a hit (or a rest), its note chosen */
static void dra_fire(uint32_t len, uint32_t first)
{
    uint8_t pool[48], chord[48];
    uint32_t steps = dra.p[DRA_STEPS], hits = dra.p[DRA_HITS], cyc = dra.cyc, n, loop = 4u * dra.p[DRA_LOOP], vel, d;
    int32_t i;
    dra.cyc = (uint8_t)((cyc + 1u) % steps);
    dra.step = (uint8_t)(cyc + 1u);
    if ((cyc * (hits < steps ? hits : steps)) % steps >= hits || rng() % 100u >= dra.p[DRA_CHANCE])
        return;
    if (!(n = dra_pool(pool, chord)))
        return;
    if (loop && ((dra.lfull >> dra.lpos) & 1u) && rng() % 100u >= dra.p[DRA_EVOLVE])
        i = dra.lp[dra.lpos] % n;                        /* LOOP: the note chosen last time round */
    else {
        i = dra_next(dra.i % (int32_t)n, (int32_t)n);
        if (!chord[i] && rng() % 100u >= dra.p[DRA_SPICE])   /* (a passing note: SPICE lets it stay) */
            i = i + 1 < (int32_t)n && chord[i + 1] ? i + 1 : i > 0 ? i - 1 : i;
        dra.lp[dra.lpos] = (uint8_t)i;
        dra.lfull |= (uint16_t)(1u << dra.lpos);
    }
    dra.i = (int16_t)i;
    if (loop)
        dra.lpos = (uint8_t)((dra.lpos + 1u) % loop);
    vel = first ? 96u + dra.p[DRA_ACCENT] * 3u / 10u : 96u - dra.p[DRA_ACCENT] / 5u;   /* ACCENT: the one leans */
    if (dra.p[DRA_HUMAN]) {                              /* HUMAN: louder or softer, a little */
        int32_t w = (int32_t)dra.p[DRA_HUMAN] * 3 / 10;
        vel = (uint32_t)clamp((int32_t)vel + (int32_t)(rng() % (uint32_t)(2 * w + 1)) - w, 20, 127);
    }
    n = pool[i];
    if (dra.nheld && dra.p[DRA_SPICE] && rng() % 200u < dra.p[DRA_SPICE])   /* KEYS: a step off, in the key */
        n = (uint32_t)dr_in_key((int32_t)n + (rng() & 1u ? 2 : 1));
    if (rng() % 100u < dra.p[DRA_LEAP])                  /* LEAP: an octave up or down */
        n = n + 12u <= 108u && (rng() & 1u || n < 36u) ? n + 12u : n - 12u;
    dra.gate = len * dra.p[DRA_GATE] / 100u;
    dra.rat = 0;
    if (rng() % 100u < dra.p[DRA_RATCHET]) {             /* RATCHET: two or three, inside the step */
        dra.rat = (uint8_t)(1u + (rng() & 1u));
        dra.rat_len = len / (dra.rat + 1u);
        dra.rat_at = dra.rat_len;
        dra.gate = dra.rat_len * 6u / 10u;
    }
    dra.rnote = (uint8_t)n;
    d = (cyc & 1u) ? len * dra.p[DRA_SWING] / 300u : 0u;   /* SWING: the off steps late, a third at most */
    if (dra.p[DRA_HUMAN])                                /* HUMAN: up to a quarter step late */
        d += rng() % (len * dra.p[DRA_HUMAN] / 400u + 1u);
    dra.rat_at += d;
    if (!d)
        dra_note_on(n, vel, dra.gate);
    else {
        dra.pend = (uint8_t)n;
        dra.pvel = (uint8_t)vel;
        dra.pend_at = d;
    }
}

/* from seq_advance, every block of n samples (after the beat moved): a step when the grid crosses one */
static void dra_tick(uint32_t n)
{
    uint32_t beat = seq_beat_samples(), len, idx;
    if (dra.note) {
        if (dra.off <= n)
            dra_note_off();
        else
            dra.off -= n;
    }
    if (!dra.on)
        return;
    if (dra.pend) {                                      /* (a late note: now) */
        if (dra.pend_at <= n) {
            dra_note_on(dra.pend, dra.pvel, dra.gate);
            dra.pend = 0;
        } else
            dra.pend_at -= n;
    }
    if (dra.rat && !dra.pend) {
        if (dra.rat_at <= n) {
            dra.rat--;
            dra.rat_at = dra.rat_len;
            dra_note_on(dra.rnote, 80, dra.gate);
        } else
            dra.rat_at -= n;
    }
    len = beat / DRA_SPB[dra.p[DRA_RATE] % 5u];
    if (!len)
        return;
    idx = (beat_n * beat + beat_pos) / len;
    if (idx == dra.last)
        return;
    dra.last = idx;
    dra_fire(len, idx == 0u);
}

/* on or off (the main loop): what sounds on track 1 ends, the keys start again */
static void dra_set(uint32_t on)
{
    if (on == dra.on)
        return;
    dra.on = 0;
    dra.nheld = dra.phys = 0;
    dra.lfull = 0;
    dra.lpos = dra.cyc = dra.rat = dra.pend = 0;
    dra.last = 0xFFFFFFFFu;
    panic_req |= 1u;                                     /* (every note on track 1 ends, in the ISR) */
    dra.on = (uint8_t)on;
}
