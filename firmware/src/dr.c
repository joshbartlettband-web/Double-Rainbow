/* SPDX-License-Identifier: GPL-3.0-only
 * Double Rainbow (joshbartlettband-web/Double-Rainbow), built on Melodee. Double Rainbow's own code lives here, kept
 * in as few files as possible so Melodee's updates merge easily (DOUBLE-RAINBOW.md).
 *
 * The startup screen: a double moonbow over dark hills, drawn from (x, y) in two canvas strips (no picture stored).
 * True to the real thing: the bows' centre is below the horizon (the point opposite the moon, which is behind you,
 * so no moon in the sky); the primary bow is red outside, violet inside; the secondary, wider and fainter, has its
 * colours the other way round; the sky is a little brighter inside the primary and darker between the two
 * (Alexander's band). Pixel-art finish: ordered dither between the sky's steps. */

#define DR_RGB(r, g, b) RGB(r, g, b)

/* the bows: centre (DR_CX, DR_CY), each band 2 px wide; radii outward (primary violet .. red, secondary red .. violet) */
#define DR_CX 120
#define DR_CY 212
#define DR_R1 100                 /* the primary's inner edge (violet) */
#define DR_R2 132                 /* the secondary's inner edge (red) */
#define DR_BAND 2
#define DR_HORIZON 152            /* the hills rise above this; the title sits on the flat ground below */
#define DR_GROUND DR_RGB(18, 11, 28)

