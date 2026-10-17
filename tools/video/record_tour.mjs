// SPDX-License-Identifier: GPL-3.0-only
// Double Rainbow, the tour: raw RGB frames (30 fps), a WAV, captions, and the keys held and lit on each frame.
//   node tools/video/record_tour.mjs build/emu/melodee.wasm OUT_DIR   then compose_tour.py in OUT_DIR
//   (build/emu/melodee.wasm: web/emu/build.sh with Emscripten, or tools/video/build_wasm_clang.sh without; or run the
//   tour-video workflow on GitHub). After Rainbow mode's tools/video/record_tour.mjs.
import fs from "fs";
const [wasmPath, out] = process.argv.slice(2);
const B = { FX: 0, SCL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11, OCTDN: 12, OCTUP: 13 };
const EN = { SELECT: 0, ALGO: 1, PRESETS: 2, K1: 3, K2: 4, K3: 5, K4: 6 };
const K = { F3: 0, G3: 2, A3: 4, B3: 6, C: 7, D: 9, E: 11, F: 12, G: 14, A: 16, B: 18, C2: 19, D2: 21, E2: 23, F2: 24, G2: 26 };
const NCHAR = 24;
const { instance } = await WebAssembly.instantiate(fs.readFileSync(wasmPath), {});
const ex = instance.exports, mem = ex.memory;
ex._initialize(); ex.web_nor_erase(); ex.web_master(1023); ex.web_boot();
let keysNow = 0;
const setKeys = (m) => { keysNow = m; ex.web_keys(m); };
const ev = [], cap = [];
const at = (t, f) => ev.push([t, f]);
const C = (t, a, b) => cap.push([t, a, b]);
const note = (t, k, len) => { at(t, () => setKeys(1 << k)); at(t + len, () => setKeys(0)); };
const hold = (t, keys, len) => { at(t, () => setKeys(keys.reduce((a, k) => a | (1 << k), 0))); at(t + len, () => setKeys(0)); };
const tap = (t, b) => { at(t, () => ex.web_buttons(1 << b)); at(t + 80, () => ex.web_buttons(0)); };
const turn = (t, r, n, gap = 120) => { for (let i = 0; i < Math.abs(n); i++) at(t + i * gap, () => ex.web_enc(r, Math.sign(n))); };
const tune = (t, keys, step) => keys.forEach((k, i) => k !== null && note(t + i * step, k, step - 60));
let navTo = null;
const go = (t, c) => at(t, () => { navTo = c; });                   // PRESETS a detent a frame (more drops steps)

// cold open: the moonbow, then Vesper
C(0, "Double Rainbow", "a game-like instrument for the M-VAVE FM-1, after dark");
hold(1600, [K.A3, K.C, K.E], 850); hold(2500, [K.F3, K.A3, K.C], 850); hold(3400, [K.C, K.E, K.G], 850); hold(4300, [K.G3, K.B3, K.D], 1300);
C(1600, "Double Rainbow", "VESPER on PROPHET: every note is her final scene");
// 1. the cast
let t = 6200;
const CAST = [[1, "SHARD on FM6: sharp, clear, honest to a fault", (s) => tune(s, [K.E2, K.B, K.G, K.E, K.G, K.B], 200)],
              [3, "WICK on VOICE: sings when nervous. So, always", (s) => { note(s, K.E, 600); note(s + 650, K.G, 600); }],
              [4, "TOCK on DRUM: keeps time. Keeps grudges", (s) => tune(s, [K.F3, K.A3, K.F3, K.A3, K.F3, K.F3, K.A3, K.B3], 160)],
              [5, "MOTE on LOFI: small. Loud. Probably your fault", (s) => tune(s, [K.C, K.E, K.G, K.C2, K.G, K.C2], 160)],
              [7, "BOG on ANALOG's ACID: squelch is a love language", (s) => tune(s, [K.A3, K.A3, K.A, K.A3, K.C, K.A3, K.D, K.E], 150)],
              [11, "HERMIT on PROPHET's brass: lives in a horn", (s) => { hold(s, [K.C, K.E, K.G], 500); hold(s + 600, [K.D, K.F, K.A], 700); }]];
