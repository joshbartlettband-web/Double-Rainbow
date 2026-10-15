/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
 * The design (the device clock driven by the audio the worklet asks for, the exports below, the saved
 * flash handed back before boot) follows X0X's browser build, web/emu/x0x_web.c of charlesvestal/fm1-x0x
 * (GPL-3.0), a Felucca fork. Ported from Felucca's web/emu/felucca_web.c to Melodee for Double Rainbow. */
/* Melodee in the browser: the firmware's sources, as the host tests build them (tests/hostsim.c: libc,
 * engines, voices, FX, sequencer; then the UI, storage and projects, main.c, in src/melodee.c's order),
 * compiled to WebAssembly and run in an AudioWorklet against a simulated FM-1:
 *   - the device clock: TIMER4 ticks (24 MHz) and fm1_ms advance with the audio the worklet asks for; the
 *     audio ISR (audio.c fm1_alnk0_irq, the real one) renders each I2S half (HALF_FRAMES) as the DMA would
 *     ask for it, and the main loop (main.c's, below in web_frame) runs every 15 ms, with ui_input on every
 *     millisecond in between as main.c's wait loop calls it;
 *   - the panel: buttons and encoders by their printed labels (panel.c's table maps them to the matrix),
 *     the keys as fm1_in.notes, the LEDs read back from fm1_led / fm1_led_dim through the key matrix;
 *   - the screen: lcd_fill / lcd_blit into a 240 x 240 RGB565 frame (the panel's byte order);
 *   - the flash: a 1 MiB NOR image behind storage.c's hooks (settings, projects, user presets); the page
 *     keeps its storage sectors and gives them back at boot.
 * Not here: USB (MIDI, audio, the editor, updates), TRS MIDI, the serial console, the battery (full), the
 * crash / UBOOT paths. Nothing of the firmware changes for this file.
 *   web/emu/build.sh  ->  build/emu/{melodee.wasm, worklet.js, index.html} */
#include <stdint.h>

#define MELODEE_WEB 1
#define MELODEE_FLASH 1
#define MELODEE_OTA 0
#define MELODEE_USB_AUDIO 0
#define MELODEE_CDC 0
#define MELODEE_UART 0
#ifndef MELODEE_VERSION
#define MELODEE_VERSION "WEB"
#endif

static uint8_t nor[0x100000];            /* the SPI NOR (erased: 0xFF) */
/* ------------------------------------------------------------ the clock --- */
#define FM1_TICKS_PER_US 24u             /* TIMER4: 24 MHz */
static uint32_t web_ticks;
/* MIDI in is stamped in TIMER4 ticks, as melodee.c has it: usb.c (through hostsim.c, below) would otherwise stamp
 * milliseconds, which the audio side compares with its ticks; past 2^31 ticks (89 s) every message then looked
 * 89 s early and MIDI in stopped */
#define MIDI_TIME_NOW() web_ticks
#define MIDI_TICKS_PER_MS (1000u * FM1_TICKS_PER_US)

#define main hostsim_main
#include "../../tests/hostsim.c"
#undef main
static uint32_t fm1_ticks(void) { return web_ticks; }

/* ------------------------------------------------------ audio (ALNK0) --- */
static uint32_t web_half;                /* the half the next render fills */
#define FM1_AUDIO_HALF 1u
static uint8_t fm1_audio_pending(void) { return FM1_AUDIO_HALF; }
static void fm1_audio_ack_aux(uint8_t p) { (void)p; }
static uint32_t fm1_audio_free_half(void) { return web_half; }
static void fm1_audio_ack_half(void) {}
static void fm1_audio_init(int32_t *buf, uint32_t words, void (*isr)(void), uint32_t prio)
{ (void)buf; (void)words; (void)isr; (void)prio; }
static void fm1_audio_stop(void) {}
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
void isr_alnk0(void) {}
#include "../../firmware/src/audio.c"

