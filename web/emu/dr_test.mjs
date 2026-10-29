// SPDX-License-Identifier: GPL-3.0-only
// Double Rainbow's checks (firmware/src/dr.c) on the browser build, driven like a player: the panel, the keys, the
// knobs; what the screen, the LEDs and the audio do.
//   node web/emu/dr_test.mjs build/emu/melodee.wasm
// Checks: it starts in Double Rainbow on Vesper; every character sounds, on its own engine, and they are about as
// loud as each other (the drums by their own measure); a knob of each new character moves its setting and its sound; PRESETS goes round the cast; a knob moves its value, the real setting under it and
// the screen; the horizon traces the wave while a note sounds and lies flat again after; EDIT held and the codex
// (three pages) change the screen; every text fits its room; the beats (PLAY, SEQ, the tempo, the bass after the
// chord, quiet in SCRY); OCT shifts the keys; HOME +
// SAVE let go early stays, held 3 s goes to the full Melodee with track 1's own sound back, and the keys still play.
// The spellbook modes on ALGORITHM: WEAVE (a black key picks the chord, the white keys strum its notes from C3 up),
// CAST (a black key holds its chord, four notes for a seventh, under the tune), SCRY (the ear game: a question heard,
// answered on the keys, the pool growing, the hint after two misses, OCT the drill, PLAY again) and TRACE (a song
// followed key by key to its end and round again, OCT and SELECT the song, PLAY the demonstration).
import fs from "fs";

const bytes = fs.readFileSync(process.argv[2] || "build/emu/melodee.wasm");
const B = { FX: 0, SCL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11, OCTDN: 12, OCTUP: 13 };
const EN = { SELECT: 0, ALGO: 1, PRESETS: 2, K1: 3, K2: 4, K3: 5, K4: 6 };
const NAMES = ["VESPER", "SHARD", "SQUALL", "WICK", "TOCK", "MOTE", "MURK", "BOG", "HUSH", "TUMBLER", "GLINT", "HERMIT",
               "BURROW", "PIP", "ECHO", "FORK", "ANTS", "RABBIT", "TOP", "CAIRN", "STRAY", "TUPPENCE", "GLOAM", "TINKER",
               "DOZE", "KITE", "BRAMBLE", "HOLLOW", "FLITTER", "MOSSBACK"];
const ENGINE = [19, 12, 11, 5, 10, 3, 2, 0, 0, 12, 15, 19, 0, 12, 5, 12, 0, 12, 2, 11, 12, 0, 12, 12, 3, 5, 10, 19, 15, 2];   // PROPHET FM6 NOISE VOICE DRUM LOFI PHASE ANALOG x2 FM6 CZ-1 PROPHET ANALOG FM6 VOICE
const N = NAMES.length, TOCK = 4, GLINT = 10, PIP = 13, ECHO = 14;
const P5_CUTOFF = 17;                              // (prophet_patch.c)
let fails = 0;
const check = (what, ok) => { console.log(`dr: ${what.padEnd(78)} ${ok ? "ok" : "FAIL"}`); if (!ok) fails++; };

const { instance } = await WebAssembly.instantiate(bytes, {});
const ex = instance.exports, mem = ex.memory;
ex._initialize();
ex.web_nor_erase();
ex.web_master(1023);
ex.web_boot();
let sumSq = 0, count = 0, peak = 0, diffSq = 0, prev = 0;
const render = (ms) => {
  const n = Math.round(ms * 44.1);
  for (let k = 0; k < n; k += 128) {
    const m = Math.min(128, n - k);
    ex.web_render(m);
    const l = new Float32Array(mem.buffer, ex.web_out_l(), m), r = new Float32Array(mem.buffer, ex.web_out_r(), m);
    for (let i = 0; i < m; i++) {
      sumSq += l[i] * l[i] + r[i] * r[i]; count += 2; peak = Math.max(peak, Math.abs(l[i]));
      diffSq += (l[i] - prev) ** 2; prev = l[i];      // (the first difference: the high frequencies' share)
    }
  }
};
const level = (fn) => {
  sumSq = 0; count = 0; peak = 0; diffSq = 0; fn();
  return { db: 10 * Math.log10(sumSq / count + 1e-20), peak, bright: 10 * Math.log10(diffSq / (sumSq / 2) + 1e-20) };
};
const tap = (b) => { ex.web_buttons(1 << b); render(80); ex.web_buttons(0); render(120); };
const turn = (role, n) => { for (let i = 0; i < Math.abs(n); i++) { ex.web_enc(role, Math.sign(n)); render(20); } render(100); };
const screen = () => new Uint16Array(mem.buffer, ex.web_screen(), 240 * 240).slice();
const rows = (fb, y0, y1) => { let h = 0; for (let i = y0 * 240; i < y1 * 240; i++) h = (h * 31 + fb[i]) >>> 0; return h; };
const phrase = () => {                             // a chord, then a tune, as the sound checks render it
  ex.web_keys(1 << 7 | 1 << 11 | 1 << 14); render(1200); ex.web_keys(0); render(300);
  for (const k of [19, 18, 14, 11, 16, 14, 12, 7]) { ex.web_keys(1 << k); render(260); ex.web_keys(0); render(50); }
  render(400);
};