CAST.forEach(([c, sub, play]) => { go(t, c); C(t, "1. A cast of 24, an engine each", sub); play(t + 450); t += 2000; });
// 2. knobs: BOG's SQUISH opened while the riff runs, then EDIT shows what it moves
go(t, 7);
C(t, "2. Four knobs, real settings", "BOG's SQUISH knob opens the filter");
for (let r = 0; r < 3; r++) tune(t + 300 + r * 1200, [K.A3, K.A3, K.A, K.A3, K.C, K.A3, K.D, K.E], 150);
turn(t + 400, EN.K1, 25, 120);
t += 4000;
at(t, () => ex.web_buttons(1 << B.EDIT)); at(t + 2600, () => ex.web_buttons(0));
C(t, "2. Four knobs, real settings", "hold EDIT: the real settings under each knob");
t += 3000;
// 3. the codex
tap(t, B.SCL);
C(t, "3. The codex", "SCL: the Keeper's ledger, a page for every character");
turn(t + 2800, EN.PRESETS, 1);
C(t + 2800, "3. The codex", "then the engine's real story");
turn(t + 5300, EN.PRESETS, 1);
C(t + 5300, "3. The codex", "and something to try");
tap(t + 7600, B.SCL);
t += 8000;
// 4. chords: WEAVE and CAST on TUMBLER, a DX7 electric piano
go(t, 9);
turn(t + 300, EN.ALGO, 1);                                         // WEAVE
C(t, "4. Spells for chords", "WEAVE: a black key picks a chord, the white keys strum it");
const sweep = (s, step) => [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19].forEach((k, i) => note(s + i * step, k, step + 30));
note(t + 800, 5, 300); sweep(t + 1100, 70);
note(t + 2100, 1, 300); sweep(t + 2400, 70);
t += 3600;
turn(t, EN.ALGO, 1);                                               // CAST
C(t, "4. Spells for chords", "CAST: hold a chord, play a tune over it");
const comp = (s, ch, mel, step) => { at(s, () => setKeys(1 << ch)); mel.forEach((k, i) => { at(s + 150 + i * step, () => setKeys((1 << ch) | (1 << k))); at(s + 150 + i * step + step - 50, () => setKeys(1 << ch)); }); at(s + 150 + mel.length * step, () => setKeys(0)); };
comp(t + 500, 20, [K.E, K.G, K.B], 330);                           // Cmaj7
comp(t + 1700, 17, [K.C2, K.A, K.E], 330);                         // Am7
comp(t + 2900, 13, [K.A, K.C2, K.E2], 330);                        // Fmaj7
C(t + 4100, "4. Spells for chords", "the top key: F minor, borrowed. The bittersweet one");
comp(t + 4100, 25, [K.G, K.A, K.C2], 400);                         // Fm
t += 6000;
// 5. the ear
turn(t, EN.ALGO, 1);                                               // SCRY
tap(t + 300, B.OCTUP);                                             // the INTERVALS drill
C(t, "5. Train your ear", "SCRY: hear two notes, find the second");
const answer = (s) => { let k = 0; at(s, () => { k = ex.web_dr_scry(8) - 53; setKeys(1 << k); }); at(s + 160, () => setKeys(0)); };
answer(t + 2100); answer(t + 3900); answer(t + 5700);
C(t + 2300, "5. Train your ear", "it names what you found. Three in a row adds a new one");
t += 7300;
turn(t, EN.ALGO, 1);                                               // TRACE
C(t, "5. Train your ear", "TRACE: learn a song, one key at a time");
for (let i = 0; i < 14; i++) { const s = t + 900 + i * 300; let k = 0; at(s, () => { k = ex.web_dr_trace(4); if (k >= 0) setKeys(1 << k); }); at(s + 200, () => setKeys(0)); }
C(t + 2700, "5. Train your ear", "14 songs, from Korobeiniki to Satie");
t += 5600;
// the summary, over GLINT's disco beat
turn(t, EN.ALGO, -4, 80);                                          // SOLO
go(t + 400, 10);                                                   // GLINT
tap(t + 1200, B.PLAY);
C(t, "Inside Double Rainbow", "__LIST__");
const items = 8, gap = 1300;
tune(t + 1700, [K.E, null, K.G, K.A, null, K.G, K.E, null, K.D, null, K.E, K.G, null, K.A, K.C2, null, K.A, K.G], 300);
t += 900 + items * gap + 1500;
C(t, "What should we add next?", "__END__");
tune(t + 300, [K.A, K.C2, K.E2, null, K.D2, K.C2], 330);
tap(t + 3600, B.PLAY);
const END = t + 5600;