/* ------------------------------------------------------------ the panel --- */
#define FM1_NCOL 11u
static const int8_t FM1_KEYMAP[6][FM1_NCOL] = {      /* as hal/fm1_input.h: key id at (column, row bit) */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    { 5, 11,  4, 10,  3,  9,  2,  8, -1, -1, -1},
    {34, 35, 36, 37, 38, 40, 39, 13,  7,  6, 12},
    {23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33},
    { 0,  1, 15, 14, 17, 16, 19, 18, 20, 21, 22},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};
static uint8_t fm1_led[FM1_NCOL];
static uint8_t fm1_led_dim[2][FM1_NCOL];          /* hal/fm1_input.h: two dim planes, each with its level */
static uint8_t fm1_led_dim_mask[2];               /* 0 = full, 1 = 1/2, 3 = 1/4, 7 = 1/8 of the frames */
static uint32_t web_pressed, web_released, web_notes_pressed;
static int32_t web_enc_steps[7];
static uint32_t fm1_input_edges(uint32_t *released)
{
    uint32_t p = web_pressed;
    if (released)
        *released = web_released;
    web_pressed = web_released = 0;
    return p;
}
static uint32_t fm1_input_note_edges(void) { uint32_t n = web_notes_pressed; web_notes_pressed = 0; return n; }
static int32_t fm1_enc_take(uint32_t e) { int32_t s = web_enc_steps[e % 7u]; web_enc_steps[e % 7u] = 0; return s; }
static void fm1_input_init(void) {}
static void fm1_input_tick(void) {}
static void fm1_wdt_feed(void) {}

/* ----------------------------------------------------------- the screen --- */
static uint16_t web_fb[240 * 240];       /* RGB565 as the panel gets it (high byte first) */
static uint32_t web_draws;
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    uint32_t i, j;
    uint16_t sw = (uint16_t)((c >> 8) | (c << 8));
    for (j = 0; j < h && y + j < 240u; j++)
        for (i = 0; i < w && x + i < 240u; i++)
            web_fb[(y + j) * 240u + x + i] = sw;
    web_draws++;
}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{
    uint32_t i, j;
    for (j = 0; j < h && y + j < 240u; j++)
        for (i = 0; i < w && x + i < 240u; i++)
            web_fb[(y + j) * 240u + x + i] = p[j * w + i];
    web_draws++;
}
static void lcd_sync(void) {}
static void lcd_init(void) {}

#include "../../firmware/src/gfx.c"
#include "../../firmware/src/panel.c"
#include "../../firmware/src/ui.c"
#include "../../firmware/src/icons.c"
#include "../../firmware/src/ui_graph.c"
#include "../../firmware/src/ui_draw.c"
#include "../../firmware/src/ui_menu.c"
#include "../../firmware/src/ui_input.c"
#include "../../firmware/src/ui_layer.c"
#include "../../firmware/src/dr.c"

/* ------------------------------------------------------------ the flash --- */
static uint32_t web_flash_writes;
static uint8_t flash_ok;
static int st_read(uint32_t off, void *dst, uint32_t n)
{
    if (off >= sizeof nor || n > sizeof nor - off)
        return -1;
    memcpy(dst, nor + off, n);
    return 0;
}
static int st_erase(uint32_t off)
{
    if (off >= sizeof nor || 4096u > sizeof nor - off)
        return -8;
    memset(nor + (off & ~4095u), 0xFF, 4096);
    web_flash_writes++;
    return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)       /* NOR: a write only clears bits */
{
    const uint8_t *s = src;
    uint32_t i;
    if (off >= sizeof nor || n > sizeof nor - off)
        return -8;
    for (i = 0; i < n; i++)
        nor[off + i] &= s[i];
    web_flash_writes++;
    return 0;
}
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0x856014u; }             /* the FM-1's part */
static void fl_plain_window_init(void) {}
#define FL_FAR(fn) (fn)
#include "../../firmware/src/storage.c"
#include "../../firmware/src/upreset.c"
#include "../../firmware/src/project.c"
#include "../../firmware/src/cz_store.c"
#include "../../firmware/src/fm6_store.c"

