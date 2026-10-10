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
enum { DR_NONE, DR_P5, DR_TP };
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
#define DR_NCHAR 3u

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
        int32_t x = rt->lo + (rt->hi - rt->lo) * v / 100;
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
    uint32_t i;
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
    apply_preset_to(t, c->preset);
    fm1_irq_off();
    if (c->engine == ENGI_PROPHET) {
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
};

static struct {
    uint8_t on, codex, page, under, dirty;   /* dirty: DR_D_* */
    uint8_t sounding;                        /* the horizon is tracing the wave (and needs one more flat redraw) */
    uint32_t wave_ms;
    uint32_t note_sig, combo_t0, msg_t0;
    char msg[24];
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
    int32_t peak = 0, x;
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
    for (x = 0; x < 240; x++) {
        int32_t v = 0;
        if (peak > 400) {
            int32_t full = peak > 12000 ? peak : 12000;     /* (quiet: smaller; loud: auto-scaled to the full height) */
            v = -snap[trig + (uint32_t)x] * DR_WAVE_PX / full;
        }
        if (v != dr_wave[x]) {
            dr_wave[x] = (int8_t)v;
            changed = 1;
        }
    }
    return (int)changed;
}

/* where the character stands: Squall floats */
static void dr_sprite_at(uint32_t k, int32_t *x, int32_t *y)
{
    const dr_sprite_t *s = &DR_SPRITES[k];
    *x = 120 - s->w / 2;
    *y = k == 2u ? 70 : 174 - s->h;
}

/* the scene's rows y0 .. y0 + h - 1 (h <= 80): background, then the character's runs over it */
static void dr_scene_rows(int32_t y0, int32_t h)
{
    const dr_sprite_t *s = &DR_SPRITES[dr.ch];
    int32_t sx, sy, x, y, pos = 0;
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
                        cv_px[py * 240 + px] = swap16(s->pal[idx - 1u]);
                }
            pos += (int32_t)len;
            if (sy + pos / s->w >= y0 + h)
                break;
        }
}

/* what was just played, as Melodee's HOME names it (ui_graph.c chord_of): a chord, else the notes; 0 = none */
static uint32_t dr_note_label(char *big, char *small)
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

/* the note label: a chip at the scene's top left (redrawn alone when it changes) */
static void dr_draw_note(void)
{
    char big[8], small[12];
    dr_scene_rows(DR_Y_SCENE, 40);
    if (dr_note_label(big, small)) {
        int32_t w = text_w(&AF_L, big) + (small[0] ? text_w(&AF_M, small) + 2 : 0) + 14;
        cv_rrect(6, 4, w, 32, 6, DR_SURF, DR_BG);
        w = cv_text_on(13, 2, &AF_L, big, DR_TEXT, DR_SURF);
        if (small[0])
            cv_text_on(w + 2, 13, &AF_M, small, DR_GOLD, DR_SURF);
    }
    cv_blit(0, DR_Y_SCENE);
}

static void dr_draw_head(void)
{
    const dr_char_t *c = &DR_CAST[dr.ch];
    char n[4] = {(char)('1' + dr.ch), '/', (char)('0' + DR_NCHAR), 0};
    int32_t w = text_w(&AF_S, c->tag);
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
    else
        cv_text_on(8, 1, &AF_S, drm.codex ? "CODEX" : DR_CAST[dr.ch].line, drm.codex ? DR_TEAL : DR_MID, DR_BG);
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
        bad += text_w(&AF_S, c->line) > 224;
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
            fmt_int(s, v);
            str_cpy(s + str_len(s), "%", 2);
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
    uint32_t k, c;
    for (k = 0; k < 27u; k++) {
        led_put(nl, 14u + k, (int)((fm1_in.notes >> k) & 1u));
        led_put(nd, 14u + k, 1);
    }
    for (k = 0; k < NELEM(USED); k++)
        led_put(nb, panel.btn[USED[k]], 1);
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
    song.octave = 0;
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
    uint32_t released, pressed, both, sig, i;
    int32_t s;
    char b[8];
    if (!drm.on)
        return 0;
    kb_mask = perf_mask = 0;                             /* every key plays, whatever is held */
    song.grid = 0;
    pressed = fm1_input_edges(&released);
    (void)fm1_input_note_edges();
    both = (1u << panel.btn[B_HOME]) | (1u << panel.btn[B_SAVE]);
    if ((fm1_in.buttons & both) == both) {               /* HOME + SAVE held: the countdown, then Melodee */
        uint32_t held = fm1_ms - drm.combo_t0;
        if (!drm.combo_t0)
            drm.combo_t0 = fm1_ms | 1u;
        else if (held >= DR_HOLD_MS) {
            drm.combo_t0 = 0;
            dr_leave();
            return 1;
        } else {
            char m[24] = "MELODEE IN ";
            m[11] = (char)('0' + (DR_HOLD_MS - held + 999u) / 1000u);
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
        if (pressed & (1u << panel.btn[bt])) {
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
            drm.dirty = DR_D_ALL;
        }
    }
    for (i = 0; i < 4u; i++)                             /* the four knobs, 2 % a detent */
        if ((s = fm1_enc_take(panel.enc[EN_K1 + i]) * panel.dir[EN_K1 + i]) != 0) {
            dr.mv[dr.ch][i] = (uint8_t)clamp(dr.mv[dr.ch][i] + s * 2, 0, 100);
            dr_macro_apply(&trk[0], dr.ch, i);
            drm.dirty |= DR_D_CARDS;
        }
    (void)fm1_enc_take(panel.enc[EN_SELECT]);            /* (not used yet: nothing piles up) */
    (void)fm1_enc_take(panel.enc[EN_ALGO]);
    if (!drm.codex && fm1_ms - drm.wave_ms >= 30u) {      /* the horizon: ~30 redraws a second while it moves */
        drm.wave_ms = fm1_ms;
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
    sig = 0;
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