ev.sort((a, b) => a[0] - b[0]);
const FPS = 30, SPF = 1470, frames = Math.ceil(END / 1000 * FPS);
const raw = fs.openSync(`${out}/frames.rgb`, "w"), pcm = [], keys = [];
let e = 0;
for (let f = 0; f < frames; f++) {
  const tt = f * 1000 / FPS;
  while (e < ev.length && ev[e][0] <= tt) ev[e++][1]();
  if (navTo !== null) {
    const cur = ex.web_dr_char();
    if (cur === navTo) navTo = null; else ex.web_enc(EN.PRESETS, (navTo - cur + NCHAR) % NCHAR <= NCHAR / 2 ? 1 : -1);
  }
  for (let k = 0; k < SPF; k += 147) {
    ex.web_render(147);
    const l = new Float32Array(mem.buffer, ex.web_out_l(), 147), r = new Float32Array(mem.buffer, ex.web_out_r(), 147);
    for (let i = 0; i < 147; i++) pcm.push(l[i], r[i]);
  }
  keys.push([keysNow, ex.web_lit_keys()]);
  const fb = new Uint16Array(mem.buffer, ex.web_screen(), 240 * 240), b = Buffer.alloc(240 * 240 * 3);
  for (let i = 0; i < fb.length; i++) {
    const v = ((fb[i] & 255) << 8) | (fb[i] >> 8);
    b[i * 3] = ((v >> 11) << 3) | (v >> 13); b[i * 3 + 1] = (((v >> 5) & 63) << 2) | ((v >> 9) & 3); b[i * 3 + 2] = ((v & 31) << 3) | ((v >> 2) & 7);
  }
  fs.writeSync(raw, b);
}
fs.closeSync(raw);
let pk = 0; for (const v of pcm) pk = Math.max(pk, Math.abs(v));
const g = 0.89 / pk, wav = Buffer.alloc(44 + pcm.length * 2);
wav.write("RIFF", 0); wav.writeUInt32LE(36 + pcm.length * 2, 4); wav.write("WAVEfmt ", 8); wav.writeUInt32LE(16, 16);
wav.writeUInt16LE(1, 20); wav.writeUInt16LE(2, 22); wav.writeUInt32LE(44100, 24); wav.writeUInt32LE(44100 * 4, 28);
wav.writeUInt16LE(4, 32); wav.writeUInt16LE(16, 34); wav.write("data", 36); wav.writeUInt32LE(pcm.length * 2, 40);
pcm.forEach((v, i) => wav.writeInt16LE(Math.max(-32767, Math.min(32767, Math.round(v * g * 32767))), 44 + i * 2));
fs.writeFileSync(`${out}/audio.wav`, wav);
fs.writeFileSync(`${out}/captions.json`, JSON.stringify({ fps: FPS, frames, captions: cap, keys, list_gap: gap }));
console.log(`frames ${frames}, audio ${(pcm.length / 2 / 44100).toFixed(1)} s, peak ${pk.toFixed(3)} (gain ${g.toFixed(2)})`);