/* ------------------------------------------------- main.c and its HAL --- */
typedef struct { uint32_t magic, count, vec, pc, emu, dbg, rets; } fm1_crash_t;
static struct { uint8_t p3_rst, wdt_con; uint32_t rst_src; } fm1_boot;
uint32_t _data_start[1], _data_end[1], _data_load[1], _bss_start[1], _bss_end[1];
uint32_t _dsp_start[1], _dsp_end[1], _dsp_load[1], _dt_start[1], _dt_end[1], _dt_load[1];
uint32_t _pool_start[1], _pool_end[1], _pd_start[1], _pd_end[1], _pd_load[1], _rt_start[1], _rt_end[1], _rt_load[1];
static uint32_t web_master_adc = 700;   /* MASTER, 0..1023 as the ADC reads it */
enum { FM1_ADC_BATT = 3, FM1_ADC_MASTER = 4 };
static int32_t fm1_adc_read(uint32_t ch) { return ch == FM1_ADC_MASTER ? (int32_t)web_master_adc : 620; }
static void fm1_adc_init(void) {}
static void fm1_timer5_ack(void) {}
static void fm1_timer5_start(void (*isr)(void), uint32_t prio) { (void)isr; (void)prio; }
void isr_timer5(void) {}
static void fm1_reboot(void) {}
static void fm1_enter_uboot(void) {}
static void fm1_guard_lock_top(void) {}
static void fm1_irq_enable_all(void) {}
static void fm1_irq_init(void) {}
static void fm1_time_init(void) {}
static void fm1_reset_reason(void) {}
static void fm1_wdt_arm(uint32_t v) { (void)v; }
static void fm1_mailbox_clear(void) {}
static void fm1_guard_enable(uint32_t m) { (void)m; }
enum { FM1_GUARD_STACK = 1, FM1_GUARD_WRITE = 2, FM1_GUARD_BUS = 4, FM1_GUARD_PC = 8 };
/* main.c's calls into the USB driver (registers): nothing here (usb.c's own functions stay as they are) */
#define usb_poll() ((void)0)
#define usb_start() ((void)0)
#define usb_retry(ms) ((void)(ms))
#define usb_detach() ((void)0)
#include "../../firmware/src/main.c"
#undef usb_poll
#undef usb_start
#undef usb_retry
#undef usb_detach

/* --------------------------------------------------------- the device --- */
static uint32_t web_booted, web_boot_ms, web_last_frame;

/* main.c fm1_main's boot, up to the main loop (no USB, no UART, no panel setup) */
static void web_power_on(void)
{
    persist_boot();
    settings_init();
    lcd_init();
    dr_splash();                                           /* main.c: the startup screen (dr.c) */
    memset(&melodee_dbg, 0, sizeof melodee_dbg);
    melodee_dbg.magic = DBG_MAGIC;
    fm1_input_init();
    fm1_adc_init();
    panel_init();
    melodee_init();
    audio_init();
    glo_restore();                                         /* main.c: CLK TUNE MIDI ROUT as last used, */
    project_boot();                                        /* then the BOOT project or the template */
    dr_boot();                                             /* main.c: Double Rainbow from power-on (dr.c) */
    web_boot_ms = fm1_ms + 430u;                           /* main.c: 30 + 400 ms before the first frame */
    web_booted = 1;
}

/* one pass of main.c fm1_main's loop (what the browser has: no USB, editor, UBOOT, console) */
static void web_frame(void)
{
    static int32_t knob = 512 * 16;
    int32_t b = fm1_adc_read(FM1_ADC_BATT), a = fm1_adc_read(FM1_ADC_MASTER);
    if (b > 0)
        song.batt_raw = song.batt_raw ? song.batt_raw + (b - song.batt_raw) / 32 : b;
    if (a >= 0) {                                          /* main.c: MASTER, smoothed, squared */
        uint32_t k10;
        knob += (a * 16 - knob) / 8;
        k10 = (uint32_t)(knob / 16);
        song.master_q12 = (k10 * k10) >> 8;
    }
    cz_service();
    fm6_service();
    melodee_dbg.ui_frames++;
    if (dr_frame())                                       /* main.c: Double Rainbow's screen while it is on */
        return;
    ui_input();
    glo_poll();
    settings_poll();
    ui_leds();
    ui_draw();
}