static const uint8_t DR_BOW[6][3] = {      /* violet, blue, green, yellow, orange, red: inside to outside */
    {160, 112, 230}, {100, 150, 236}, {120, 206, 140}, {240, 214, 120}, {240, 160, 96}, {232, 96, 104},
};
static const uint8_t DR_BAYER[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

static uint32_t dr_hash(uint32_t x, uint32_t y)
{
    uint32_t h = x * 374761393u + y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

/* the hills' top edge at column x: two slow waves, never below DR_HORIZON - 2 (the title's ground stays flat) */
static int32_t dr_hill(int32_t x)
{
    static const int8_t SIN[32] = {0, 2, 3, 5, 6, 7, 8, 8, 8, 8, 8, 7, 6, 5, 3, 2,
                                   0, -2, -3, -5, -6, -7, -8, -8, -8, -8, -8, -7, -6, -5, -3, -2};   /* 8 sin */
    int32_t t = DR_HORIZON - 12 + SIN[(x * 32 / 150) & 31] + SIN[((x + 37) * 32 / 70) & 31] / 2;
    return t > DR_HORIZON - 2 ? DR_HORIZON - 2 : t;
}

static uint16_t dr_mix(const uint8_t *a, uint32_t r, uint32_t g, uint32_t b, uint32_t pct)   /* a over (r g b) */
{
    return DR_RGB((a[0] * pct + r * (100u - pct)) / 100u, (a[1] * pct + g * (100u - pct)) / 100u,
                  (a[2] * pct + b * (100u - pct)) / 100u);
}

static uint16_t dr_pixel(int32_t x, int32_t y)
{
    int32_t dx = x - DR_CX, dy = y - DR_CY, d2 = dx * dx + dy * dy, top = dr_hill(x);
    uint32_t r, g, b, step, t;
    if (y > top) {                                       /* the hills: a moonlit rim, then dark */
        if (y == top + 1)
            return DR_RGB(58, 38, 78);
        if (y > top + 1 && y < DR_HORIZON + 4 && dr_hash((uint32_t)x, (uint32_t)y) % 97u == 0u)
            return DR_RGB(255, 214, 110);                /* a firefly */
        return DR_GROUND;
    }
    /* the sky: 12 steps from deep indigo (top) to plum (horizon), dithered between them */
    t = (uint32_t)(y * 12 * 16 / DR_HORIZON);
    step = t / 16u + ((t & 15u) > DR_BAYER[y & 3][x & 3] ? 1u : 0u);
    r = 14u + step * 3u;
    g = 10u + step * 14u / 10u;
    b = 32u + step * 36u / 10u;
    if (d2 < DR_R1 * DR_R1) {                            /* brighter inside the primary bow */
        r += 6u; g += 5u; b += 8u;
    } else if (d2 >= (DR_R1 + 6 * DR_BAND) * (DR_R1 + 6 * DR_BAND) && d2 < DR_R2 * DR_R2) {
        r -= r > 20u ? 5u : 0u; g -= g > 14u ? 4u : 0u; b -= 6u;    /* Alexander's dark band */
    }
    {
        int32_t k;
        for (k = 0; k < 6; k++) {                        /* the primary: violet (inner) .. red (outer) */
            int32_t a = DR_R1 + k * DR_BAND, z = a + DR_BAND;
            if (d2 >= a * a && d2 < z * z)
                return dr_mix(DR_BOW[k], r, g, b, 78u);
        }
        for (k = 0; k < 6; k++) {                        /* the secondary: red (inner) .. violet (outer), fainter */
            int32_t a = DR_R2 + k * DR_BAND, z = a + DR_BAND;
            if (d2 >= a * a && d2 < z * z)
                return dr_mix(DR_BOW[5 - k], r, g, b, 42u);
        }
    }
    {                                                    /* stars, sparser near the horizon and on the bows' glow */
        uint32_t h = dr_hash((uint32_t)x, (uint32_t)y);
        if (h % (240u + (uint32_t)y * 4u) == 0u)
            return (h >> 9) & 1u ? DR_RGB(243, 233, 198) : DR_RGB(150, 136, 180);
    }
    return DR_RGB(r, g, b);
}

/* the startup screen (main.c, in place of Melodee's), drawn once in two strips */
static void dr_splash(void)
{
    uint32_t half, x, y;
    for (half = 0; half < 2u; half++) {
        uint32_t y0 = half * 120u;
        cv_begin(240, 120, DR_GROUND);
        for (y = 0; y < 120u; y++)
            for (x = 0; x < 240u; x++)
                cv_px[y * 240u + x] = swap16(dr_pixel((int32_t)x, (int32_t)(y0 + y)));
        if (half) {                                      /* the title on the flat ground (rows 152..239) */
            cv_text_c(120, 156 - 120, &AF_L, "DOUBLE", DR_RGB(255, 205, 96), DR_GROUND);
            cv_text_c(120, 186 - 120, &AF_L, "RAINBOW", DR_RGB(255, 205, 96), DR_GROUND);
            cv_text_c(120, 221 - 120, &AF_S, "BUILT ON MELODEE", DR_RGB(156, 140, 180), DR_GROUND);
        }
        cv_blit(0, y0);
    }
    lcd_sync();
}

/* ------------------------------------------------------------------ the cast --- */
/* A character is an engine and a sound, four named knobs (macros) that each move up to three real settings,
 * a name, the engine's tag and a line of personality. A knob runs 0..100; each of its routes maps that linearly
 * from lo to hi onto a Prophet program field (DR_P5) or a track parameter (DR_TP, t->p[]). "under" is what EDIT
 * held shows ("what the knobs move"): one or two lines, split at '|', each at most 51 px of AF_S (dr_text_check). */
#define ENGI_NOISE 11u                       /* engines.c ENGINES[] */
#define ENGI_PHASE 2u
#define ENGI_LOFI 3u
#define ENGI_VOICE 5u
#define ENGI_ANALOG 0u
enum { DR_NONE, DR_P5, DR_TP, DR_PRESET };   /* DR_PRESET: the knob picks the engine's preset (lo .. hi), a step a detent */
typedef struct { uint8_t kind, id; int16_t lo, hi; } dr_route_t;
typedef struct { const char *name, *under; uint8_t def; dr_route_t r[3]; } dr_macro_t;
typedef struct {
    const char *name, *tag, *line;
    uint8_t engine, preset;                  /* the engine, and the preset the sound starts from */
    int8_t e[8];                             /* P_E0..P_E7 (DR_E_KEEP: the preset's) */
    uint8_t env[4];                          /* ATK DEC SUS REL (0, 0, 0, 0: the preset's) */
    uint8_t level, chor, rev;                /* P_LEVEL (half dB steps), the chorus and reverb sends */
    dr_macro_t m[4];
    uint8_t glow[3];                         /* the horizon's colour while it sounds (0, 0, 0: the moonbow's bands) */
} dr_char_t;
#define DR_E_KEEP {-128, -128, -128, -128, -128, -128, -128, -128}
#define DR_NCHAR 12u

/* VESPER: our own Prophet program, a warm pad that opens slowly. Saw A and saw + pulse B a little apart, the SSI
 * filter (the older one, saturating sooner: the warmth), half key tracking, velocity on the filter and the
 * amplifier, a slow triangle LFO on both oscillators' pitch (its amount: DRIFT). The knobs set the rest. */
static void dr_vesper_program(p5_patch_t *p)
{
    static const uint8_t F[][2] = {
        {P5_FREQ_A, 24}, {P5_FREQ_B, 24}, {P5_FINE_B, 18}, {P5_SAW_A, 1}, {P5_SAW_B, 1}, {P5_PULSE_B, 1},
        {P5_PW_A, 64}, {P5_PW_B, 76}, {P5_KEY_B, 1}, {P5_LEVEL_A, 120}, {P5_LEVEL_B, 104}, {P5_NOISE, 6},
        {P5_CUTOFF, 70}, {P5_RESONANCE, 20}, {P5_KEY_FILTER, 1}, {P5_FILTER_REV, 0},
        {P5_LFO_RATE, 66}, {P5_LFO_TRI, 1}, {P5_WHEEL_FREQ_A, 1}, {P5_WHEEL_FREQ_B, 1},
        {P5_VEL_FILTER, 1}, {P5_VEL_AMP, 1}, {P5_ENV_FILTER, 40},
        {P5_ATTACK_FILTER, 40}, {P5_DECAY_FILTER, 74}, {P5_SUSTAIN_FILTER, 62}, {P5_RELEASE_FILTER, 84},
        {P5_ATTACK_AMP, 36}, {P5_DECAY_AMP, 100}, {P5_SUSTAIN_AMP, 118}, {P5_RELEASE_AMP, 88},
        {P5_RELEASE_ON, 1}, {P5_UNISON_COUNT, 5}, {P5_BEND, 2},
    };
    uint32_t i;
    p5_patch_init(p);
    p->raw[P5_LEVEL_A] = 0;                  /* (p5_patch_init's defaults, then ours) */
    p->raw[P5_CUTOFF] = 0;
    for (i = 0; i < NELEM(F); i++)
        p->raw[F[i][0]] = F[i][1];
    memset(p->raw + P5_NAME, ' ', P5_NAME_LEN);
    memcpy(p->raw + P5_NAME, "VESPER", 6);
}

static const dr_char_t DR_CAST[DR_NCHAR] = {
    {"VESPER", "PROPHET", "Every note is her final scene.", ENGI_PROPHET, 0, DR_E_KEEP, {0, 0, 0, 0}, 101, 22, 40, {
        {"GLOW", "CUTOFF", 55, {{DR_P5, P5_CUTOFF, 30, 108}}},
        {"BITE", "RESO|ENV AMT", 30, {{DR_P5, P5_RESONANCE, 4, 78}, {DR_P5, P5_ENV_FILTER, 12, 92}}},
        {"BLOOM", "ATTACK|RELEASE", 35, {{DR_P5, P5_ATTACK_AMP, 4, 96}, {DR_P5, P5_ATTACK_FILTER, 10, 100},
                                           {DR_P5, P5_RELEASE_AMP, 55, 118}}},
        {"DRIFT", "VINTAGE|VIBRATO", 30, {{DR_P5, P5_VINTAGE, 0, 120}, {DR_P5, P5_LFO_INITIAL, 0, 30},
                                            {DR_P5, P5_FINE_B, 6, 48}}},
    }, {255, 170, 80}},                                         /* her lantern eyes */
    /* SHARD: Melodee's own FM6 BELL (tools/gen_fm6_patches.py; of its eight voices the one the ratio and level move
     * furthest), through the engine's four macros: low SHINE is a soft glassy piano, high a cutting bell */
    {"SHARD", "FM6", "Sharp, clear, honest to a fault.", ENGI_FM6, 1, DR_E_KEEP, {0, 0, 0, 0}, 112, 26, 46, {
        {"SHINE", "MOD|LEVEL", 45, {{DR_TP, P_E0 + 2, -44, 18}}},
        {"METAL", "MOD|RATIO", 25, {{DR_TP, P_E0 + 3, -4, 12}}},
        {"RING", "MOD|DECAY", 60, {{DR_TP, P_E0 + 4, -40, 60}}},
        {"TWIST", "DETUNE", 15, {{DR_TP, P_E0 + 6, 0, 110}}},
    }, {0, 0, 0}},                                              /* his prism: the bands */
    /* SQUALL: NOISE's META source: a 127-step register sequence once per period of the key, an airy storm-hiss
     * that still carries the tune (measured: 76 % of its energy on the note's harmonics, against 20 % for filtered
     * white noise, which never sings), through a low-pass that follows the key. GUST swells it with the LFO (the
     * drift would wander the pitch), HOWL shortens the sequence from hiss to a buzzing howl, RUMBLE darkens it,
     * SPARK drives the track's distortion (META's last value would shift the pitch). {MODE, COLR, FREQ, RES, TRK,
     * LEN, DRFT, CLK} */
    {"SQUALL", "NOISE", "Escaped from a jar. Still mad.", ENGI_NOISE, 0, {3 /* META */, 64, 60, 40, 127, 0, 0, 0},
     {40, 90, 110, 80}, 108, 0, 60, {
        {"GUST", "LFO|TO LEVEL", 35, {{DR_TP, P_LD_AMP, 0, 100}, {DR_TP, P_LRATE, 30, 72}}},
        {"HOWL", "SEQ|LENGTH", 20, {{DR_TP, P_E0 + 5, 0, 112}}},
        {"RUMBLE", "FREQ|COLOR", 35, {{DR_TP, P_E0 + 2, 84, 30}, {DR_TP, P_E0 + 1, 40, 112}}},
        {"SPARK", "DIST", 10, {{DR_TP, P_DIST, 0, 90}}},
    }, {255, 226, 110}},                                        /* his bolt */
    /* WICK: VOICE (formants: a buzz shaped into vowels), from its CHOIR AAH, quicker to speak and a little
     * breathy. VOWEL walks AH .. OO, TALK has her move between two vowels by herself, TREMBLE is her nerves
     * (vibrato, a step of the LFO's pitch depth is about 19 cents, and tremolo). {VOWL VOWL2 TALK SHIFT | BUZZ BRTH Q RAND} */
    {"WICK", "VOICE", "Sings when nervous. So, always.", ENGI_VOICE, 0, {-128, -128, -128, 0, 52, -128, 70, 6},
     {14, 90, 112, 70}, 96, 24, 50, {
        {"VOWEL", "AH|TO OO", 20, {{DR_TP, P_E0, 0, 127}}},
        {"TALK", "MORPH|VOWEL 2", 0, {{DR_TP, P_E0 + 2, 0, 96}, {DR_TP, P_E0 + 1, 40, 120}}},
        {"BREATH", "BREATH", 20, {{DR_TP, P_E0 + 5, 4, 110}}},
        {"TREMBLE", "VIBRATO|SHAKE", 30, {{DR_TP, P_LD_PIT, 0, 2}, {DR_TP, P_LD_AMP, 0, 46}, {DR_TP, P_LRATE, 58, 84}}},
    }, {255, 214, 140}},                                        /* her flame */
    /* TOCK: the DRUM engine's 808 kit (the keys are the kit, General MIDI: the first C the kick, D the snare,
     * F# the closed hat, A# the open one). {KIT TUNE TONE DECY | SNAP ACC KICK DRV} */
    {"TOCK", "DRUM", "Keeps time. Keeps grudges.", ENGI_DRUM, 0, DR_E_KEEP, {0, 0, 0, 0}, 108, 0, 16, {
        {"TUNE", "TUNE", 50, {{DR_TP, P_E0 + 1, 30, 100}}},
        {"BOOM", "DECAY", 44, {{DR_TP, P_E0 + 3, 20, 120}}},
        {"SNAP", "SNAP|TONE", 50, {{DR_TP, P_E0 + 4, 20, 120}, {DR_TP, P_E0 + 2, 40, 100}}},
        {"GRIT", "DRIVE", 0, {{DR_TP, P_E0 + 7, 0, 100}}},
    }, {63, 224, 197}},                                         /* his clock face */
    /* MOTE: LOFI's PULSE LD, an old game console's square wave. DUTY thins the pulse, SHAPE changes the wave
     * (pulse, triangle, saw),
     * ZAP sweeps each note's pitch, CHIRP turns a held key into an arpeggio (off, octaves, major, minor).
     * {CHIP WAVE DUTY CRSH | SWP VIB ARP TONE} */
    {"MOTE", "LOFI", "Small. Loud. Probably your fault.", ENGI_LOFI, 0, DR_E_KEEP, {0, 0, 0, 0}, 100, 0, 24, {
        {"DUTY", "PULSE|WIDTH", 50, {{DR_TP, P_E0 + 2, 64, 8}}},
        {"SHAPE", "WAVE|SHAPE", 0, {{DR_TP, P_E0 + 1, 0, 2}}},
        {"ZAP", "PITCH|SWEEP", 0, {{DR_TP, P_E0 + 4, 0, 80}}},
        {"CHIRP", "ARP", 0, {{DR_TP, P_E0 + 6, 0, 3}}},
    }, {255, 120, 120}},                                        /* his grin */
    /* MURK: PHASE, Casio's phase distortion (the CZ-1 engine plays Casio's own tone data and has no settings to
     * turn), from its STRING: a sine read unevenly bends into brighter waves. BEND is how far (DCW), SWELL how
     * much each note's bend rises and falls, TIDE steps through the eight waves (SAW SQR PLS DSIN SPLS, then the
     * CZ's watery resonant three: RSAW RTRI RTRP), DEPTH the sub below.
     * {WAVE WAVE2 DCW ENV | DTN LINE SUB EG} */
    {"MURK", "PHASE", "Bends the light. Bends the rules.", ENGI_PHASE, 2, DR_E_KEEP, {60, 96, 104, 90}, 96, 40, 70, {
        {"BEND", "DCW", 40, {{DR_TP, P_E0 + 2, 16, 116}}},
        {"SWELL", "DCW|ENV", 40, {{DR_TP, P_E0 + 3, 0, 127}}},
        {"TIDE", "WAVE|SHAPE", 70, {{DR_TP, P_E0, 0, 7}}},
        {"DEPTH", "SUB", 20, {{DR_TP, P_E0 + 6, 0, 110}}},
    }, {110, 230, 214}},                                        /* her glow */
    /* BOG: ANALOG's ACID, the TB-303's squelch: one saw, a resonant low-pass and a short envelope sweeping it, mono
     * with a slide. SQUISH is the cutoff, GOO the resonance, GULP how far and how long the envelope sweeps it, GRIT
     * the drive. {WAVE DTN MIX NOIS | CUT RES DRV KTR} */
    {"BOG", "ANALOG", "Squelch is a love language.", ENGI_ANALOG, 4, DR_E_KEEP, {0, 0, 0, 0}, 116, 0, 18, {
        {"SQUISH", "CUTOFF", 40, {{DR_TP, P_E0 + 4, 16, 100}}},
        {"GOO", "RESO", 60, {{DR_TP, P_E0 + 5, 40, 125}}},
        {"GULP", "ENV AMT|DECAY", 50, {{DR_TP, P_ED_FLT, 10, 63}, {DR_TP, P_DEC, 30, 95}}},
        {"GRIT", "DRIVE", 20, {{DR_TP, P_E0 + 6, 0, 110}}},
    }, {170, 240, 90}},                                         /* his throat */
    /* HUSH: ANALOG's PWM STR, a string machine: a pulse whose width sweeps, a second one detuned against it, a
     * gentle filter. DUSK darkens, FLUTTER detunes, HOLLOW mixes the two, WINGS is how slowly she arrives and
     * leaves. */
    {"HUSH", "ANALOG", "Writes everything down. Says nothing.", ENGI_ANALOG, 3, DR_E_KEEP, {0, 0, 0, 0}, 100, 50, 64, {
        {"DUSK", "CUTOFF", 56, {{DR_TP, P_E0 + 4, 30, 110}}},
        {"FLUTTER", "DETUNE", 14, {{DR_TP, P_E0 + 1, 0, 60}}},
        {"HOLLOW", "OSC MIX", 30, {{DR_TP, P_E0 + 2, 0, 127}}},
        {"WINGS", "ATTACK|RELEASE", 50, {{DR_TP, P_ATK, 4, 110}, {DR_TP, P_REL, 40, 118}}},
    }, {214, 196, 240}},                                        /* her ink */
    /* TUMBLER: FM6's DX TINE, the DX7's electric piano (the sound of a thousand 80s ballads). BARK is the tine's
     * modulator level (soft to barking), CLANG the modulator's ratio (a bell, then a gong), RING the modulator's decay, SHEEN the chorus every
     * electric piano of the time went through. {ALG FB MLVL MRAT | MEG VMOD DTUN -} */
    {"TUMBLER", "FM6", "Opens anything. Asks no one.", ENGI_FM6, 8, DR_E_KEEP, {0, 0, 0, 0}, 106, 40, 40, {
        {"BARK", "MOD|LEVEL", 50, {{DR_TP, P_E0 + 2, -40, 40}}},
        {"CLANG", "MOD|RATIO", 26, {{DR_TP, P_E0 + 3, -2, 6}}},
        {"RING", "MOD|DECAY", 50, {{DR_TP, P_E0 + 4, -40, 40}}},
        {"SHEEN", "CHORUS", 40, {{DR_TP, P_CHOR, 0, 110}}},
    }, {230, 190, 90}},                                         /* his brass */
    /* GLINT: the CZ-1, Casio's own 64 factory tones played from their own data (the engine has no settings to
     * turn: it is the real tones). TONE picks one (A-1 .. H-8, its name on the line), the rest are the track's
     * effects: SHINE the chorus, GRIT the distortion, SPACE the reverb. */
    {"GLINT", "CZ-1", "Takes the best part of everything.", ENGI_CZ, 1, DR_E_KEEP, {0, 0, 0, 0}, 94, 30, 40, {
        {"TONE", "CASIO|TONES", 1, {{DR_PRESET, 0, 1, 64}}},
        {"SHINE", "CHORUS", 28, {{DR_TP, P_CHOR, 0, 110}}},
        {"GRIT", "DIST", 0, {{DR_TP, P_DIST, 0, 80}}},
        {"SPACE", "REVERB", 36, {{DR_TP, P_REV, 0, 110}}},
    }, {120, 200, 230}},                                        /* her sheen */
    /* HERMIT: Sequential's own Iconic Brass (factory program 130), two saws through the filter, the envelope
     * opening it like a breath. The knobs start where the program has them: BLARE the cutoff, BITE the envelope
     * amount, SWELL the filter's attack, GROWL the resonance and the vintage drift. */
    {"HERMIT", "PROPHET", "Lives in a horn. Plays it, too.", ENGI_PROPHET, 130, DR_E_KEEP, {0, 0, 0, 0}, 116, 20, 36, {
        {"BLARE", "CUTOFF", 26, {{DR_P5, P5_CUTOFF, 10, 88}}},
        {"BITE", "ENV AMT", 68, {{DR_P5, P5_ENV_FILTER, 40, 127}}},
        {"SWELL", "ATTACK", 34, {{DR_P5, P5_ATTACK_FILTER, 0, 100}, {DR_P5, P5_ATTACK_AMP, 0, 50}}},
        {"GROWL", "RESO|VINTAGE", 10, {{DR_P5, P5_RESONANCE, 0, 90}, {DR_P5, P5_VINTAGE, 40, 127}}},
    }, {255, 150, 90}},                                         /* his claws */
};

static struct {
    uint8_t ch;                              /* the character on track 1 */
    uint8_t mv[DR_NCHAR][4];                 /* each character's knobs, 0..100 (kept while switching) */
    uint8_t ready;
} dr;

/* one knob of the character on track 1: its routes, from its value */
static void dr_macro_apply(track_t *t, uint32_t k, uint32_t m)
{
    const dr_macro_t *mc = &DR_CAST[k].m[m];
    int32_t v = dr.mv[k][m];
    uint32_t r;
    for (r = 0; r < 3u && mc->r[r].kind; r++) {
        const dr_route_t *rt = &mc->r[r];
        int32_t x = rt->lo + ((rt->hi - rt->lo) * v + (rt->hi > rt->lo ? 50 : -50)) / 100;   /* (rounded) */
        if (rt->kind == DR_PRESET)                       /* (dr_load picks it) */
            continue;
        if (rt->kind == DR_P5)
            p5_edit_value(t, rt->id, x);
        else {
            const param_desc_t *d = track_desc(t, rt->id);
            t->p[rt->id] = (int16_t)clamp(x, d->min, d->max);
        }
    }
}

/* the character k onto track 1 (Melodee's own load path: the engine, its preset, then ours), selected */
static void dr_load(uint32_t k)
{
    const dr_char_t *c = &DR_CAST[k % DR_NCHAR];
    track_t *t = &trk[0];
    uint32_t i, preset;
    if (!dr.ready) {
        uint32_t j, m;
        for (j = 0; j < DR_NCHAR; j++)
            for (m = 0; m < 4u; m++)
                dr.mv[j][m] = DR_CAST[j].m[m].def;
        dr.ready = 1;
    }
    dr.ch = (uint8_t)(k % DR_NCHAR);
    song.sel = 0;
    set_engine_of(t, c->engine);
    preset = c->preset;
    for (i = 0; i < 4u; i++)                             /* (a knob that picks the preset: its tone) */
        if (c->m[i].r[0].kind == DR_PRESET)
            preset = dr.mv[dr.ch][i];
    apply_preset_to(t, preset);
    fm1_irq_off();
    if (c->engine == ENGI_PROPHET && !c->preset) {       /* (Vesper: our own program; others a factory one) */
        dr_vesper_program(p5_patch_of(t));
        p5_track_accept(t);
    }
    for (i = 0; i < 8u; i++)
        if (c->e[i] != -128)
            t->p[P_E0 + i] = c->e[i];
    if (c->env[0] | c->env[1] | c->env[2] | c->env[3]) {
        t->p[P_ATK] = c->env[0];
        t->p[P_DEC] = c->env[1];
        t->p[P_SUS] = c->env[2];
        t->p[P_REL] = c->env[3];
    }
    t->p[P_LEVEL] = c->level;
    t->p[P_CHOR] = c->chor;
    t->p[P_REV] = c->rev;
    for (i = 0; i < 4u; i++)
        dr_macro_apply(t, dr.ch, i);
    fm1_irq_on();
}

/* ------------------------------------------------------------- the mode --- */
/* Double Rainbow's own screen, from power-on: the character on track 1 (keys play it as always: seq.c
 * keyboard_block reads the keys in the audio ISR), its four knobs, EDIT held for what they move, SCL for its
 * codex. HOME and SAVE held 3 s together: full Melodee, as it would have started (until the next power-on).
 *   main.c: dr_boot() after project_boot(); each pass, dr_frame() instead of Melodee's UI while it returns 1. */
#include "dr_art.h"                          /* DR_SPRITES (tools/art/gen_dr_art.py) */
#define DR_BG DR_RGB(16, 11, 34)
#define DR_SURF DR_RGB(38, 26, 62)
#define DR_TEXT DR_RGB(246, 234, 210)
#define DR_GOLD DR_RGB(255, 205, 96)
#define DR_TEAL DR_RGB(63, 224, 197)
#define DR_MID DR_RGB(176, 162, 196)
#define DR_DIM DR_RGB(120, 106, 150)
#define DR_PLUM DR_RGB(58, 38, 78)
#define DR_H_HEAD 22                         /* rows: header 0..21, line 22..37, scene 38..197, cards 198..239 */
#define DR_Y_LINE 22
#define DR_Y_SCENE 38
#define DR_Y_CARDS 198
#define DR_HOLD_MS 3000u                     /* HOME + SAVE: to Melodee */

static void project_boot(void);              /* project.c (included after this file) */

/* the codex, three pages: the Jarkeeper's ledger (the lore, in the Jarkeeper's own voice: DOUBLE-RAINBOW.md, The
 * world), the engine's story, something to try */
typedef struct { const char *entry, *when, *lore[6], *title, *sub, *body[6], *tip[4]; } dr_codex_t;
static const dr_codex_t DR_CODEX[DR_NCHAR] = {
    {"The moth", "Never caught. Not yet.",
     {"She came the night jar twelve", "broke, drawn to the glass and", "the moonlight in it. She has",
      "followed it ever since. Every", "lamp in the hills goes out as", "she passes. I keep mine lit."},
     "PROPHET-5", "Sequential, 1978",
     {"The first synth that could", "remember its sounds. Two", "oscillators feed one filter",
      "that trims the brightness", "away, and that is where", "the warmth comes from."},
     {"Hold a chord and turn GLOW", "up slowly. Then turn BLOOM", "all the way up and play", "slow notes."}},
    {"Jar twelve, the glass", "It did not stay broken.",
     {"The storm got out. The jar", "did not stay in pieces: the", "glass pulled itself together",
      "around the moonlight it had", "swallowed, and walked off.", "It will not lie to me. Pity."},
     "FM SYNTHESIS", "Chowning 1967, Yamaha DX7 1983",
     {"John Chowning found it by", "speeding up a vibrato until", "it stopped wobbling and",
      "became a new tone. One wave", "bends another, and that is", "the glassy shine."},
     {"Turn METAL slowly: each step", "is a new ratio between the", "waves. Low SHINE is a soft", "piano, high is a bell."}},
    {"Jar twelve, a storm", "Caught at the cliffs, 3 a.m.",
     {"Small, but it held a whole", "night of weather. It broke", "the jar from the inside and",
      "took the moonbow's second", "arc with it. I want it back.", "All of it."},
     "NOISE", "Every pitch at once",
     {"White noise hisses, pink is", "softer, like rain, and brown", "rumbles like thunder. Repeat",
      "a scrap of noise fast enough", "and it turns into a note:", "that is Squall's voice."},
     {"Turn HOWL up slowly: the hiss", "gets shorter and buzzier until", "it howls. Then turn GUST up",
      "and hold a long note."}},
    {"The night-light", "Never once let go out.",
     {"The only light I allow in the", "room. She was meant to watch", "the jars, not sing to them.",
      "The night twelve broke, she", "saw who did it. She will not", "say. She sings instead."},
     "VOICE", "How vowels are made",
     {"Your throat buzzes and your", "mouth shapes the buzz. Each", "vowel is a shape: a few loud",
      "bands of pitch, the formants.", "Move them and AH turns into", "OO. That is all a vowel is."},
     {"Hold a note and turn VOWEL", "slowly from AH to OO. Then", "turn TALK up: she moves", "between two vowels by herself."}},
    {"The clock", "Wound every night at nine.",
     {"He sits on my shelf and", "counts. He has counted every", "night since jar one, and he",
      "has never once lost the beat.", "He knows how long the jars", "have been here. So do I."},
     "DRUM MACHINE", "Roland TR-808, 1980",
     {"No recordings inside: every", "drum is a little circuit. The", "kick is a ringing filter left",
      "to fade, the hats are metal", "made of six square waves", "beating against each other."},
     {"From the first C up: C is the", "kick, D the snare, F# the", "closed hat, A# the open one.",
      "BOOM up: a long, deep kick."}},
    {"The dust", "On no shelf. Yet.",
     {"Not a sound I caught. He came", "with the dust between the", "shelves, and he takes the",
      "stoppers off my jars. That is", "why some of them hum at", "night. I count the stoppers."},
     "CHIPTUNE", "Game consoles, 1980s",
     {"Old game machines had a few", "plain voices: square waves,", "a triangle, a hiss. Composers",
      "made them sing with fast", "arpeggios and pitch sweeps.", "Fewer bits, more crunch."},
     {"Turn CHIRP up and hold a key:", "the note becomes a fast", "arpeggio, like an old game.", "SHAPE: pulse, triangle, saw."}},
    {"Jar one", "Let go. On purpose.",
     {"The oldest jar, and the only", "one I ever opened myself.", "I had my reasons. Some nights",
      "she drifts back and looks at", "the shelf. She knows what is", "in jar thirteen."},
     "PHASE", "Phase distortion: Casio CZ, 1984",
     {"Casio's trick: read a sine", "wave at an uneven speed and", "it bends into a saw, a square",
      "or a ringing tone. The more", "it bends, the brighter it", "sounds. No filter needed."},
     {"Turn TIDE slowly: eight waves,", "the last three ring like", "water in a glass. Then turn",
      "BEND from soft to sharp."}},
    {"Jar four, the reed jar", "Filled in the marsh below.",
     {"I dipped it in the marsh one", "summer and he was in it,", "singing four notes. He has",
      "sung the same four ever since.", "He will not stop, and I have", "stopped asking him to."},
     "ACID", "Roland TB-303, 1981",
     {"A little bass machine that", "failed, until Chicago found", "it. One saw wave, a filter",
      "that rings, and an envelope", "that sweeps it shut on every", "note: the squelch."},
     {"Play a fast low line and turn", "SQUISH up and down while it", "plays. Then turn GOO up until",
      "the filter sings by itself."}},
    {"My owl", "She keeps the ledger.",
     {"She writes these pages when", "my hands shake. She has read", "every one of them, even the",
      "ones I tore out. She has", "never said what was on them.", "I have never asked."},
     "STRINGS", "String machines: Solina, Elka, 1970s",
     {"Not real strings: a pulse", "wave whose width keeps", "moving, a second one tuned",
      "slightly off, and a chorus.", "Bands in the 70s used them", "for a whole string section."},
     {"Hold a big chord and turn", "FLUTTER up: the strings", "drift apart. WINGS up makes", "her arrive slowly."}},
    {"The key", "Not where I left it.",
     {"The key to this room. Someone", "let it out, or it let itself", "out. The door has not locked",
      "since, and the jars know it.", "That is why they keep", "getting away."},
     "FM PIANO", "Yamaha DX7, 1983",
     {"The DX7's electric piano was", "on a thousand 80s ballads.", "Two waves, one bending the",
      "other: hit a key hard and", "the bend gets brighter, the", "bark of a real tine."},
     {"Play slow chords with SHEEN", "up, like an 80s ballad. Then", "turn BARK up and hit the",
      "keys harder."}},
    {"The magpie", "Another collector.",
     {"I am not the only one in", "these hills who keeps sounds.", "She takes the best part of",
      "everything and keeps it. She", "has sixty-four now. I would", "like to see her collection."},
     "CZ-1", "Casio, 1986",
     {"Casio's own sixty-four tones,", "played from the real data. A", "factory tone is a starting",
      "point: the CZ came with", "brass, strings, bells and", "basses already made."},
     {"Turn TONE slowly: each step", "is a new Casio tone, its name", "on the line. Find a favorite,",
      "then add SPACE."}},
    {"Jar nine", "Moved in. Will not move out.",
     {"Jar nine held an old brass", "horn. Something moved into", "it, the way crabs do, and",
      "would not come out again. It", "answers when I call it.", "Loudly."},
     "PROPHET-5", "Sequential, 1978",
     {"This is one of Sequential's", "own programs, Iconic Brass.", "Two saw waves, and a filter",
      "envelope that opens like a", "breath: that swell is what", "synth brass is."},
     {"Play a chord and let go.", "Turn SWELL up: the brass", "breathes in. BITE down makes", "it softer and rounder."}},
};

static struct {
    uint8_t on, codex, page, under, dirty;   /* dirty: DR_D_* */
    uint8_t sounding;                        /* the horizon is tracing the wave (and needs one more flat redraw) */
    uint8_t drum;                            /* a drum character: the lane last struck + 1 (Melodee's live notes skip drums) */
    uint32_t wave_ms, sweep_t0;              /* sweep_t0: Shard's light began (0 = none) */
    uint32_t note_sig, combo_t0, msg_t0;
    char msg[40];
} drm;
enum { DR_D_HEAD = 1, DR_D_LINE = 2, DR_D_SCENE = 4, DR_D_CARDS = 8, DR_D_NOTE = 16, DR_D_ALL = 31 };

/* the scene's background at (x, y): the sky, a faint moonbow, the hill. The hill's edge is the horizon, and it
 * traces the live waveform (dr_wave, from Melodee's scope: audio.c scope_buf) while the character sounds: a line
 * in the character's glow over the dark ground; silent, it lies flat on the hill. */
#define DR_Y_WAVE 154                        /* the rows the horizon can reach: DR_Y_WAVE .. DR_Y_CARDS - 1 */
#define DR_WAVE_PX 10                        /* the wave's height either way */
static int8_t dr_wave[240];
static int32_t dr_scene_hill(int32_t x)
{
    int32_t d = x - 120;
    int32_t t = 168 + d * d / 900;
    return t > 186 ? 186 : t;
}
static uint16_t dr_glow(int32_t x)
{
    const uint8_t *g = DR_CAST[dr.ch].glow;
    if (!(g[0] | g[1] | g[2]))                           /* Shard: the bands, drifting along */
        g = DR_BOW[5 - (uint32_t)((x + (int32_t)(fm1_ms / 40u)) / 40) % 6u];
    return DR_RGB(g[0], g[1], g[2]);
}
static uint16_t dr_scene_px(int32_t x, int32_t y)
{
    int32_t top = dr_scene_hill(x) + dr_wave[x], dx = x - 120, dy = y - 214, d2 = dx * dx + dy * dy, k;
    uint32_t r, g, b, t, step;
    if (y > top) {
        if (y == top + 1)
            return dr_wave[x] || drm.sounding ? dr_glow(x) : DR_PLUM;
        if (y == top + 2 && (dr_wave[x] || drm.sounding)) {   /* a softer row under it: it reads across a room */
            uint16_t c = dr_glow(x);
            uint8_t rgb[3] = {(uint8_t)((c >> 11) << 3), (uint8_t)(((c >> 5) & 63u) << 2), (uint8_t)((c & 31u) << 3)};
            return dr_mix(rgb, 18, 11, 28, 45u);
        }
        return DR_GROUND;
    }
    t = (uint32_t)((y - DR_Y_SCENE) * 8 * 16 / (186 - DR_Y_SCENE));
    step = t / 16u + ((t & 15u) > DR_BAYER[y & 3][x & 3] ? 1u : 0u);
    r = 14u + step * 3u;
    g = 10u + step * 3u / 2u;
    b = 32u + step * 4u;
    for (k = 0; k < 6; k++) {                            /* the moonbow, faint: violet inside .. red outside */
        int32_t a = 104 + 2 * k, z = a + 2;
        if (d2 >= a * a && d2 < z * z)
            return dr_mix(DR_BOW[k], r, g, b, 32u);
    }
    {
        uint32_t h = dr_hash((uint32_t)x, (uint32_t)y);
        if (h % 260u == 0u)
            return (h >> 9) & 1u ? DR_RGB(243, 233, 198) : DR_RGB(130, 116, 160);
    }
    return DR_RGB(r, g, b);
}

/* the horizon's wave from Melodee's scope (audio.c scope_buf, every other sample of the output, as HOME's
 * graph_scope reads it): from a rising zero crossing, so it stands still; 1 when it changed. Below a whisper it lies
 * flat; louder, it scales with the sound (a quiet note a small ripple, a loud chord the full DR_WAVE_PX) */
static int dr_wave_update(void)
{
    static int16_t snap[SCOPE_N];
    uint32_t w = scope_w, i, trig = 0, changed = 0;
    int32_t peak = 0, x, m, gate, lo;
    for (i = 0; i < SCOPE_N; i++) {
        snap[i] = scope_buf[(w + i) & (SCOPE_N - 1u)];
        if (snap[i] > peak)
            peak = snap[i];
        else if (-snap[i] > peak)
            peak = -snap[i];
    }
    for (i = 1; i < SCOPE_N - 240u; i++)
        if (snap[i - 1] < 0 && snap[i] >= 0) {
            trig = i;
            break;
        }
    /* the scope is taken after MASTER (squared: half way is a quarter), so its thresholds move with it: the same
     * note draws the same wave at any volume */
    m = (int32_t)song.master_q12 > 16 ? (int32_t)song.master_q12 : 16;
    gate = 400 * m >> 12;
    lo = 12000 * m >> 12;
    for (x = 0; x < 240; x++) {
        int32_t v = 0;
        if (peak > (gate > 3 ? gate : 3)) {
            int32_t full = peak > lo ? peak : lo;     /* (quiet: smaller; loud: auto-scaled to the full height) */
            v = -snap[trig + (uint32_t)x] * DR_WAVE_PX / full;
        }
        if (v != dr_wave[x]) {
            dr_wave[x] = (int8_t)v;
            changed = 1;
        }
    }
    return (int)changed;
}

/* where the character stands: Squall and Murk float */
static void dr_sprite_at(uint32_t k, int32_t *x, int32_t *y)
{
    const dr_sprite_t *s = &DR_SPRITES[k];
    *x = 120 - s->w / 2;
    *y = k == 2u || k == 6u ? 70 : 174 - s->h;          /* (Squall and Murk float) */
}

/* Shard's light: each note sends a band of spectrum across the glass of his chest, the moonlight caught inside him.
 * A lens round his chest (sprite pixels), the band's six colours two pixels each, crossing it diagonally in
 * DR_SWEEP_MS, each half the glass's own colour (light inside it, not paint on it); the outline stays. */
#define DR_SWEEP_MS 560u
#define DR_SWEEP_Y0 108                      /* the screen rows it can touch: DR_SWEEP_Y0 .. + DR_SWEEP_H - 1 */
#define DR_SWEEP_H 42
static const uint16_t DR_SPECTRUM[6] = {DR_RGB(236, 72, 72), DR_RGB(250, 150, 50), DR_RGB(250, 222, 70),
                                        DR_RGB(90, 206, 96), DR_RGB(70, 140, 240), DR_RGB(156, 96, 230)};
static int32_t dr_sweep_pos(void)            /* where the band is (-1000: none) */
{
    int32_t t = (int32_t)(fm1_ms - drm.sweep_t0);       /* (signed: t0 may be fm1_ms | 1, a millisecond ahead) */
    if (!drm.sweep_t0 || dr.ch != 1u || t >= (int32_t)DR_SWEEP_MS)
        return -1000;
    if (t < 0)
        t = 0;
    return -32 + t * 76 / (int32_t)DR_SWEEP_MS;
}
static uint16_t dr_sweep_px(int32_t pos, int32_t x, int32_t y, uint16_t c)   /* Shard's pixel (x, y), its colour c */
{
    int32_t dx = x - 26, dy = y - 62, k;
    if (pos == -1000 || dx * dx * 256 / 169 + dy * dy * 256 / 256 >= 256)
        return c;
    k = (dx + dy - pos + 12) / 2;            /* (lines of dx + dy: rising to his left, as the light in his glass) */
    return k >= 0 && k < 6 ? (uint16_t)(((DR_SPECTRUM[5 - k] & 0xF7DEu) >> 1) + ((c & 0xF7DEu) >> 1)) : c;   /* (half glass) */
}

/* the scene's rows y0 .. y0 + h - 1 (h <= 80): background, then the character's runs over it */
static void dr_scene_rows(int32_t y0, int32_t h)
{
    const dr_sprite_t *s = &DR_SPRITES[dr.ch];
    int32_t sx, sy, x, y, pos = 0, sweep = dr_sweep_pos();
    uint32_t i;
    cv_begin(240, (uint32_t)h, DR_BG);
    for (y = 0; y < h; y++)
        for (x = 0; x < 240; x++)
            cv_px[y * 240 + x] = swap16(dr_scene_px(x, y0 + y));
    dr_sprite_at(dr.ch, &sx, &sy);
    if (sy + s->h > y0 && sy < y0 + h)
        for (i = 0; i < s->n; i++) {                    /* the runs, row-major from the sprite's top left */
            uint32_t idx = s->run[i] >> 4, len = (s->run[i] & 15u) + 1u, j;
            if (idx)
                for (j = 0; j < len; j++) {
                    int32_t p = pos + (int32_t)j, py = sy + p / s->w - y0, px = sx + p % s->w;
                    if (py >= 0 && py < h)
                        cv_px[py * 240 + px] = swap16(idx == 1u ? s->pal[0] :     /* (1: the outline) */
                                                      dr_sweep_px(sweep, p % s->w, p / s->w, s->pal[idx - 1u]));
                }
            pos += (int32_t)len;
            if (sy + pos / s->w >= y0 + h)
                break;
        }
}

/* the spellbook modes on ALGORITHM (seq.c dr_kmode: how the keys play; SCRY and TRACE, below) */
static const char *const DR_KMODE_NAME[5] = {"SOLO", "WEAVE", "CAST", "SCRY", "TRACE"};
static const char *const DR_KMODE_HELP[5] = {"SOLO: every key plays its note", "WEAVE: black keys pick, white strum",
                                             "CAST: black keys hold, white sing", "SCRY: listen, then find it",
                                             "TRACE: follow the lit keys"};
/* SELECT: the key, round the circle of fifths; flat keys spelled with flats (the big font has no lower-case b: the
 * root's letter is drawn large, its accidental small, before the chord's quality) */
static const uint8_t DR_FIFTHS[12] = {0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5};
static const char *const DR_SHARP[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const char *const DR_FLAT[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
static uint8_t dr_fifth;                     /* DR_FIFTHS index: 0 C .. 11 F */
static const char *dr_pc_name(uint32_t pc)
{
    return dr_fifth >= 7u ? DR_FLAT[pc % 12u] : DR_SHARP[pc % 12u];   /* Db Ab Eb Bb F: flats; C .. F#: sharps */
}
/* seq.c's chords (dr_chord_tones ids): their names and their place in the key */
static const char *const DR_CHORD_Q[11] = {"", "", "m", "", "m", "maj7", "7", "m7", "maj7", "m7", "m"};
static const char *const DR_CHORD_RN[11] = {"IV", "V", "vi", "I", "ii", "IVmaj7", "V7", "vi7", "Imaj7", "ii7", "iv"};

/* ------------------------------------------------- SCRY and TRACE --- */
static void dr_say(const char *s);
/* The player: SCRY's questions and TRACE's demonstrations, as notes into Melodee's MIDI input queue (usb.c
 * midi_enqueue: channel 1 is track 1, the character), from the main loop at their times. A few notes at once. */
#include "dr_songs.h"                        /* DR_SONGS (tools/gen_dr_songs.py) */
#define DR_PN 8u
static struct { uint8_t note, state; uint32_t on, off; } drp[DR_PN];   /* state 0 free, 1 waiting, 2 sounding */
static void dr_midi(uint32_t note, uint32_t on)
{
    if (on)
        drm.sweep_t0 = fm1_ms | 1u;                      /* (Shard's light, for the player's notes too) */
    (void)midi_enqueue(on ? 0x09u | 0x90u << 8 | (note & 127u) << 16 | 100u << 24 : 0x08u | 0x80u << 8 | (note & 127u) << 16, 1);
}
static void dr_play(uint32_t note, uint32_t after_ms, uint32_t ms)
{
    uint32_t i;
    for (i = 0; i < DR_PN; i++)
        if (!drp[i].state) {
            drp[i].note = (uint8_t)note;
            drp[i].on = fm1_ms + after_ms;
            drp[i].off = fm1_ms + after_ms + ms;
            drp[i].state = 1;
            return;
        }
}
static void dr_play_stop(void)                /* everything waiting forgotten, everything sounding ended */
{
    uint32_t i;
    for (i = 0; i < DR_PN; i++) {
        if (drp[i].state == 2u)
            dr_midi(drp[i].note, 0);
        drp[i].state = 0;
    }
}
static int dr_play_poll(void)                 /* due notes on and off; 1 while anything is left */
{
    uint32_t i, busy = 0;
    for (i = 0; i < DR_PN; i++) {
        if (drp[i].state == 1u && (int32_t)(fm1_ms - drp[i].on) >= 0) {
            dr_midi(drp[i].note, 1);
            drp[i].state = 2;
        }
        if (drp[i].state == 2u && (int32_t)(fm1_ms - drp[i].off) >= 0) {
            dr_midi(drp[i].note, 0);
            drp[i].state = 0;
        }
        busy |= drp[i].state;
    }
    return busy != 0u;
}
static uint32_t dr_rand(void)
{
    static uint32_t x = 0x2545F491u;
    x ^= fm1_ms * 2654435761u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

/* SCRY, the ear game: three drills on OCT- / OCT+, each growing its pool after three right in a row.
 *   NOTES      the key's home note, then a mystery note: find it (any octave).
 *   INTERVALS  two notes, the first lit: play the second (any octave); the answer names the interval.
 *   CHORDS     a chord of the key: answer with the black key that holds it in CAST (the black keys play their chords).
 * PLAY asks again; after two misses the right key glows. */
enum { DRS_NOTES, DRS_INTERVALS, DRS_CHORDS };
static const char *const DR_DRILL[3] = {"NOTES", "INTERVALS", "CHORDS"};
static const char *const DR_SCRY_LINE[3] = {"NOTES: the home note, then find the other", "INTERVALS: play the second note",
                                            "CHORDS: find the chord's black key"};
static const uint8_t DR_DEG[8] = {0, 7, 4, 2, 9, 5, 11, 12};          /* NOTES: the scale's notes, easiest first */
static const uint8_t DR_IVL[12] = {7, 12, 4, 5, 3, 2, 9, 10, 11, 8, 1, 6};   /* INTERVALS: in semitones */
static const char *const DR_IVL_NAME[13] = {"UNISON", "MINOR SECOND", "MAJOR SECOND", "MINOR THIRD", "MAJOR THIRD",
                                            "PERFECT FOURTH", "TRITONE", "PERFECT FIFTH", "MINOR SIXTH", "MAJOR SIXTH",
                                            "MINOR SEVENTH", "MAJOR SEVENTH", "OCTAVE"};
static const uint8_t DR_QCH[11] = {3, 0, 1, 2, 4, 6, 10, 8, 7, 5, 9};   /* CHORDS: I IV V vi ii V7 iv Imaj7 vi7 .. */
static const uint8_t DR_POOL0[3] = {2, 2, 4}, DR_POOLN[3] = {8, 12, 11};
static struct {
    uint8_t drill, pool[3], streak, best, run, misses, hint, asked, reveal;
    uint8_t a, b;                            /* NOTES: tonic, target notes; INTERVALS: the two notes; CHORDS: b = chord */
    uint32_t next_ms;                        /* a new question (or the same again) at this time; 0 = none due */
} drs;
static void dr_scry_ask(int again)
{
    uint32_t d = drs.drill, i, root;
    int8_t iv[4];
    dr_play_stop();
    if (!again) {
        uint32_t pick = dr_rand() % drs.pool[d];
        drs.misses = 0;
        drs.hint = 0xFF;
        if (d == DRS_NOTES) {
            uint32_t t = 60u + dr_key;
            if (t > 66u)
                t -= 12u;
            drs.a = (uint8_t)t;
            drs.b = (uint8_t)(t + DR_DEG[pick]);
        } else if (d == DRS_INTERVALS) {
            uint32_t ivl = DR_IVL[pick];
            drs.a = (uint8_t)(53u + dr_rand() % (27u - ivl));
            drs.b = (uint8_t)(drs.a + ivl);
        } else
            drs.b = DR_QCH[pick];
    }
    if (d == DRS_CHORDS) {
        uint32_t n = dr_chord_tones(drs.b, &root, iv);
        for (i = 0; i < n; i++)
            dr_play(53u + (root + 7u) % 12u + (uint32_t)iv[i], 0, 1100);
    } else {
        dr_play(drs.a, 0, 520);
        dr_play(drs.b, 640, 760);
    }
    drs.asked = 1;
    drs.reveal = 0;
    drs.next_ms = 0;
    drm.dirty |= DR_D_NOTE;
}
static void dr_scry_answer(uint32_t k)        /* key k pressed during a question */
{
    uint32_t right, d = drs.drill;
    char m[40];
    if (!drs.asked)
        return;
    if (d == DRS_CHORDS) {
        int32_t id = dr_chord_of_key(k);
        if (id < 0)
            return;                                      /* (white keys: free to play along) */
        right = (uint32_t)id == drs.b;
    } else
        right = (53u + k) % 12u == drs.b % 12u;
    if (right) {
        drs.asked = 0;
        drs.reveal = 1;
        drs.hint = 0xFF;
        drs.streak++;
        if (drs.streak > drs.best)
            drs.best = drs.streak;
        str_cpy(m, "YES: ", 6);
        if (d == DRS_NOTES)
            str_cpy(m + 5, dr_pc_name(drs.b % 12u), 3);
        else if (d == DRS_INTERVALS)
            str_cpy(m + 5, DR_IVL_NAME[drs.b - drs.a], 20);
        else {
            uint32_t root;
            int8_t iv[4];
            dr_chord_tones(drs.b, &root, iv);
            str_cpy(m + 5, dr_pc_name(root), 3);
            str_cpy(m + str_len(m), DR_CHORD_Q[drs.b % 11u], 6);
            str_cpy(m + str_len(m), "  ", 3);
            str_cpy(m + str_len(m), DR_CHORD_RN[drs.b % 11u], 8);
        }
        if (++drs.run >= 3u && drs.pool[d] < DR_POOLN[d]) {   /* three in a row: one more to tell apart */
            drs.run = 0;
            drs.pool[d]++;
            str_cpy(m + str_len(m), "  +1 NEW", 9);
        }
        dr_say(m);
        drs.next_ms = fm1_ms + 1400u;
    } else {
        drs.streak = 0;
        drs.run = 0;
        if (++drs.misses >= 2u)                          /* the right key glows */
            drs.hint = (uint8_t)(d == DRS_CHORDS ? 0xFE : (drs.b - 53u));
        dr_say(drs.misses >= 2u ? "THE GLOWING KEY. LISTEN AGAIN." : "NOT THAT ONE. LISTEN AGAIN.");
        drs.next_ms = fm1_ms + 700u;
        drs.asked = 0;
        drs.reveal = 2;                                  /* (2: ask the same again) */
    }
    drm.dirty |= DR_D_NOTE;
}

/* TRACE: a song to learn at your own pace; the next key lights, a wrong key changes nothing. OCT- / OCT+ the song,
 * PLAY hears it (any key stops it). At the end, a moment, then from the top. */
static struct {
    uint8_t song, pos, done, demo, dpos;
    uint32_t done_ms, dnext;
} drt;
static uint32_t dr_trace_skip(uint32_t pos)   /* the next note from pos, past rests (n: the end) */
{
    const dr_song_t *sg = &DR_SONGS[drt.song % DR_NSONGS];
    while (pos < sg->n && !sg->nd[2u * pos])
        pos++;
    return pos;
}
static int32_t dr_trace_key(void)             /* the key of the next note, -1 none */
{
    const dr_song_t *sg = &DR_SONGS[drt.song % DR_NSONGS];
    if (drt.demo)
        return drt.dpos && sg->nd[2u * (drt.dpos - 1u)] ? (int32_t)sg->nd[2u * (drt.dpos - 1u)] - 53 : -1;
    return drt.pos < sg->n ? (int32_t)sg->nd[2u * drt.pos] - 53 : -1;
}
static void dr_trace_start(uint32_t song)
{
    dr_play_stop();
    drt.song = (uint8_t)(song % DR_NSONGS);
    drt.pos = (uint8_t)dr_trace_skip(0);
    drt.done = drt.demo = 0;
    drm.dirty |= DR_D_NOTE | DR_D_LINE;
}
static void dr_trace_press(uint32_t k)
{
    const dr_song_t *sg = &DR_SONGS[drt.song % DR_NSONGS];
    if (drt.demo) {                                      /* a key stops the demonstration */
        drt.demo = 0;
        dr_play_stop();
        drm.dirty |= DR_D_NOTE;
        return;
    }
    if (drt.done || drt.pos >= sg->n || (int32_t)k != (int32_t)sg->nd[2u * drt.pos] - 53)
        return;
    drt.pos = (uint8_t)dr_trace_skip(drt.pos + 1u);
    if (drt.pos >= sg->n) {
        drt.done = 1;
        drt.done_ms = fm1_ms;
        dr_say("THE END. AGAIN FROM THE TOP");
    }
    drm.dirty |= DR_D_NOTE;
}
static void dr_trace_poll(void)
{
    const dr_song_t *sg = &DR_SONGS[drt.song % DR_NSONGS];
    if (drt.done && fm1_ms - drt.done_ms > 2600u)
        dr_trace_start(drt.song);
    if (drt.demo && (int32_t)(fm1_ms - drt.dnext) >= 0) {   /* the demonstration: a note at a time */
        uint32_t eighth = 30000u / sg->bpm, len;
        if (drt.dpos >= sg->n) {
            drt.demo = 0;
            drm.dirty |= DR_D_NOTE;
            return;
        }
        len = sg->nd[2u * drt.dpos + 1u] * eighth;
        if (sg->nd[2u * drt.dpos])
            dr_play(sg->nd[2u * drt.dpos], 0, len * 9u / 10u);
        drt.dpos++;
        drt.dnext = fm1_ms + len;
        drm.dirty |= DR_D_NOTE;
    }
}

/* how the keys play: entering and leaving SCRY and TRACE set them up and clean them away */
static void dr_kmode_set(uint32_t m)
{
    dr_play_stop();
    dr_castkeys = 0;
    drt.demo = 0;
    dr_kmode = (uint8_t)m;
    if (m == DRK_SCRY || m == DRK_TRACE)
        song.octave = 0;                                 /* (the keys at their own notes: OCT picks the drill or the song) */
    if (m == DRK_SCRY) {
        if (!drs.pool[0]) {
            drs.pool[0] = DR_POOL0[0];
            drs.pool[1] = DR_POOL0[1];
            drs.pool[2] = DR_POOL0[2];
        }
        dr_castkeys = drs.drill == DRS_CHORDS;
        drs.asked = 0;
        drs.next_ms = fm1_ms + 900u;
        drs.reveal = 0;
    } else if (m == DRK_TRACE)
        dr_trace_start(drt.song);
    drm.dirty |= DR_D_NOTE | DR_D_LINE;
}


/* what was just played, as Melodee's HOME names it (ui_graph.c chord_of): a chord, else the notes; 0 = none.
 * WEAVE and CAST: the chord the black keys picked, and its numeral (rn) */
static uint32_t dr_note_label(char *big, char *small, const char **rn)
{
    uint32_t bits[4], i, n = 0, pcs = 0, root = 0, bass = 0;
    const char *q;
    fm1_irq_off();
    for (i = 0; i < 4u; i++)
        bits[i] = live_last[i];
    fm1_irq_on();
    for (i = 0; i < 128u; i++)
        if ((bits[i >> 5] >> (i & 31u)) & 1u) {
            if (!n)
                bass = i;
            pcs |= 1u << (i % 12u);
            n++;
        }
    big[0] = small[0] = 0;
    *rn = 0;
    if (drum_track(&trk[0])) {                           /* a drum: the one last struck, by name */
        if (drm.drum)
            str_cpy(big, drum_lane_name(&trk[0], drm.drum - 1u), 8);
        return drm.drum;
    }
    if (dr_kmode == DRK_WEAVE || dr_kmode == DRK_CAST) {
        uint32_t r;
        int8_t iv[4];
        dr_chord_tones(dr_chord, &r, iv);
        big[0] = dr_pc_name(r)[0];
        big[1] = 0;
        str_cpy(small, dr_pc_name(r) + 1, 3);           /* (its accidental, if any) */
        str_cpy(small + str_len(small), DR_CHORD_Q[dr_chord % 11u], 8);
        *rn = DR_CHORD_RN[dr_chord % 11u];
        return 1;
    }
    if (!n)
        return 0;
    q = n > 1u ? chord_of(pcs, bass % 12u, &root) : 0;
    if (q) {
        str_cpy(big, N_NOTE[root], 4);
        str_cpy(small, q, 12);
    } else
        note_name(big, bass);
    return n;
}

/* SCRY and TRACE's chip: what to do or what it was (a), the score or the place in the song (b) */
static void dr_quiz_lines(char *a, char *b)
{
    if (dr_kmode == DRK_SCRY) {
        static const char *const Q[3] = {"WHICH NOTE?", "AND THE SECOND?", "WHICH CHORD?"};
        if (drs.reveal == 1u) {                          /* right: the answer, named */
            if (drs.drill == DRS_NOTES)
                str_cpy(a, dr_pc_name(drs.b % 12u), 24);
            else if (drs.drill == DRS_INTERVALS)
                str_cpy(a, DR_IVL_NAME[(drs.b - drs.a) % 13u], 24);
            else {
                uint32_t root;
                int8_t iv[4];
                dr_chord_tones(drs.b, &root, iv);
                str_cpy(a, dr_pc_name(root), 24);
                str_cpy(a + str_len(a), DR_CHORD_Q[drs.b % 11u], 8);
            }
        } else
            str_cpy(a, drs.asked ? Q[drs.drill % 3u] : "LISTEN", 24);
        str_cpy(b, "STREAK ", 24);
        fmt_int(b + str_len(b), drs.streak);
        str_cpy(b + str_len(b), "   BEST ", 9);
        fmt_int(b + str_len(b), drs.best);
    } else {
        const dr_song_t *sg = &DR_SONGS[drt.song % DR_NSONGS];
        int32_t k = dr_trace_key();
        uint32_t i, at = 0, all = 0;
        if (drt.demo)
            str_cpy(a, "LISTEN", 24);
        else if (drt.done || k < 0)
            str_cpy(a, "THE END", 24);
        else {
            str_cpy(a, "NEXT  ", 24);
            str_cpy(a + str_len(a), (sg->flats ? DR_FLAT : DR_SHARP)[(53u + (uint32_t)k) % 12u], 3);
        }
        for (i = 0; i < sg->n; i++)                      /* (rests not counted) */
            if (sg->nd[2u * i]) {
                all++;
                at += i < (drt.demo ? drt.dpos : drt.pos);
            }
        str_cpy(b, "NOTE ", 24);
        fmt_int(b + str_len(b), (int32_t)(at + (at < all && !drt.demo)));
        str_cpy(b + str_len(b), " OF ", 5);
        fmt_int(b + str_len(b), (int32_t)all);
    }
}

/* the note label: a chip at the scene's top left (redrawn alone when it changes); the mode's at the top right */
static void dr_draw_note(void)
{
    char big[8], small[12];
    const char *rn;
    dr_scene_rows(DR_Y_SCENE, 40);
    if (dr_kmode == DRK_SCRY || dr_kmode == DRK_TRACE) {
        char a[24], b[24];
        int32_t w, wb;
        dr_quiz_lines(a, b);
        w = text_w(&AF_M, a);
        wb = text_w(&AF_S, b);
        w = (w > wb ? w : wb) + 14;
        cv_rrect(6, 3, w, 34, 6, DR_SURF, DR_BG);
        cv_text_on(13, 4, &AF_M, a, DR_GOLD, DR_SURF);
        cv_text_on(13, 21, &AF_S, b, DR_TEAL, DR_SURF);
    } else if (dr_note_label(big, small, &rn)) {
        int32_t w = text_w(&AF_L, big) + (small[0] ? text_w(&AF_M, small) + 2 : 0) + (rn ? text_w(&AF_M, rn) + 8 : 0) + 14;
        cv_rrect(6, 4, w, 32, 6, DR_SURF, DR_BG);
        w = cv_text_on(13, 2, &AF_L, big, DR_TEXT, DR_SURF);
        if (small[0])
            w = cv_text_on(w + 2, 13, &AF_M, small, DR_GOLD, DR_SURF);
        if (rn)
            cv_text_on(w + 8, 13, &AF_M, rn, DR_TEAL, DR_SURF);
    }
    if (dr_kmode != DRK_SOLO) {                           /* the mode and its key: "WEAVE  G" */
        char m[20];
        int32_t w;
        str_cpy(m, DR_KMODE_NAME[dr_kmode % 5u], 8);
        if (dr_kmode == DRK_SCRY)                        /* "SCRY" (its drill on the line), "TRACE  3/14", "WEAVE  G" */
            ;
        else if (dr_kmode == DRK_TRACE) {
            str_cpy(m + str_len(m), "  ", 3);
            fmt_int(m + str_len(m), drt.song + 1);
            str_cpy(m + str_len(m), "/", 2);
            fmt_int(m + str_len(m), (int32_t)DR_NSONGS);
        } else {
            str_cpy(m + str_len(m), "  ", 3);
            str_cpy(m + str_len(m), dr_pc_name(dr_key), 3);
        }
        w = text_w(&AF_S, m) + 12;
        cv_rrect(234 - w, 4, w, 18, 5, DR_SURF, DR_BG);
        cv_text_on(240 - w, 5, &AF_S, m, DR_TEAL, DR_SURF);
    }
    cv_blit(0, DR_Y_SCENE);
}

static void dr_draw_head(void)
{
    const dr_char_t *c = &DR_CAST[dr.ch];
    char n[8];
    int32_t w = text_w(&AF_S, c->tag);
    fmt_int(n, dr.ch + 1);
    str_cpy(n + str_len(n), "/", 2);
    fmt_int(n + str_len(n), (int32_t)DR_NCHAR);
    cv_begin(240, DR_H_HEAD, DR_BG);
    cv_text_on(8, 2, &AF_M, c->name, DR_GOLD, DR_BG);
    cv_rrect(196 - w - 10, 3, w + 10, 16, 4, DR_SURF, DR_BG);
    cv_text_on(196 - w - 5, 4, &AF_S, c->tag, DR_TEAL, DR_SURF);
    cv_text_on(205, 4, &AF_S, n, DR_DIM, DR_BG);
    cv_rect(0, DR_H_HEAD - 1, 240, 1, DR_PLUM);
    cv_blit(0, 0);
}

/* the line under the header: the character's, or a message for a moment (OCT, the HOME + SAVE countdown) */
static void dr_draw_line(void)
{
    cv_begin(240, 16, DR_BG);
    if (drm.msg[0])
        cv_text_on(8, 1, &AF_S, drm.msg, DR_TEAL, DR_BG);
    else if (drm.codex)
        cv_text_on(8, 1, &AF_S, "CODEX", DR_TEAL, DR_BG);
    else if (dr_kmode == DRK_SCRY)
        cv_text_on(8, 1, &AF_S, DR_SCRY_LINE[drs.drill % 3u], DR_MID, DR_BG);
    else if (dr_kmode == DRK_TRACE)
        cv_text_on(8, 1, &AF_S, DR_SONGS[drt.song % DR_NSONGS].title, DR_MID, DR_BG);
    else
        cv_text_on(8, 1, &AF_S, DR_CAST[dr.ch].line, DR_MID, DR_BG);
    cv_blit(0, DR_Y_LINE);
}

/* "RESO|ENV AMT" -> "RESO", "ENV AMT" (16 bytes each) */
static void dr_under_lines(const char *u, char *a, char *b)
{
    uint32_t j = 0;
    while (u[j] && u[j] != '|')
        j++;
    str_cpy(a, u, j + 1 < 16u ? j + 1 : 16u);
    str_cpy(b, u[j] ? u + j + 1 : "", 16);
}

/* every text Double Rainbow draws, measured in its font against its room: how many do not fit (0 = all do;
 * web/emu/dr_test.mjs checks it) */
static uint32_t dr_text_check(void)
{
    uint32_t k, i, bad = 0;
    char a[16], b[16];
    for (k = 0; k < DR_NCHAR; k++) {
        const dr_char_t *c = &DR_CAST[k];
        const dr_codex_t *x = &DR_CODEX[k];
        bad += text_w(&AF_M, c->name) > 150;
        bad += k < 5u && text_w(&AF_S, DR_KMODE_HELP[k]) > 224;
        bad += text_w(&AF_S, c->line) > 224;
        bad += k < 3u && text_w(&AF_S, DR_SCRY_LINE[k]) > 224;
        bad += text_w(&AF_L, x->title) > 224;
        bad += text_w(&AF_M, x->entry) > 224;
        bad += text_w(&AF_S, x->when) > 224;
        for (i = 0; i < 6u; i++)
            bad += text_w(&AF_S, x->lore[i]) > 224;
        bad += text_w(&AF_S, x->sub) > 224;
        for (i = 0; i < 6u; i++)
            bad += text_w(&AF_S, x->body[i]) > 224;
        for (i = 0; i < 4u; i++)
            bad += text_w(&AF_S, x->tip[i]) > 216;
        for (i = 0; i < 4u; i++) {
            bad += text_w(&AF_S, c->m[i].name) > 51;
            dr_under_lines(c->m[i].under, a, b);
            bad += text_w(&AF_S, a) > 51;
            bad += text_w(&AF_S, b) > 51;
        }
    }
    for (k = 0; k < DR_NSONGS; k++)                      /* TRACE: the titles on the line, who wrote them */
        bad += text_w(&AF_S, DR_SONGS[k].title) > 224 || text_w(&AF_S, DR_SONGS[k].who) > 224;
    for (k = 0; k < 13u; k++)                            /* SCRY: its answers in the chip (left of "SCRY"), on the line */
        bad += text_w(&AF_M, DR_IVL_NAME[k]) > 160 || text_w(&AF_S, DR_IVL_NAME[k]) > 150;
    return bad;
}

static void dr_draw_cards(void)
{
    const dr_char_t *c = &DR_CAST[dr.ch];
    uint32_t i;
    cv_begin(240, 240 - DR_Y_CARDS, DR_BG);
    for (i = 0; i < 4u; i++) {
        int32_t x = 3 + (int32_t)i * 59, v = dr.mv[dr.ch][i];
        cv_rrect(x, 2, 55, 37, 5, DR_SURF, DR_BG);
        cv_text_on(x + 5, 3, &AF_S, c->m[i].name, drm.under ? DR_MID : DR_GOLD, DR_SURF);
        if (drm.under) {                                 /* EDIT held: "RESO|ENV AMT" as two lines */
            char a[16], b[16];
            dr_under_lines(c->m[i].under, a, b);
            cv_text_fit(x + 3, 15, &AF_S, a, DR_TEXT, DR_SURF, 51);
            if (b[0])
                cv_text_fit(x + 3, 26, &AF_S, b, DR_TEXT, DR_SURF, 51);
        } else {
            char s[8];
            const dr_route_t *pr = &c->m[i].r[0];
            if (pr->kind == DR_PRESET) {                 /* the tone: A-1 .. H-8, as the CZ-1 numbers them */
                s[0] = (char)('A' + (v - 1) / 8);
                s[1] = '-';
                s[2] = (char)('1' + (v - 1) % 8);
                s[3] = 0;
                v = (v - pr->lo) * 100 / (pr->hi - pr->lo);
            } else {
                fmt_int(s, v);
                str_cpy(s + str_len(s), "%", 2);
            }
            cv_text_on(x + 5, 15, &AF_M, s, DR_TEXT, DR_SURF);
            cv_rect(x + 5, 33, 45, 2, DR_PLUM);
            if (v)
                cv_rect(x + 5, 33, 45 * v / 100, 2, DR_GOLD);
        }
    }
    cv_blit(0, DR_Y_CARDS);
}

/* the codex: page 0 the ledger, 1 the engine's story, 2 something to try; the whole area under the line */
static void dr_draw_codex(void)
{
    const dr_codex_t *x = &DR_CODEX[dr.ch];
    uint32_t i;
    char pg[24] = "PRESETS: page 1/3";
    cv_begin(240, 100, DR_BG);
    if (drm.page == 0) {
        cv_text_on(8, 0, &AF_S, "THE JARKEEPER'S LEDGER", DR_TEAL, DR_BG);
        cv_text_on(8, 17, &AF_M, x->entry, DR_GOLD, DR_BG);
        cv_text_on(8, 37, &AF_S, x->when, DR_MID, DR_BG);
        for (i = 0; i < 3u; i++)
            cv_text_on(8, 55 + (int32_t)i * 15, &AF_S, x->lore[i], DR_TEXT, DR_BG);   /* (the third ends at row 100) */
    } else if (drm.page == 1) {
        cv_text_on(8, 0, &AF_L, x->title, DR_GOLD, DR_BG);
        cv_text_on(8, 33, &AF_S, x->sub, DR_MID, DR_BG);
        for (i = 0; i < 3u; i++)
            cv_text_on(8, 54 + (int32_t)i * 15, &AF_S, x->body[i], DR_TEXT, DR_BG);
    } else {
        cv_rrect(4, 2, 232, 96, 6, DR_SURF, DR_BG);
        cv_text_on(12, 6, &AF_S, "TRY THIS", DR_TEAL, DR_SURF);
        for (i = 0; i < 4u; i++)
            cv_text_on(12, 26 + (int32_t)i * 16, &AF_S, x->tip[i], DR_TEXT, DR_SURF);
    }
    cv_blit(0, DR_Y_SCENE);
    cv_begin(240, 102, DR_BG);
    for (i = 3; drm.page < 2u && i < 6u; i++)
        cv_text_on(8, (int32_t)(i - 3u) * 15, &AF_S, drm.page ? x->body[i] : x->lore[i], DR_TEXT,
                   DR_BG);
    pg[14] = (char)('1' + drm.page);
    cv_text_on(8, 84, &AF_S, pg, DR_DIM, DR_BG);
    cv_text_r(232, 84, &AF_S, "SCL: close", DR_DIM, DR_BG);
    cv_blit(0, DR_Y_SCENE + 100);
}

static void dr_say(const char *s)
{
    str_cpy(drm.msg, s, sizeof drm.msg);
    drm.msg_t0 = fm1_ms;
    drm.dirty |= DR_D_LINE;
}

/* the LEDs: every key glows, a held key is lit; the buttons Double Rainbow uses glow, SCL lit in the codex,
 * EDIT while held, OCT- / OCT+ while the keys are shifted */
static void dr_leds(void)
{
    static const uint8_t USED[] = {B_SCL, B_EDIT, B_HOME, B_SAVE, B_OCTDN, B_OCTUP};
    uint8_t nl[FM1_NCOL] = {0}, nd[FM1_NCOL] = {0}, nb[FM1_NCOL] = {0};
    uint32_t k, c, quiz = dr_kmode == DRK_SCRY || dr_kmode == DRK_TRACE, guide = 0, blink = (fm1_ms / 160u) & 1u;
    if (dr_kmode == DRK_SCRY) {                           /* SCRY: the home note or the first note lit, the hint blinking */
        if (drs.asked && drs.drill != DRS_CHORDS)
            guide |= 1u << (drs.a - 53u);
        if (drs.hint != 0xFF && blink) {
            if (drs.hint != 0xFE)
                guide |= 1u << drs.hint;
            else
                for (k = 0; k < 27u; k++)
                    guide |= (uint32_t)(dr_chord_of_key(k) == (int32_t)drs.b) << k;
        }
    } else if (dr_kmode == DRK_TRACE && dr_trace_key() >= 0)   /* TRACE: the next key (the demo: the note it plays) */
        guide = 1u << dr_trace_key();
    for (k = 0; k < 27u; k++) {
        led_put(nl, 14u + k, (int)(((fm1_in.notes | guide) >> k) & 1u));
        led_put(nd, 14u + k, 1);
    }
    for (k = 0; k < NELEM(USED); k++)
        led_put(nb, panel.btn[USED[k]], 1);
    led_put(nb, panel.btn[B_PLAY], (int)quiz);
    led_put(nl, panel.btn[B_PLAY], dr_kmode == DRK_TRACE && drt.demo);
    led_put(nl, panel.btn[B_SCL], drm.codex);
    led_put(nl, panel.btn[B_EDIT], drm.under);
    led_put(nl, panel.btn[B_OCTDN], song.octave < 0);
    led_put(nl, panel.btn[B_OCTUP], song.octave > 0);
    fm1_led_dim_mask[0] = LIGHTS_MASK[settings_lights % LIGHTS_N];
    fm1_led_dim_mask[1] = BTN_DIM_MASK;
    for (c = 0; c < FM1_NCOL; c++) {
        fm1_led_dim[0][c] = (uint8_t)(nd[c] | nl[c]);
        fm1_led_dim[1][c] = nb[c];
        fm1_led[c] = nl[c];
    }
}

/* power-on (main.c, after project_boot): the first character, the screen to draw */
static void dr_boot(void)
{
    led_pos_init();                                      /* (ui_leds sets it up on its first call; Double Rainbow's come first) */
    dr_load(0);
    drm.on = 1;
    drm.dirty = DR_D_ALL;
}

/* to Melodee: track 1 back to Melodee's own start (its default sound, then the BOOT project or the template, as
 * project_boot loads them at power-on); nothing is saved. Double Rainbow comes back at the next power-on. */
static void dr_leave(void)
{
    track_t *t = &trk[0];
    drm.on = 0;
    dr_kmode_set(DRK_SOLO);                              /* (Melodee's keys as always; SCRY's and TRACE's notes ended) */
    song.octave = 0;
    dr_key = 0;
    undo_depth++;
    set_engine_of(t, TRK_DEF[0][0]);
    apply_preset_to(t, TRK_DEF[0][1]);
    undo_depth--;
    project_boot();
    lcd_fill(0, 0, 240, 240, T_BG);
    ui.home = 1;
    ui.force = 1;
}

/* one main-loop pass while Double Rainbow is on: input, the LEDs, what changed on the screen. 0: Melodee's UI */
static int dr_frame(void)
{
    uint32_t released, pressed, notes, both, sig, i;
    int32_t s;
    char b[8];
    if (!drm.on)
        return 0;
    kb_mask = perf_mask = 0;                             /* every key plays, whatever is held */
    song.grid = 0;
    pressed = fm1_input_edges(&released);
    notes = fm1_input_note_edges();
    both = (1u << panel.btn[B_HOME]) | (1u << panel.btn[B_SAVE]);
    if ((fm1_in.buttons & both) == both) {               /* HOME + SAVE held: the countdown, then Melodee */
        int32_t held = (int32_t)(fm1_ms - drm.combo_t0); /* (signed: t0 is fm1_ms | 1, maybe a millisecond ahead) */
        if (!drm.combo_t0)
            drm.combo_t0 = fm1_ms | 1u;
        else if (held >= (int32_t)DR_HOLD_MS) {
            drm.combo_t0 = 0;
            dr_leave();
            return 1;
        } else {
            char m[24] = "MELODEE IN ";
            m[11] = (char)('0' + (DR_HOLD_MS - (uint32_t)(held > 0 ? held : 0) + 999u) / 1000u);
            m[12] = 0;
            if (!drm.msg[0] || drm.msg[11] != m[11])
                dr_say(m);
        }
    } else if (drm.combo_t0) {
        drm.combo_t0 = 0;
        dr_say("");
    }
    if (drm.msg[0] && !drm.combo_t0 && fm1_ms - drm.msg_t0 > 1500u) {
        drm.msg[0] = 0;
        drm.dirty |= DR_D_LINE;
    }
    if (pressed & (1u << panel.btn[B_SCL])) {            /* the codex, open and close */
        drm.codex ^= 1u;
        drm.page = 0;
        drm.dirty = DR_D_ALL;
    }
    if (((fm1_in.buttons >> panel.btn[B_EDIT]) & 1u) != drm.under) {   /* EDIT held: what the knobs move */
        drm.under ^= 1u;
        drm.dirty |= DR_D_CARDS;
    }
    for (i = 0; i < 2u; i++) {                           /* OCT- / OCT+: the keys an octave down or up */
        uint32_t bt = i ? B_OCTUP : B_OCTDN;
        if ((pressed & (1u << panel.btn[bt])) && dr_kmode == DRK_SCRY) {   /* SCRY: the drill */
            drs.drill = (uint8_t)((drs.drill + (i ? 1u : 2u)) % 3u);
            dr_kmode_set(DRK_SCRY);
            dr_say(DR_DRILL[drs.drill]);
        } else if ((pressed & (1u << panel.btn[bt])) && dr_kmode == DRK_TRACE) {   /* TRACE: the song */
            dr_trace_start(drt.song + (i ? 1u : DR_NSONGS - 1u));
            dr_say(DR_SONGS[drt.song].who);
        } else if (pressed & (1u << panel.btn[bt])) {
            song.octave += i ? (song.octave < 2 ? 1 : 0) : (song.octave > -2 ? -1 : 0);
            str_cpy(b, "OCT ", 5);
            b[4] = song.octave < 0 ? '-' : '+';
            b[5] = (char)('0' + (song.octave < 0 ? -song.octave : song.octave));
            b[6] = 0;
            dr_say(song.octave ? b : "OCT 0");
        }
    }
    if ((s = fm1_enc_take(panel.enc[EN_PRESET]) * panel.dir[EN_PRESET]) != 0) {
        if (drm.codex) {                                 /* the codex: its pages */
            drm.page = (uint8_t)clamp((int32_t)drm.page + (s > 0 ? 1 : -1), 0, 2);
            drm.dirty = DR_D_ALL;
        } else {                                         /* the next or the previous character */
            dr_load((dr.ch + (s > 0 ? 1u : DR_NCHAR - 1u)) % DR_NCHAR);
            drm.drum = 0;
            drm.dirty = DR_D_ALL;
        }
    }
    for (i = 0; i < 4u; i++)                             /* the four knobs, 2 % a detent */
        if ((s = fm1_enc_take(panel.enc[EN_K1 + i]) * panel.dir[EN_K1 + i]) != 0) {
            const dr_route_t *pr = &DR_CAST[dr.ch].m[i].r[0];
            if (pr->kind == DR_PRESET) {                 /* a tone a detent: the character again with it */
                uint8_t v = (uint8_t)clamp(dr.mv[dr.ch][i] + s, pr->lo, pr->hi);
                if (v != dr.mv[dr.ch][i]) {
                    dr.mv[dr.ch][i] = v;
                    dr_load(dr.ch);
                    dr_say(ENGINES[DR_CAST[dr.ch].engine]->presets[v].name);
                }
            } else {
                dr.mv[dr.ch][i] = (uint8_t)clamp(dr.mv[dr.ch][i] + s * 2, 0, 100);
                dr_macro_apply(&trk[0], dr.ch, i);
            }
            drm.dirty |= DR_D_CARDS;
        }
    if ((s = fm1_enc_take(panel.enc[EN_ALGO]) * panel.dir[EN_ALGO]) != 0) {   /* ALGORITHM: how the keys play */
        uint32_t m = (uint32_t)clamp((int32_t)dr_kmode + (s > 0 ? 1 : -1), DRK_SOLO, DRK_TRACE);
        if (m != dr_kmode)
            dr_kmode_set(m);
        dr_say(DR_KMODE_HELP[dr_kmode]);
    }
    if (pressed & (1u << panel.btn[B_PLAY])) {           /* PLAY: SCRY asks again, TRACE plays the song (or stops) */
        if (dr_kmode == DRK_SCRY && !drm.codex)
            dr_scry_ask(drs.asked || drs.reveal == 2u);
        else if (dr_kmode == DRK_TRACE) {
            dr_play_stop();
            drt.demo ^= 1u;
            drt.dpos = 0;
            drt.dnext = fm1_ms;
            drm.dirty |= DR_D_NOTE;
        }
    }
    if ((s = fm1_enc_take(panel.enc[EN_SELECT]) * panel.dir[EN_SELECT]) != 0 && dr_kmode == DRK_TRACE) {
        dr_trace_start(drt.song + (s > 0 ? 1u : DR_NSONGS - 1u));   /* SELECT in TRACE: the song */
        dr_say(DR_SONGS[drt.song].who);
    } else if (s) {                                      /* SELECT: the key, by fifths */
        char m[24] = "KEY: ";
        dr_fifth = (uint8_t)((dr_fifth + (s > 0 ? 1u : 11u)) % 12u);
        dr_key = DR_FIFTHS[dr_fifth];
        str_cpy(m + 5, dr_pc_name(dr_key), 3);
        str_cpy(m + str_len(m), " MAJOR", 7);
        dr_say(m);
        drm.dirty |= DR_D_NOTE;
        if (dr_kmode == DRK_SCRY) {                      /* (a new question in the new key) */
            dr_play_stop();
            drs.asked = drs.reveal = 0;
            drs.next_ms = fm1_ms + 700u;
        }
    }
    if (notes)                                           /* (Shard: each note sends his light across) */
        drm.sweep_t0 = fm1_ms | 1u;
    if (notes && drum_track(&trk[0])) {                  /* (Tock: the drum the lowest new key strikes) */
        for (i = 0; !((notes >> i) & 1u); i++)
            ;
        drm.drum = (uint8_t)(drum_lane((uint32_t)drum_keys(&trk[0], i)) + 1u);
        drm.dirty |= DR_D_NOTE;
    }
    for (i = 0; notes && i < 27u; i++)                   /* the keys' presses: SCRY's answers, TRACE's next notes */
        if ((notes >> i) & 1u) {
            if (dr_kmode == DRK_SCRY && !drm.codex)
                dr_scry_answer(i);
            else if (dr_kmode == DRK_TRACE)
                dr_trace_press(i);
        }
    (void)dr_play_poll();
    if (dr_kmode == DRK_SCRY && drs.next_ms && !drm.codex && (int32_t)(fm1_ms - drs.next_ms) >= 0)
        dr_scry_ask(drs.reveal == 2u);
    if (dr_kmode == DRK_TRACE)
        dr_trace_poll();
    if (!drm.codex && fm1_ms - drm.wave_ms >= 30u) {      /* the horizon: ~30 redraws a second while it moves */
        drm.wave_ms = fm1_ms;
        if (drm.sweep_t0 && !(drm.dirty & DR_D_SCENE)) {    /* Shard's light crossing his chest (then one clean pass) */
            if (dr_sweep_pos() == -1000)
                drm.sweep_t0 = 0;
            if (dr.ch == 1u) {
                dr_scene_rows(DR_SWEEP_Y0, DR_SWEEP_H);
                cv_blit(0, DR_SWEEP_Y0);
            }
        }
        if (dr_wave_update() || (drm.sounding && !(drm.dirty & DR_D_SCENE))) {
            uint32_t any = 0;
            for (i = 0; i < 240u; i++)
                any |= dr_wave[i] != 0;
            drm.sounding = (uint8_t)(any != 0);          /* (silent again: this redraw lays it flat, then rests) */
            if (!(drm.dirty & DR_D_SCENE)) {
                dr_scene_rows(DR_Y_WAVE, DR_Y_CARDS - DR_Y_WAVE);
                cv_blit(0, DR_Y_WAVE);
            }
        }
    }
    sig = (dr_chord_gen * 31u + dr_kmode) * 13u + dr_key;
    for (i = 0; i < 4u; i++)
        sig = sig * 2654435761u + live_last[i];
    if (sig != drm.note_sig) {
        drm.note_sig = sig;
        drm.dirty |= DR_D_NOTE;
    }
    dr_leds();
    if (drm.dirty & DR_D_HEAD)
        dr_draw_head();
    if (drm.dirty & DR_D_LINE)
        dr_draw_line();
    if (drm.codex) {
        if (drm.dirty & DR_D_SCENE)
            dr_draw_codex();
    } else {
        if (drm.dirty & DR_D_SCENE) {
            dr_draw_note();
            dr_scene_rows(DR_Y_SCENE + 40, 80);            /* the scene's 160 rows: 40 (with the note), 80, 40 */
            cv_blit(0, DR_Y_SCENE + 40);
            dr_scene_rows(DR_Y_SCENE + 120, 40);
            cv_blit(0, DR_Y_SCENE + 120);
        } else if (drm.dirty & DR_D_NOTE)
            dr_draw_note();
    }
    if ((drm.dirty & DR_D_CARDS) && !drm.codex)          /* (the codex covers the cards' rows) */
        dr_draw_cards();
    drm.dirty = 0;
    return 1;
}