render(1500);
check("power-on: Double Rainbow, Vesper on the Prophet", ex.web_dr_on() === 1 && ex.web_dr_char() === 0 && ex.web_engine(0) === ENGINE[0]);
check(`every text fits its room (${ex.web_dr_text_check()} do not)`, ex.web_dr_text_check() === 0);
ex.web_keys(1 << 7); render(100);
check("the LEDs: every key glows, a held key lit, SCL EDIT OCT HOME SAVE glow", ex.web_lit_keys() === 1 << 7 &&
      ex.web_dim_keys(0) === (1 << 27) - 1 && (ex.web_dim_buttons(1) & (1 << B.SCL | 1 << B.EDIT | 1 << B.OCTUP)) ===
      (1 << B.SCL | 1 << B.EDIT | 1 << B.OCTUP));
ex.web_keys(0); render(1500);

const loud = [];
const P_E0 = ex.web_p_e0(), P_REV = ex.web_p_rev();                        // the new four: a knob each, its setting and its sound
const E = (e) => () => ex.web_param(0, P_E0 + e), F = (f) => () => ex.web_p5_field(f);
const KNOB = {                                     // [knob, the setting under it, what it does, the check]
  3: [0, E(0), "VOWEL (AH to OO): darker", (lo, hi) => hi.bright - lo.bright < -3],
  4: [1, E(3), "BOOM (decay): longer, louder", (lo, hi) => hi.db - lo.db > 3],
  5: [2, E(4), "ZAP (pitch sweep): darker", (lo, hi) => hi.bright - lo.bright < -3],
  6: [2, E(0), "TIDE (the eight waves): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  7: [0, E(4), "SQUISH (cutoff): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  8: [0, E(4), "DUSK (cutoff): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  9: [0, E(2), "BARK (modulator level): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  11: [0, F(P5_CUTOFF), "BLARE (Sequential's Iconic Brass, its cutoff): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  12: [0, E(4), "DEEP (the cutoff, closing): darker", (lo, hi) => hi.bright - lo.bright < -3],
  13: [0, E(2), "KNOCK (modulator level): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  14: [1, E(5), "WHISPER (breath for buzz): airier", (lo, hi) => hi.bright - lo.bright > 3],
  15: [0, E(2), "PURITY (modulator level, turned down): purer", (lo, hi) => hi.bright - lo.bright < -3],
  16: [3, E(4), "HUM (cutoff): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  17: [1, E(2), "SHEEN (modulator level): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  18: [2, E(2), "WHIRR (DCW): another tone", (lo, hi) => Math.abs(hi.bright - lo.bright) > 3],
  19: [0, E(2), "CLACK (the stones' pitch): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  20: [0, E(2), "HEROIC (modulator level): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  21: [3, E(4), "BODY (cutoff): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  22: [1, E(2), "TOLL (modulator level): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  23: [0, E(2), "CLICK (modulator level): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  24: [2, E(7), "TONE: brighter", (lo, hi) => hi.bright - lo.bright > 3],
  25: [2, E(0), "VOWEL (AH to OO): darker", (lo, hi) => hi.bright - lo.bright < -3],
  26: [2, E(4), "SPIKE (snap and tone): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  27: [2, F(P5_CUTOFF), "TONE (the cutoff): brighter", (lo, hi) => hi.bright - lo.bright > 3],
  28: [3, () => ex.web_param(0, P_REV), "CAVE (reverb): a longer room (the tail after a note)", (lo, hi) => hi.tail - lo.tail > 3],
  29: [0, E(2), "MOSS (DCW): brighter", (lo, hi) => hi.bright - lo.bright > 3],
};
for (let c = 0; c < N; c++) {
  if (c) turn(EN.PRESETS, 1);
  render(300);
  const l = level(phrase);
  loud.push(l.db);
  check(`${NAMES[c]} (PRESETS ${c ? "turned" : "at power-on"}): engine ${ex.web_engine(0)}, sounds (peak ${l.peak.toFixed(3)})`,
        ex.web_dr_char() === c && ex.web_engine(0) === ENGINE[c] && l.peak > 0.02);
  if (c === GLINT) {                               // TONE: a Casio tone a detent, the card says which
    const cards = rows(screen(), 198, 240), a = level(phrase);
    turn(EN.K1, 4);
    const t = ex.web_dr_knob(0), b = level(phrase), cards5 = rows(screen(), 198, 240);
    turn(EN.K1, -60);
    check(`GLINT: TONE steps through Casio's own tones (A-1 -> A-${t}: bright ${a.bright.toFixed(1)} -> ${b.bright.toFixed(1)}), and stops at A-1`,
          t === 5 && Math.abs(b.bright - a.bright) + Math.abs(b.db - a.db) > 1 && cards5 !== cards &&
          rows(screen(), 198, 240) === cards && ex.web_dr_knob(0) === 1 && ex.web_engine(0) === 15);
  }
  if (c === TOCK) {                                // the chip names the drum struck: the kick, then the snare
    const chip = () => rows(screen(), 38, 78);
    ex.web_keys(1 << 7); render(150); ex.web_keys(0); render(200); const kick = chip();
    ex.web_keys(1 << 9); render(150); ex.web_keys(0); render(200); const snare = chip();
    check("TOCK: the chip names the drum struck (C the kick, D the snare)", kick !== snare);
  }
  if (c === 19) {                                  // CAIRN: his core, a kalimba on track 3 under the stones
    const v0 = ex.web_dr_knob(3), eng = ex.web_engine(2);
    turn(EN.K1 + 3, -60); const lo = level(phrase);
    turn(EN.K1 + 3, 60); const hi = level(phrase);
    turn(EN.K1 + 3, -60); turn(EN.K1 + 3, v0 / 2);
    check(`CAIRN: track 3 his core (engine ${eng}: FM6 KALIMBA), CORE louder (${lo.db.toFixed(1)} -> ${hi.db.toFixed(1)} dB)`,
          eng === 12 && hi.db - lo.db > 2 && ex.web_dr_knob(3) === v0);
  }
  if (c === 20)
    check(`STRAY: no core, track 3 Melodee's own again (engine ${ex.web_engine(2)})`, ex.web_engine(2) === 3);
  if (KNOB[c]) {
    const [m, get, what, ok] = KNOB[c], v0 = ex.web_dr_knob(m);
    const tail = () => level(() => { ex.web_keys(1 << 12); render(300); ex.web_keys(0); render(100); }) &&
                       level(() => render(800)).db;   // (the 800 ms after the note)
    turn(EN.K1 + m, -60); const p0 = get(); const lo = level(phrase); lo.tail = tail();
    turn(EN.K1 + m, 60); const p1 = get(); const hi = level(phrase); hi.tail = tail();
    turn(EN.K1 + m, -60); turn(EN.K1 + m, v0 / 2);
    check(`${NAMES[c]}: ${what} (${p0} -> ${p1}; ${lo.db.toFixed(1)} -> ${hi.db.toFixed(1)} dB, bright ${lo.bright.toFixed(1)} -> ${hi.bright.toFixed(1)})`,
          p1 !== p0 && ok(lo, hi) && ex.web_dr_knob(m) === v0);
  }
}
const CAIRN = 19, TINKER = 23, BRAMBLE = 26;
const tonal = loud.filter((_, i) => i !== TOCK && i !== PIP && i !== CAIRN && i !== TINKER && i !== BRAMBLE), spread = Math.max(...tonal) - Math.min(...tonal);
const mean = tonal.reduce((p, x) => p + x, 0) / tonal.length;   // (struck sounds die away: by their own measure)
check(`the cast are about as loud as each other (RMS ${loud.map((x) => x.toFixed(1)).join(" ")} dB: ${spread.toFixed(1)} dB apart; the drums, mallets and stones within 6)`,
      spread < 3 && Math.abs(loud[TOCK] - mean) < 6 && Math.abs(loud[PIP] - mean) < 6 && Math.abs(loud[CAIRN] - mean) < 6 && Math.abs(loud[TINKER] - mean) < 6 &&
      Math.abs(loud[BRAMBLE] - mean) < 6);
{ // ECHO: a short note comes back, softer, an eighth later (twice at her knob's start); ECHO down, it does not
  const tail = () => level(() => { ex.web_keys(1 << 12); render(120); ex.web_keys(0); render(160); }) &&
                     level(() => render(700)).db;   // (the 700 ms after the note itself is over)
  turn(EN.PRESETS, ECHO - (N - 1)); render(800);          // (the last one is not Echo now: go to her)
  const withEcho = tail(); render(1500);
  turn(EN.K3, -30); const without = tail(); turn(EN.K3, 20); render(1500);
  check(`ECHO: her notes come back (${withEcho.toFixed(1)} dB after the note), not with ECHO at 0 (${without.toFixed(1)} dB)`,
        withEcho - without > 10 && ex.web_dr_knob(2) === 40);
  turn(EN.PRESETS, (N - 1) - ECHO); render(800);
}
{ // Shard's light: each note sends a band of spectrum across his chest, then he is plain glass again
  turn(EN.PRESETS, 1 - (N - 1)); render(800);
  const ok = [];
  for (const wait of [300, 301]) {                // (an even and an odd millisecond: the start time is fm1_ms | 1)
    render(wait);
    const rest = rows(screen(), 108, 150);
    ex.web_keys(1 << 9); render(240);
    const mid = rows(screen(), 108, 150);
    ex.web_keys(0); render(900);
    ok.push(mid !== rest && rows(screen(), 108, 150) === rest);
  }
  check("SHARD: a note sends his light across his chest, and it passes", ex.web_dr_char() === 1 && ok.every(Boolean));
  turn(EN.PRESETS, N - 2);
}
turn(EN.PRESETS, 1);
check(`PRESETS goes round: after ${NAMES[N - 1]}, VESPER again`, ex.web_dr_char() === 0 && ex.web_engine(0) === ENGINE[0]);
turn(EN.PRESETS, -1);
check(`and back: ${NAMES[N - 1]}`, ex.web_dr_char() === N - 1);
turn(EN.PRESETS, 1);

{ // a knob: its value, the setting under it, the screen
  const v0 = ex.web_dr_knob(0), cut0 = ex.web_p5_field(P5_CUTOFF), d0 = ex.web_screen_draws(), cards0 = rows(screen(), 198, 240);
  turn(EN.K1, 5);
  check(`KNOB 1 (GLOW) 5 detents: ${v0} -> ${ex.web_dr_knob(0)} %, CUTOFF ${cut0} -> ${ex.web_p5_field(P5_CUTOFF)}, the card redrawn`,
        ex.web_dr_knob(0) === v0 + 10 && ex.web_p5_field(P5_CUTOFF) > cut0 && ex.web_screen_draws() > d0 && rows(screen(), 198, 240) !== cards0);
  turn(EN.K1, -60);
  check(`turned far down it stops at 0 (${ex.web_dr_knob(0)} %)`, ex.web_dr_knob(0) === 0);
  turn(EN.K1, 5 + v0 / 2 - 5);
  const bright = level(() => { ex.web_keys(1 << 7); render(800); ex.web_keys(0); render(200); });
  turn(EN.K1, 20);
  const brighter = level(() => { ex.web_keys(1 << 7); render(800); ex.web_keys(0); render(200); });
  check(`GLOW up 40 %: the same note brighter (high-frequency share ${bright.bright.toFixed(1)} -> ${brighter.bright.toFixed(1)} dB)`,
        brighter.bright - bright.bright > 2);
}

{ // the animations: Vesper's eyes (rows 88..96) brighten with the sound; Squall rises and falls on his own
  render(3000);
  const eyes = () => { let m = 0; const fb = screen(); for (let y = 88; y < 97; y++) for (let x = 0; x < 240; x++) { const v = fb[y * 240 + x], s = (v >> 8 | v << 8) & 0xffff; m += ((s >> 11) & 31) + ((s >> 5) & 63) / 2 + (s & 31); } return m / 100; };   // (her eyes' rows, their brightness)
  let quiet = 0; for (let i = 0; i < 6; i++) { render(150); quiet = Math.max(quiet, eyes()); }
  ex.web_keys(1 << 7 | 1 << 11 | 1 << 14); render(500);
  const loudEyes = eyes(); ex.web_keys(0); render(3000);
  check(`VESPER: her eyes glow brighter while she sounds (${quiet.toFixed(1)} -> ${loudEyes.toFixed(1)})`, loudEyes > quiet + 1);
  const amber = () => { let n = 0; const fb = screen(); for (let y = 88; y < 97; y++) for (let x = 90; x < 150; x++) { const v = fb[y * 240 + x], s = (v >> 8 | v << 8) & 0xffff; n += ((s >> 11) & 31) > 24 && ((s >> 5) & 63) > 36 && (s & 31) < 16; } return n; };
  let open = 0, shut = 99; for (let i = 0; i < 300; i++) { render(30); const n = amber(); open = Math.max(open, n); shut = Math.min(shut, n); }
  check(`VESPER blinks: her amber eyes (${open} px) are gone for a moment (${shut} px) every few seconds`, open > 10 && shut === 0);
  const top = () => { let x = 0; const fb = screen(); for (let y = 60; y < 72; y++) for (let c = 100; c < 150; c++) x = (x * 31 + fb[y * 240 + c]) >>> 0; return x; };   // (above her head, right of the chip)
  render(500); const head = top(); ex.web_keys(1 << 7); render(60); const pressed = top(); render(400); ex.web_keys(0); render(1500);
  check("VESPER squashes on a note (her head dips) and springs back", pressed !== head && top() === head);
  turn(EN.PRESETS, 2); render(800);
  const seen = new Set(); for (let i = 0; i < 12; i++) { render(250); seen.add(rows(screen(), 60, 160)); }
  check(`SQUALL floats: he rises and falls with no note (${seen.size} different frames in 3 s)`, seen.size >= 3);
  turn(EN.PRESETS, 6); render(1500);                      // HUSH: her locket shows the Line, flat when quiet
  const locket = () => rows(screen(), 120, 140);
  const still = locket(); ex.web_keys(1 << 7); render(500); const live = locket(); render(300); const live2 = locket();
  ex.web_keys(0); render(3000);
  check("HUSH: her locket shows the Line (it moves while she sounds, lies flat again after)",
        live !== still && locket() === still);
  turn(EN.PRESETS, -8); render(800);
}

{ // the horizon: the live wave while a note sounds, flat (as it was) when it is over
  render(4000);
  const rest = rows(screen(), 154, 198);
  ex.web_keys(1 << 7);
  render(600);
  const a = rows(screen(), 154, 198);
  render(97);
  const b = rows(screen(), 154, 198);
  ex.web_keys(0);
  render(6000);
  check("the horizon traces the wave while a note sounds, and lies flat again after", a !== rest && b !== a &&
        rows(screen(), 154, 198) === rest);
  ex.web_master(300); render(500);                 // a quiet, ear-friendly volume (MASTER under a third)
  ex.web_keys(1 << 7); render(600);
  const quiet = rows(screen(), 154, 198);
  ex.web_keys(0); render(6000);
  ex.web_master(1023); render(500);
  check("at a low volume the horizon still traces the wave", quiet !== rest && rows(screen(), 154, 198) === rest);
}

{ // EDIT held, the codex
  const cards = rows(screen(), 198, 240);
  ex.web_buttons(1 << B.EDIT); render(200);
  const held = rows(screen(), 198, 240), heldView = ex.web_dr_view();
  ex.web_buttons(0); render(200);
  check("EDIT held: the cards show what the knobs move; let go: back", held !== cards && (heldView & 8) && rows(screen(), 198, 240) === cards);
  const still = () => rows(screen(), 38, 86) + rows(screen(), 100, 240);   // (not her eyes: they breathe)
  const scene = still();
  tap(B.SCL);
  const page0 = rows(screen(), 38, 240);
  turn(EN.PRESETS, 1);
  const page1 = rows(screen(), 38, 240), v1 = ex.web_dr_view();
  turn(EN.PRESETS, 1);
  const page2 = rows(screen(), 38, 240);
  turn(EN.PRESETS, 3);
  const v2 = ex.web_dr_view();
  turn(EN.PRESETS, -5);
  const v0 = ex.web_dr_view();
  tap(B.SCL);
  check("SCL: the codex (the ledger, the engine, something to try: PRESETS, stopping at both ends), SCL: back",
        page0 !== scene && page1 !== page0 && page2 !== page1 && (v1 & 7) === 3 && (v2 & 7) === 5 && (v0 & 7) === 1 &&
        ex.web_dr_view() === 0 && still() === scene && ex.web_dr_char() === 0);
  check("the keys still play in the codex and with EDIT held", (() => {
    tap(B.SCL); const a = level(() => { ex.web_keys(1 << 9); render(400); ex.web_keys(0); render(100); }); tap(B.SCL);
    ex.web_buttons(1 << B.EDIT); const b = level(() => { ex.web_keys(1 << 9); render(400); ex.web_keys(0); render(100); });
    ex.web_buttons(0); render(100);
    return a.peak > 0.02 && b.peak > 0.02;
  })());
}

// SCRY: S(field) as web_dr_scry names them
function scry() {
  const S = (f) => ex.web_dr_scry(f);
  const DEG = [-1, 3, -1, 4, -1, -1, 0, -1, 1, -1, 2, -1];
  const chordKey = (id) => { for (let k = 0; k < 27; k++) { const d = DEG[(53 + k) % 12]; if (d >= 0 && (k >= 24 ? 10 : (k >= 12 ? 5 : 0) + d) === id) return k; } return -1; };
  const press = (k) => { ex.web_keys(1 << k); render(150); ex.web_keys(0); render(60); };
  const answer = () => S(0) === 2 ? chordKey(S(8)) : S(8) - 53;
  turn(EN.ALGO, 2);                                // (from WEAVE)
  const asked = level(() => render(2000));
  check(`ALGORITHM to SCRY: NOTES, a question heard (peak ${asked.peak.toFixed(3)}), its home note lit`,
        ex.web_dr_kmode() === 3 && S(0) === 0 && S(1) === 2 && S(6) === 1 && asked.peak > 0.02 &&
        ((ex.web_lit_keys() >> (S(7) - 53)) & 1) === 1 && S(7) % 12 === ex.web_dr_key());
  const chip = rows(screen(), 38, 78);
  press(answer());
  check(`the right key: YES, a streak of 1, the chip shows it`, S(9) === 1 && S(2) === 1 && S(6) === 0 && rows(screen(), 38, 78) !== chip);
  for (let i = 0; i < 2; i++) { render(1600); press(answer()); }
  check(`three right in a row: the pool grows (${S(1)} notes) streak ${S(2)}`, S(1) === 3 && S(2) === 3 && S(3) === 3);
  render(1600);
  const wrong = (S(8) - 53 + 1) % 27;
  press(wrong);
  render(900);
  press(wrong);
  let blink = 0;
  for (let i = 0; i < 12; i++) { render(40); blink |= ex.web_lit_keys(); }
  check(`two misses: the streak gone, asked again, the right key blinks (hint ${S(5)})`,
        S(2) === 0 && S(3) === 3 && S(4) === 2 && S(5) === S(8) - 53 && ((blink >> S(5)) & 1) === 1);
  render(900);
  const again = S(8);
  const replay = level(() => { tap(B.PLAY); render(1500); });
  check(`PLAY: the same question again (peak ${replay.peak.toFixed(3)})`, S(8) === again && S(6) === 1 && replay.peak > 0.02);
  press(answer());
  check("answered after the hint: the streak starts again", S(2) === 1 && S(5) === 255);
  tap(B.OCTUP);
  render(1600);
  const o = S(8) - 53 >= 12 ? -12 : 12;            // (another octave counts)
  check(`OCT+: INTERVALS (${S(7)} -> ${S(8)}), the keys not shifted`, S(0) === 1 && S(6) === 1 && ex.web_octave() === 0 && S(8) - S(7) <= 12);
  press(S(8) - 53 + (S(8) - 53 + o >= 0 && S(8) - 53 + o < 27 ? o : 0));
  check("INTERVALS: the second note, in any octave", S(9) === 1 && S(2) === 2);
  tap(B.OCTUP);
  render(1600);
  check(`OCT+ again: CHORDS (chord ${S(8)}), the black keys play chords`, S(0) === 2 && S(10) === 1 && S(6) === 1);
  press(0);
  check("CHORDS: a white key plays along, no answer", S(6) === 1 && S(4) === 0);
  press(answer());
  check("CHORDS: the black key that holds it", S(9) === 1 && S(2) === 3);
  tap(B.OCTUP);
  check("OCT+ round to NOTES; the chords off the black keys", S(0) === 0 && S(10) === 0);
  turn(EN.ALGO, 1);
  check("ALGORITHM on: SCRY's notes stopped", (() => { render(1500); return ex.web_voices(0) === 0; })());
}

// TRACE: T(field) as web_dr_trace names them
function trace() {
  const T = (f) => ex.web_dr_trace(f);
  const press = (k) => { ex.web_keys(1 << k); render(70); ex.web_keys(0); render(30); };
  check(`TRACE: the first song, its first key lit (${T(4)})`, ex.web_dr_kmode() === 4 && T(0) === 0 && T(1) === 0 &&
        ((ex.web_lit_keys() >> T(4)) & 1) === 1);
  press((T(4) + 2) % 27);
  check("a wrong key changes nothing", T(1) === 0);
  let n = 0;
  while (!T(2) && n < 300) { press(T(4)); n++; }
  check(`followed to its end (${n} keys), then round again from the top`, T(2) === 1 && (() => { render(3000); return T(2) === 0 && T(1) === 0; })());
  tap(B.OCTUP);
  check("OCT+: the next song", T(0) === 1);
  turn(EN.SELECT, -2);
  check("SELECT back past the first: the last song", T(0) === 13);
  turn(EN.SELECT, 1);
  const demo = level(() => { tap(B.PLAY); render(2500); });
  check(`PLAY: the song played (peak ${demo.peak.toFixed(3)}), its key lit as it goes`,
        T(3) === 1 && T(5) > 2 && demo.peak > 0.02 && ((ex.web_lit_keys() >> T(4)) & 1) === 1 && T(0) === 0);
  tap(B.PLAY);
  render(2500);
  check("PLAY again: stopped, silent", T(3) === 0 && ex.web_voices(0) === 0 && T(1) === 0);
}

{ // the spellbook modes
  turn(EN.ALGO, 1);
  check("ALGORITHM: WEAVE", ex.web_dr_kmode() === 1);
  ex.web_keys(1 << 5); render(200);                       // A#3: A minor
  const strum = [];
  for (const w of [0, 2, 4, 6, 7]) { ex.web_keys(1 << 5 | 1 << w); render(120); strum.push(ex.web_kb_note(w)); }
  ex.web_keys(0); render(1500);
  check(`WEAVE: A#3 picks Am, the white keys strum it from C3 (${strum.join(" ")})`,
        ex.web_dr_chord() === 2 && strum.join() === [48, 52, 57, 60, 64].join());
  turn(EN.ALGO, 1);
  check("ALGORITHM again: CAST", ex.web_dr_kmode() === 2);
  render(3000);
  ex.web_keys(1 << 15); render(400);                      // G#4: G7
  const chord = ex.web_voices(0);
  ex.web_keys(1 << 15 | 1 << 19); render(300);            // and a tune note (C5)
  const withTune = ex.web_voices(0);
  ex.web_keys(0); render(3000);
  check(`CAST: G#4 holds G7 (${chord} voices), a tune over it (${withTune})`, ex.web_dr_chord() === 6 && chord === 4 && withTune === 5);
  turn(EN.SELECT, 1);
  check("SELECT: the key a fifth up, G", ex.web_dr_key() === 7);
  turn(EN.ALGO, -1);                                      // WEAVE in G: A#3 is vi, E minor
  ex.web_keys(1 << 5); render(200);
  const inG = [];
  for (const w of [0, 2, 4, 6, 7]) { ex.web_keys(1 << 5 | 1 << w); render(120); inG.push(ex.web_kb_note(w)); }
  ex.web_keys(0); render(1500);
  check(`WEAVE in G: A#3 is vi, Em, strummed from C3 (${inG.join(" ")})`, inG.join() === [52, 55, 59, 64, 67].join());
  const tune = () => [0, 6, 7].map((w) => { ex.web_keys(1 << w); render(120); const n = ex.web_kb_note(w); ex.web_keys(0); render(150); return n; });
  turn(EN.ALGO, 1);                                       // CAST in G: the white keys take its F#
  const castG = tune();
  check(`CAST in G: F3 B3 C4 play F#3 B3 C4 (${castG.join(" ")})`, castG.join() === [54, 59, 60].join());
  turn(EN.SELECT, -2);
  check("SELECT the other way: round to F", ex.web_dr_key() === 5);
  const castF = tune();
  check(`CAST in F: F3 B3 C4 play F3 Bb3 C4 (${castF.join(" ")})`, castF.join() === [53, 58, 60].join());
  render(1500);
  turn(EN.SELECT, 1);
  turn(EN.ALGO, -1);
  scry();
  trace();
  turn(EN.ALGO, 3);
  check("ALGORITHM stops at the last mode", ex.web_dr_kmode() === 4);
  turn(EN.ALGO, -7);
  check("and back to SOLO: the keys their own notes", ex.web_dr_kmode() === 0 && (() => {
    ex.web_keys(1 << 7); render(100); const n = ex.web_kb_note(7); ex.web_keys(0); render(1500); return n === 60; })());
}

{ // the beats: PLAY, SEQ, SEQ + SELECT, the bass after CAST's chord, quiet in SCRY
  const BPM = [84, 122, 128, 138, 108, 150, 76, 124, 82, 104, 118, 100, 86, 112, 92, 100, 108, 76, 132, 94, 104, 84, 90, 116, 72, 112, 94, 80, 120, 70], c = ex.web_dr_char();
  tap(B.PLAY);
  const beat = level(() => render(3000));
  check(`PLAY: ${NAMES[c]}'s own beat (${ex.web_bpm()} BPM, ${beat.db.toFixed(1)} dB)`,
        ex.web_playing() === 1 && ex.web_bpm() === BPM[c] && beat.db > -34 && beat.db < -22);
  tap(B.SEQ);
  check(`SEQ: the next beat (${ex.web_bpm()} BPM)`, ex.web_playing() === 1 && ex.web_bpm() === BPM[(c + 1) % N]);
  ex.web_buttons(1 << B.SEQ); render(100); turn(EN.SELECT, 2); ex.web_buttons(0); render(200);
  check(`SEQ held, SELECT: the tempo, not the beat (${ex.web_bpm()} BPM)`, ex.web_bpm() === BPM[(c + 1) % N] + 8);
  {                                                       // REC: the tap tempo
    const tapAt = (gap, n) => { for (let i = 0; i < n; i++) { ex.web_buttons(1 << B.REC); render(60); ex.web_buttons(0); render(gap - 60); } };
    const head = () => rows(screen(), 0, 22);
    render(2500); const h0 = head(); tapAt(500, 4); render(100); const b1 = ex.web_bpm(), h1 = head();
    check(`REC: four taps half a second apart, 120 BPM (${b1}), the header shows it`, Math.abs(b1 - 120) <= 3 && h1 !== h0 && ex.web_playing() === 1);
    render(2500); tapAt(400, 4); render(100); const b2 = ex.web_bpm();
    check(`REC: faster taps, 150 BPM (${b2}; the buttons are read once a frame, so a BPM or two either way)`, Math.abs(b2 - 150) <= 3);
    render(2500); tapAt(120, 2); render(100);
    check(`REC: a pause starts over, and a bounce is not a tap (still ${ex.web_bpm()})`, ex.web_bpm() === b2);
  }
  turn(EN.ALGO, 2);                                       // CAST: G#4 picks G7, the bass moves to G
  ex.web_keys(1 << 15); render(300);
  const g = ex.web_step_note(1, 0); ex.web_keys(0); render(300);
  check(`CAST: the bass follows the chord (G7: ${g})`, g === 31 && ex.web_playing() === 1);
  turn(EN.ALGO, 1);
  check("SCRY: the beat stops (the ear wants quiet)", ex.web_playing() === 0);
  turn(EN.ALGO, -3); render(1500);
  tap(B.PLAY); render(500);                               // (playing, for HOME + SAVE to stop)
}

{ // YOUR NAME: HOME held and ALGORITHM open it, PRESETS a letter, ALGORITHM the place, SAVE keeps it; it stays after
  // a power-off, and the startup screen greets it
  const splashTop = (e) => { let gold = 0; const s = new Uint16Array(e.memory.buffer, e.web_screen(), 240 * 240);
    for (let i = 0; i < 240 * 70; i++) { const v = (s[i] >> 8) | (s[i] & 255) << 8;   // (big-endian, as the LCD takes it)
      if ((v >> 11) > 26 && ((v >> 5) & 63) > 40 && (v & 31) < 16) gold++; } return gold; };
  ex.web_buttons(1 << B.HOME); render(100); turn(EN.ALGO, 1); ex.web_buttons(0); render(200);
  const opened = ex.web_dr_name_on();
  ex.web_keys(1 << 7); render(100); ex.web_keys(0); render(100);                       // (the keys still play)
  const letters = [10, 15, 4];                                                        // J O D: J's the 10th after the space
  for (let i = 0; i < letters.length; i++) { turn(EN.PRESETS, letters[i]); turn(EN.ALGO, 1); }
  tap(B.SAVE); render(1500);                                                          // (settings_poll writes it)
  const name = [0, 1, 2, 3].map((i) => ex.web_dr_name(i)).filter((c) => c).map((c) => String.fromCharCode(c)).join("");
  check(`HOME held, ALGORITHM: YOUR NAME; PRESETS, ALGORITHM, SAVE: kept (${name})`, opened === 1 && ex.web_dr_name_on() === 0 && name === "JOD");
  const { instance: again } = await WebAssembly.instantiate(bytes, {});              // a power-off: the same flash
  const e2 = again.exports; e2._initialize();
  new Uint8Array(e2.memory.buffer, e2.web_nor(), e2.web_nor_size()).set(new Uint8Array(mem.buffer, ex.web_nor(), ex.web_nor_size()));
  e2.web_master(1023); e2.web_boot();
  for (let k = 0; k < 300 * 44.1; k += 128) e2.web_render(128);
  const named = splashTop(e2);
  const kept = [0, 1, 2].map((i) => String.fromCharCode(e2.web_dr_name(i))).join("");
  check(`after a power-off: still ${kept}, and the startup screen says hello (${named} gold px in the sky)`, kept === "JOD" && named > 150);
  ex.web_buttons(1 << B.HOME); render(100); turn(EN.ALGO, 1); ex.web_buttons(0); render(200);
  turn(EN.PRESETS, -1); tap(B.HOME); render(300);                                     // HOME: left as it was
  check("HOME leaves YOUR NAME as it was", ex.web_dr_name_on() === 0 && ex.web_dr_name(0) === 74);
}

{ // GLO held, PRESETS: five characters a detent (round the cast both ways)
  const c0 = ex.web_dr_char();
  ex.web_buttons(1 << B.GLO); render(100); turn(EN.PRESETS, 1); ex.web_buttons(0); render(200);
  const up = ex.web_dr_char();
  ex.web_buttons(1 << B.GLO); render(100); turn(EN.PRESETS, -2); ex.web_buttons(0); render(200);
  const down = ex.web_dr_char();
  check(`GLO held, PRESETS: five a detent (${NAMES[c0]} -> ${NAMES[up]}, then back two detents -> ${NAMES[down]})`,
        up === (c0 + 5) % N && down === (c0 + N - 5) % N);
  turn(EN.PRESETS, -(down - c0 + N) % N === 0 ? 0 : (c0 - down + N) % N); render(500);
}

{ // ARP: the generative arp. On, its settings four at a time, the held keys' notes on the beat, AUTO, off
  turn(EN.ALGO, -10); render(200);                         // (SOLO)
  turn(EN.PRESETS, -ex.web_dr_char()); render(400);        // (Vesper: up and back in triplets, 9 hits in 12)
  const cards = () => rows(screen(), 198, 240), line = () => rows(screen(), 22, 38);
  const c0 = cards();
  tap(B.ARP);
  check(`ARP: on, its NOTES on the cards (SHAPE ${ex.web_dr_arp_p(0)}, RATE ${ex.web_dr_arp_p(1)})`,
        ex.web_dr_arp() === (1 | 1 << 1) && cards() !== c0);
  const n0 = ex.web_dr_arp_count(), seen = new Set();
  ex.web_keys(1 << 7 | 1 << 11 | 1 << 14);                 // C E G held
  for (let i = 0; i < 20; i++) { render(100); seen.add(ex.web_dr_arp_note() % 12); }
  const n1 = ex.web_dr_arp_count(), per = (n1 - n0) / 2, want = 3 * ex.web_bpm() / 60 * 9 / 12;
  check(`ARP: C E G held 2 s, ${n1 - n0} notes (${per.toFixed(1)} a second, about ${want.toFixed(1)} at ${ex.web_bpm()} BPM), C E G and (SPICE) a passing note in C now and then (${[...seen].join(" ")})`,
        Math.abs(per - want) < 1.2 && [0, 4, 7].every((p) => seen.has(p)) && [...seen].every((p) => [0, 2, 4, 5, 7, 9, 11].includes(p)));
  ex.web_keys(0); render(400); const n2 = ex.web_dr_arp_count(); render(1000);
  check("ARP: keys let go (SOURCE KEYS), it rests", ex.web_dr_arp_count() === n2);
  const l1 = line();
  tap(B.ARP);
  check("ARP again: RHYTHM on the cards and the line (its steps)", ex.web_dr_arp() === (1 | 2 << 1) && line() !== l1);
  const st = ex.web_dr_arp_p(4); turn(EN.K1, -4);
  check(`RHYTHM: STEPS a step a detent (${st} -> ${ex.web_dr_arp_p(4)}), HITS no more than STEPS`,
        ex.web_dr_arp_p(4) === st - 4 && ex.web_dr_arp_p(5) <= ex.web_dr_arp_p(4));
  turn(EN.K1, 4);
  tap(B.ARP); tap(B.ARP);
  check(`ARP twice more: FEEL (SWING ${ex.web_dr_arp_p(12)}, HUMAN ${ex.web_dr_arp_p(13)}, ACCENT ${ex.web_dr_arp_p(14)}, LEAP ${ex.web_dr_arp_p(15)})`,
        ex.web_dr_arp() === (1 | 4 << 1) && ex.web_dr_arp_p(13) === 30);
  { // HUMAN: the notes land a little late, at random; with it at 0 they land on the grid
    const spread = (h) => {
      turn(EN.K2, -60); turn(EN.K2, h / 5);
      const step = 60000 / ex.web_bpm() / 3, offs = [];                   // (Vesper's RATE: eighth triplets)
      ex.web_keys(1 << 7 | 1 << 11 | 1 << 14); render(300);
      for (let ms = 0; ms < 3000; ms += 2) { const c = ex.web_dr_arp_count(); render(2); if (ex.web_dr_arp_count() !== c) offs.push(ms % step); }
      ex.web_keys(0); render(400);
      const lo = Math.min(...offs); return Math.max(...offs.map((o) => (o - lo + step) % step)); };   // (round the step)
    const tight = spread(0), loose = spread(100);
    turn(EN.K2, -60); turn(EN.K2, 6);
    check(`FEEL: HUMAN spreads when the notes start (${tight.toFixed(0)} ms on the grid at 0, ${loose.toFixed(0)} ms at 100)`,
          tight < 8 && loose > 15);
  }
  tap(B.ARP);
  check("ARP again: the character's own knobs (the arp still on)", ex.web_dr_arp() === 1 && cards() === c0);
  turn(EN.PRESETS, 6); render(500);                         // Murk: AUTO, she plays by herself
  const a0 = ex.web_dr_arp_count(), keyset = new Set();
  for (let i = 0; i < 30; i++) { render(100); keyset.add(ex.web_dr_arp_note() % 12); }
  const C_MAJOR = [0, 2, 4, 5, 7, 9, 11];
  check(`ARP, MURK (SOURCE AUTO): plays with no keys held (${ex.web_dr_arp_count() - a0} notes), in C major (${[...keyset].join(" ")})`,
        ex.web_dr_arp_count() - a0 >= 4 && [...keyset].every((p) => C_MAJOR.includes(p)));
  ex.web_buttons(1 << B.ARP); render(900); ex.web_buttons(0); render(300);
  const a1 = ex.web_dr_arp_count(); render(800);
  check("ARP held: off, it stops", ex.web_dr_arp() === 0 && ex.web_dr_arp_count() === a1);
  const l = level(() => { ex.web_keys(1 << 7); render(500); ex.web_keys(0); render(200); });
  check(`and the keys play as before (peak ${l.peak.toFixed(3)})`, l.peak > 0.02);
  tap(B.ARP); turn(EN.ALGO, 3); render(300);
  check("SCRY turns the arp off", ex.web_dr_arp() === 0);
  turn(EN.ALGO, -10); turn(EN.PRESETS, -6); render(500);
}

tap(B.OCTUP);
check("OCT+: the keys an octave up", ex.web_octave() === 1);
tap(B.OCTDN);
check("OCT-: back", ex.web_octave() === 0);

{ // HOME + SAVE
  ex.web_buttons(1 << B.HOME | 1 << B.SAVE); render(2000); ex.web_buttons(0); render(300);
  check("HOME + SAVE let go after 2 s: still Double Rainbow", ex.web_dr_on() === 1);
  ex.web_buttons(1 << B.HOME | 1 << B.SAVE); render(3200); ex.web_buttons(0); render(500);
  check(`held 3 s: the full Melodee, track 1 its own sound again (engine ${ex.web_engine(0)}: ANALOG)`,
        ex.web_dr_on() === 0 && ex.web_engine(0) === 0);
  check(`and the beat stopped, track 2 Melodee's own again (engine ${ex.web_engine(1)}: FM6), the tempo back (${ex.web_bpm()})`,
        ex.web_playing() === 0 && ex.web_engine(1) === 12 && ex.web_bpm() === 120);
  const l = level(() => { ex.web_keys(1 << 7); render(400); ex.web_keys(0); render(200); });
  check(`and the keys play Melodee (peak ${l.peak.toFixed(3)})`, l.peak > 0.02);
}
console.log(fails ? `dr: ${fails} FAILED` : "dr: all ok");
process.exit(fails ? 1 : 0);