/* output ring, stereo float, filled by web_ms, drained by web_render */
#define RING 4096u
static float ring[RING * 2];
static uint32_t ring_w, ring_r, web_samples_due;

/* one millisecond of the device: the clock, the audio halves due by now, the main loop */
static void web_ms(void)
{
    fm1_ms++;
    web_ticks += 1000u * FM1_TICKS_PER_US;
    web_samples_due += FS;
    while (web_samples_due >= 1000u * HALF_FRAMES) {      /* an I2S half: the audio ISR renders it */
        uint32_t k;
        const int32_t *o = &abuf[web_half * HALF_WORDS];
        web_samples_due -= 1000u * HALF_FRAMES;
        fm1_alnk0_irq();
        for (k = 0; k < HALF_FRAMES; k++) {
            uint32_t i = (ring_w++ % RING) * 2u;
            ring[i] = (float)o[2u * k] / 8388608.0f;
            ring[i + 1] = (float)o[2u * k + 1u] / 8388608.0f;
        }
        web_half ^= 1u;
    }
    if (!web_booted || (int32_t)(fm1_ms - web_boot_ms) < 0)
        return;
    if (web_boot_ms) {                                    /* main.c: the splash goes, the loop starts */
        web_boot_ms = 0;
        lcd_fill(0, 0, 240, 240, T_BG);
        web_last_frame = fm1_ms - 15u;
    }
    if (fm1_ms - web_last_frame >= 15u) {                /* main.c: a frame, then ui_input until 15 ms are over */
        web_last_frame = fm1_ms;
        web_frame();
    } else if (!drm.on) {
        ui_input();                                       /* main.c's wait loop */
    }
}

#define EXPORT __attribute__((used, visibility("default")))
#undef __attribute__

static float out_l[1024], out_r[1024];
EXPORT float *web_out_l(void) { return out_l; }
EXPORT float *web_out_r(void) { return out_r; }
/* n frames (<= 1024) into out_l / out_r: the device runs until they exist */
EXPORT void web_render(uint32_t n)
{
    uint32_t k;
    if (n > 1024u)
        n = 1024u;
    while (ring_w - ring_r < n)
        web_ms();
    for (k = 0; k < n; k++) {
        uint32_t i = (ring_r++ % RING) * 2u;
        out_l[k] = ring[i];
        out_r[k] = ring[i + 1];
    }
}

/* the flash: erased, then the page's saved sectors (web_nor), then boot */
EXPORT uint8_t *web_nor(void) { return nor; }
EXPORT uint32_t web_nor_size(void) { return sizeof nor; }
EXPORT void web_nor_erase(void) { memset(nor, 0xFF, sizeof nor); }
EXPORT uint32_t web_flash_writes_count(void) { return web_flash_writes; }
EXPORT void web_boot(void)
{
    if (!web_booted)
        web_power_on();
}

/* input, by the printed labels: buttons bit b = B_FX .. B_OCTUP (panel.c), keys bit k = key k (F3 .. G5),
 * encoders by role (EN_SELECT .. EN_K4), MASTER 0..1023 */
EXPORT void web_buttons(uint32_t labels)
{
    uint32_t m = 0, b, old = fm1_in.buttons;
    for (b = 0; b < NB; b++)
        if ((labels >> b) & 1u)
            m |= 1u << panel.btn[b];
    web_pressed |= m & ~old;
    web_released |= old & ~m;
    fm1_in.buttons = m;
}
EXPORT void web_keys(uint32_t mask)
{
    uint32_t old = fm1_in.notes;
    mask &= (1u << 27) - 1u;
    web_notes_pressed |= mask & ~old;
    fm1_in.notes = mask;
}
EXPORT void web_enc(uint32_t role, int32_t n)
{
    if (role < NE)
        web_enc_steps[panel.enc[role] % 7u] += n * panel.dir[role];
}
EXPORT void web_master(uint32_t v) { web_master_adc = v > 1023u ? 1023u : v; }
/* a channel message from Web MIDI, as a USB-MIDI packet arrives (usb.c midi_enqueue): 0 = the ring was full */
EXPORT uint32_t web_midi(uint32_t status, uint32_t d1, uint32_t d2)
{
    if (status < 0x80u || status >= 0xF0u)
        return 1;
    return (uint32_t)midi_enqueue((status >> 4) | status << 8 | (d1 & 127u) << 16 | (d2 & 127u) << 24, 1);
}

/* the LEDs: bit b = the button labelled b lit, keys bit k */
static uint32_t led_on(const uint8_t *l, uint32_t id)
{
    uint32_t c, r;
    for (c = 0; c < FM1_NCOL; c++)
        for (r = 1; r < 5u; r++)
            if (FM1_KEYMAP[r][c] == (int8_t)id)
                return (l[c] >> r) & 1u;
    return 0;
}
static uint32_t led_buttons(const uint8_t *l)
{
    uint32_t b, m = 0;
    for (b = 0; b < NB; b++)
        m |= led_on(l, panel.btn[b]) << b;
    return m;
}
static uint32_t led_keys(const uint8_t *l)
{
    uint32_t k, m = 0;
    for (k = 0; k < 27u; k++)
        m |= led_on(l, 14u + k) << k;
    return m;
}
EXPORT uint32_t web_lit_buttons(void) { return led_buttons(fm1_led); }
EXPORT uint32_t web_lit_keys(void) { return led_keys(fm1_led); }
/* the two dim planes (hal/fm1_input.h): each LED in either, and each plane's level (its mask: 0 full .. 7 1/8) */
EXPORT uint32_t web_dim_buttons(uint32_t plane) { return led_buttons(fm1_led_dim[plane & 1u]); }
EXPORT uint32_t web_dim_keys(uint32_t plane) { return led_keys(fm1_led_dim[plane & 1u]); }
EXPORT uint32_t web_dim_mask(uint32_t plane) { return fm1_led_dim_mask[plane & 1u]; }

/* for the test and the bench (web/emu/emu_test.mjs), a heavy song straight into the tracks (as hostsim's
 * renders set them): T1 PROPHET, T2 FM6, T3 CZ-1, 3-note chords on all 16 steps, held; T4 DRUM, kick snare
 * and hats. PLAY is the page's (or the test's) to press */
EXPORT void web_test_heavy(void)
{
    static const uint8_t ENG[3] = {ENGI_PROPHET, ENGI_FM6, ENGI_CZ};
    static const uint8_t CH[4][3] = {{48, 55, 64}, {45, 52, 60}, {41, 48, 57}, {43, 50, 59}};
    uint32_t p, i;
    for (p = 0; p < 3u; p++) {
        track_t *t = &trk[p];
        host_preset(t, ENG[p], 1);
        t->p[P_VOICE] = V_POLY;
        t->p[P_SUS] = 127;
        for (i = 0; i < 16u; i++) {
            uint8_t n[3], k;
            for (k = 0; k < 3u; k++)
                n[k] = (uint8_t)(CH[i / 4u][k] + 12u * p);
            put_step(t, i, 3, n, ST_NOTE, 0);
        }
    }
    host_808_kit(&trk[3]);
    for (i = 0; i < 16u; i++) {
        uint8_t n[4];
        uint32_t k = 0;
        if (i % 4u == 0u)
            n[k++] = 36;
        if (i == 4u || i == 12u)
            n[k++] = 38;
        n[k++] = i % 2u ? 42 : 46;
        put_step(&trk[3], i, k, n, ST_NOTE, 0);
    }
    ui.force = 1;
}

EXPORT uint16_t *web_screen(void) { return web_fb; }
EXPORT uint32_t web_screen_draws(void) { return web_draws; }
EXPORT uint32_t web_now_ms(void) { return fm1_ms; }
EXPORT uint32_t web_playing(void) { return song.playing; }
EXPORT uint32_t web_master_q12(void) { return song.master_q12; }
EXPORT uint32_t web_voices(uint32_t t)                  /* a track's sounding voices */
{
    uint32_t i, n = 0;
    for (i = 0; i < NVOICE; i++)
        n += trk[t % NTRK].v[i].active && trk[t % NTRK].v[i].stage != 4u;
    return n;
}
EXPORT uint32_t web_engine(uint32_t t) { return trk[t % NTRK].engine; }
EXPORT uint32_t web_test_user_preset(uint32_t i) { return (uint32_t)up_used(i); }
/* Double Rainbow (dr.c; web/emu/dr_test.mjs) */
EXPORT uint32_t web_dr_on(void) { return drm.on; }
EXPORT uint32_t web_dr_char(void) { return dr.ch; }
EXPORT uint32_t web_dr_knob(uint32_t m) { return dr.mv[dr.ch % DR_NCHAR][m & 3u]; }
EXPORT uint32_t web_dr_view(void) { return drm.codex | drm.page << 1 | drm.under << 3; }   /* codex, page (2 bits), EDIT */
EXPORT uint32_t web_dr_text_check(void) { return dr_text_check(); }
EXPORT int32_t web_octave(void) { return song.octave; }
EXPORT uint32_t web_dr_kmode(void) { return dr_kmode; }              /* SOLO WEAVE CAST SCRY TRACE (seq.c) */
EXPORT uint32_t web_dr_chord(void) { return dr_chord; }
EXPORT uint32_t web_dr_key(void) { return dr_key; }
/* SCRY: 0 the drill, 1 its pool, 2 the streak, 3 the best, 4 the misses, 5 the hint key (255 none, 254 the chord's),
 * 6 asking, 7 8 the question's notes (CHORDS: 8 the chord), 9 shown (1 right, 2 again), 10 the black keys' chords on */
EXPORT uint32_t web_dr_scry(uint32_t f)
{
    const uint32_t v[11] = {drs.drill, drs.pool[drs.drill % 3u], drs.streak, drs.best, drs.misses, drs.hint, drs.asked,
                            drs.a, drs.b, drs.reveal, dr_castkeys};
    return f < 11u ? v[f] : 0;
}
/* TRACE: 0 the song, 1 the place in it, 2 the end, 3 the demonstration, 4 the key lit (-1 none), 5 the demo's place,
 * 6 the song's length (notes and rests) */
EXPORT int32_t web_dr_trace(uint32_t f)
{
    const int32_t v[7] = {drt.song, drt.pos, drt.done, drt.demo, dr_trace_key(), drt.dpos, DR_SONGS[drt.song % DR_NSONGS].n};
    return f < 7u ? v[f] : 0;
}
EXPORT uint32_t web_kb_note(uint32_t k) { return kb_note[k % 27u]; }    /* what key k is playing (seq.c) */
EXPORT uint32_t web_p5_field(uint32_t id) { return p5_patch_of(&trk[0])->raw[id % P5_RAW_MAX]; }
EXPORT int32_t web_param(uint32_t t, uint32_t id) { return trk[t % NTRK].p[id % P_COUNT]; }
EXPORT int32_t web_bpm(void) { return song.g[G_BPM]; }
EXPORT uint32_t web_step_note(uint32_t t, uint32_t i) { return trk[t % NTRK].step[i % NSTEP].note[0]; }
EXPORT uint32_t web_p_e0(void) { return P_E0; }                      /* (core.h: the engine's eight) */
EXPORT uint32_t web_sample_rate(void) { return FS; }
