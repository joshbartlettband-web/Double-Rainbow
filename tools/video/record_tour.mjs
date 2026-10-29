// Double Rainbow, a tour: raw RGB frames (30 fps), a WAV, captions, and the keys held on each frame.
//   node tools/video/record_tour.mjs build/emu/melodee.wasm OUT_DIR   then compose_tour.py in OUT_DIR
// Everything is played on the browser build, as a player would: SCRY is answered and TRACE followed by pressing the keys the
// game asks for (watchers on its state), so what the screen shows is what the firmware did.
import fs from "fs";
const [wasmPath, out] = process.argv.slice(2);
const B = { FX: 0, SCL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11, OCTDN: 12, OCTUP: 13 };
const EN = { SELECT: 0, ALGO: 1, PRESETS: 2, K1: 3, K2: 4, K3: 5, K4: 6 };
const K = { F3: 0, G3: 2, A3: 4, B3: 6, C: 7, D: 9, E: 11, F: 12, G: 14, A: 16, B: 18, C2: 19, D2: 21, E2: 23, F2: 24, G2: 26 };
const BK = { IV: 1, V: 3, vi: 5, I: 8, ii: 10, IVmaj7: 13, V7: 15, vi7: 17, Imaj7: 20, ii7: 22, iv: 25 };   // the black keys' chords
const NC = 30;
const CH = { VESPER: 0, SHARD: 1, SQUALL: 2, WICK: 3, TOCK: 4, MOTE: 5, MURK: 6, BOG: 7, HUSH: 8, GLINT: 10, HERMIT: 11,
             ECHO: 14, CAIRN: 19, GLOAM: 22, TUPPENCE: 21, KITE: 25, BRAMBLE: 26, HOLLOW: 27, FLITTER: 28, DOZE: 24,
             MOSSBACK: 29 };
const { instance } = await WebAssembly.instantiate(fs.readFileSync(wasmPath), {});
const ex = instance.exports, mem = ex.memory;
ex._initialize(); ex.web_nor_erase(); ex.web_master(1023); ex.web_boot();
let keysNow = 0;
const setKeys = (m) => { keysNow = m; ex.web_keys(m); };
const ev = [], watchers = [], cap = [];
const at = (t, f) => ev.push([t, f]);
const C = (t, a, b) => cap.push([t, a, b]);
const note = (t, k, len) => { at(t, () => setKeys(1 << k)); at(t + len, () => setKeys(0)); };
const hold = (t, keys, len) => { at(t, () => setKeys(keys.reduce((a, k) => a | (1 << k), 0))); at(t + len, () => setKeys(0)); };
const tap = (t, b) => { at(t, () => ex.web_buttons(1 << b)); at(t + 80, () => ex.web_buttons(0)); };
const turn = (t, r, n, gap = 120) => { for (let i = 0; i < Math.abs(n); i++) at(t + i * gap, () => ex.web_enc(r, Math.sign(n))); };
const tune = (t, keys, step) => keys.forEach((k, i) => k !== null && note(t + i * step, k, step - 60));
const watch = (t, f) => at(t, () => watchers.push(f));             // f(ms) each frame from t until it returns true
let navTo = null;
const go = (t, c) => at(t, () => { navTo = c; });
// SCRY: each question answered a while after it is asked (NOTES: the key of the note asked; web_dr_scry: 6 asked, 8 the note)
const scry = (t, n, think = 900) => { let done = 0, since = -1; watch(t, (ms) => {
  if (ex.web_dr_scry(6) !== 1) { since = -1; return done >= n; }
  if (since < 0) since = ms;
  if (ms - since >= think) { const k = ex.web_dr_scry(8) - 53; note(ms, k, 220); done++; since = 1e9; }
  return false; }); };
// TRACE: the lit key pressed, n times, a step apart (web_dr_trace(4): the key lit)
const trace = (t, n, step) => { let done = 0, next = 0; watch(t, (ms) => {
  if (ms < next) return false;
  const k = ex.web_dr_trace(4);
  if (k >= 0) { note(ms, k, step - 80); done++; }
  next = ms + step; return done >= n; }); };

// cold open: the moonbow, then Vesper
C(0, "Double Rainbow", "free firmware for the M-VAVE FM-1, for players ten and up");
hold(2600, [K.A3, K.C, K.E], 900); hold(3600, [K.F3, K.A3, K.C], 900); hold(4600, [K.G3, K.B3, K.D], 1400);
C(2600, "Double Rainbow", "VESPER, a moth in a velvet cape: a Prophet-5");
// 1. the cast
let t = 6600;
const CAST = [[CH.SHARD, "SHARD, a glass fox: FM, like the DX7", (s) => tune(s, [K.E2, K.B, K.G, K.E], 260)],
              [CH.SQUALL, "SQUALL, a storm cloud: noise", (s) => { note(s, K.C, 500); note(s + 600, K.G3, 600); }],
              [CH.WICK, "WICK, a candle who sings vowels: formants", (s) => tune(s, [K.C, K.E, K.G], 380)],
              [CH.TOCK, "TOCK, a clockwork beetle: the 808", (s) => tune(s, [K.F3, K.A3, K.G3, K.A3, K.F3, K.A3, K.G3, K.B3], 150)],
              [CH.MOTE, "MOTE, a gremlin: an old game console", (s) => tune(s, [K.C, K.E, K.G, K.C2, K.G], 200)],
              [CH.MURK, "MURK, a jellyfish: phase distortion", (s) => hold(s, [K.C, K.E, K.A], 1200)],
              [CH.GLINT, "GLINT, a magpie: Casio's CZ-1, its own 64 tones", (s) => tune(s, [K.A, K.E, K.C, K.E], 280)],
              [CH.CAIRN, "CAIRN, stacked pebbles: struck stones and a kalimba", (s) => tune(s, [K.C, K.D, K.E, K.G], 260)],
              [CH.GLOAM, "GLOAM, the Keeper's logbook: tower bells", (s) => { note(s, K.C, 500); note(s + 550, K.G3, 650); }],
              [CH.FLITTER, "FLITTER, a bat with a squeezebox: the CZ-1's accordion", (s) => tune(s, [K.C, K.E, K.G, K.E, K.C], 220)],
              [CH.DOZE, "DOZE, a snail in a gramophone horn: lo-fi", (s) => tune(s, [K.E, K.D, K.C], 480)],
              [CH.BRAMBLE, "BRAMBLE, a hedgehog: bottle caps on an 808", (s) => tune(s, [K.F3, K.A3, K.G3, K.A3, K.F3, K.F3, K.G3, K.A3], 150)]];
CAST.forEach(([c, sub, play], i) => { go(t, c); C(t, "1. Meet 30 escaped sounds", sub); play(t + 350); t += 1800; });
// 2. knobs, EDIT, the codex
go(t, CH.BOG);
C(t, "2. Four knobs, real settings", "BOG, a swamp toad: the TB-303 squelch. SQUISH and GOO");
const acid = [K.C, K.C, K.C2, K.C, K.D2, K.C, K.G, K.C];
for (let r = 0; r < 3; r++) tune(t + 400 + r * 1600, acid, 200);
turn(t + 600, EN.K1, 18, 60); turn(t + 2000, EN.K2, 16, 60); turn(t + 3200, EN.K1, -12, 60);
at(t + 4200, () => ex.web_buttons(1 << B.EDIT));
C(t + 4200, "2. Four knobs, real settings", "hold EDIT: what each knob really moves");
at(t + 6000, () => ex.web_buttons(0));
t += 6400;
tap(t, B.SCL);
C(t, "3. The Keeper's ledger", "SCL: a page per character, in the Keeper's voice");
turn(t + 2200, EN.PRESETS, 1);
C(t + 2200, "3. The Keeper's ledger", "then the engine's real history, and something to try");
turn(t + 3800, EN.PRESETS, 1);
tap(t + 5400, B.SCL);
t += 5700;
// 4. beats
go(t, CH.MOTE);
tap(t + 400, B.PLAY);
C(t, "4. A beat for everyone", "PLAY: Mote's ARCADE. The bass follows the key");
tune(t + 800, [K.C, K.E, K.G, K.E, K.A, K.G, K.E, K.D], 200);
go(t + 2700, CH.HERMIT);
C(t + 2700, "4. A beat for everyone", "HERMIT, a crab in a horn: PARADE, and Sequential's Iconic Brass");
hold(t + 3300, [K.C, K.E, K.G], 700); hold(t + 4100, [K.D, K.F, K.A], 700); hold(t + 4900, [K.E, K.G, K.B], 900);
C(t + 6000, "4. A beat for everyone", "tap REC to set the tempo. It shows at the top while it plays");
for (let i = 0; i < 5; i++) tap(t + 6100 + i * 430, B.REC);
hold(t + 8400, [K.C, K.E, K.G], 600); hold(t + 9300, [K.D, K.F, K.A], 600);
tap(t + 10400, B.PLAY);
t += 10700;
// 5. the arp
go(t, CH.KITE);
tap(t + 300, B.PLAY);
tap(t + 600, B.ARP);
C(t, "5. An arp for everyone", "ARP: each character's own, on the beat. Hold a chord");
hold(t + 900, [K.C, K.E, K.G, K.B], 3200);
at(t + 4300, () => {}); tap(t + 4400, B.ARP); tap(t + 4700, B.ARP); tap(t + 5000, B.ARP);
C(t + 4400, "5. An arp for everyone", "16 settings, four at a time. FEEL: SWING, and HUMAN, as hands play");
turn(t + 5400, EN.K2, 14, 50);
hold(t + 5500, [K.A3, K.C, K.E, K.G], 2600);
go(t + 8400, CH.HOLLOW);
C(t + 8400, "5. An arp for everyone", "some play by themselves. Hollow, the empty jar, hums along");
tap(t + 8700, B.ARP);
at(t + 12300, () => ex.web_buttons(1 << B.ARP)); at(t + 13200, () => ex.web_buttons(0));
tap(t + 13400, B.PLAY);
t += 13800;
// 5. the spellbook: WEAVE and CAST
go(t, CH.SHARD);
turn(t + 300, EN.ALGO, 1);
C(t, "6. The spellbook: WEAVE", "a black key picks a chord, the white keys strum it");
const strum = (s, bk, step = 60) => { at(s, () => setKeys(1 << bk)); [0, 2, 4, 6, 7, 9, 11, 12, 14].forEach((w, i) => at(s + 150 + i * step, () => setKeys(1 << bk | 1 << w))); at(s + 150 + 9 * step + 200, () => setKeys(0)); };
strum(t + 700, BK.I); strum(t + 1700, BK.vi); strum(t + 2700, BK.IV); strum(t + 3700, BK.V);
t += 4800;
go(t, CH.HUSH);
turn(t + 200, EN.ALGO, 1);
C(t, "6. The spellbook: CAST", "chords under a tune, with their names and numerals");
const cast = (s, bk, tn, len) => { at(s, () => setKeys(1 << bk)); tn.forEach((k, i) => { at(s + 200 + i * len, () => setKeys(1 << bk | 1 << k)); at(s + 200 + i * len + len - 60, () => setKeys(1 << bk)); }); at(s + 200 + tn.length * len, () => setKeys(0)); };
cast(t + 600, BK.Imaj7, [K.E2, K.D2], 380); cast(t + 1500, BK.vi7, [K.C2, K.B], 380); cast(t + 2400, BK.ii7, [K.A, K.C2], 380); cast(t + 3300, BK.V7, [K.B, K.G], 450);
turn(t + 4500, EN.SELECT, 1);
C(t + 4500, "6. The spellbook: CAST", "SELECT: a new key, round the circle of fifths");
cast(t + 4900, BK.Imaj7, [K.D2, K.B], 380); cast(t + 5800, BK.V7, [K.A, K.F], 450);
turn(t + 7000, EN.SELECT, -1);
t += 7200;
// 6. SCRY
turn(t, EN.ALGO, 1);
C(t, "7. SCRY: train your ear", "hear a note, then find it. Three right, and a new one joins");
scry(t + 600, 4);
t += 9000;
// 7. TRACE
turn(t, EN.ALGO, 1);
C(t, "8. TRACE: songs to learn", "the next key lights up. Korobeiniki, at your own pace");
trace(t + 800, 16, 300);
C(t + 5800, "8. TRACE: songs to learn", "14 tunes: Hall of the Mountain King, Für Elise, Dies Irae");
t += 7000;
// the summary, over a beat
turn(t, EN.ALGO, -4, 80);
go(t + 300, CH.TUPPENCE);
tap(t + 1000, B.PLAY);
C(t, "What's inside", "__LIST__");
const items = 9, gap = 1450;
tune(t + 1400, [K.A3, null, K.C, null, K.E, null, K.D, null, K.C, null, K.B3, null, K.C, null, K.E, null, K.A3, null, K.C, null, K.E], 420);
t += 1000 + items * gap + 1800;
C(t, "Which jar should open next?", "__END__");
tune(t + 300, [K.E, K.A, K.C2, null, K.B, K.E2], 330);
tap(t + 3600, B.PLAY);
const END = t + 5200;

ev.sort((a, b) => a[0] - b[0]);
const FPS = 30, SPF = 1470, frames = Math.ceil(END / 1000 * FPS);
const raw = fs.openSync(`${out}/frames.rgb`, "w"), pcm = [], keys = [];
let e = 0, sorted = ev.length;
const resort = () => { if (ev.length !== sorted) { const rest = ev.splice(e).sort((a, b) => a[0] - b[0]); ev.push(...rest); sorted = ev.length; } };
for (let f = 0; f < frames; f++) {
  const now = f * 1000 / FPS;
  resort();
  while (e < ev.length && ev[e][0] <= now) { ev[e++][1](); resort(); }
  for (let i = watchers.length - 1; i >= 0; i--) if (watchers[i](now)) watchers.splice(i, 1);
  resort();
  if (navTo !== null) {
    const cur = ex.web_dr_char();
    if (cur === navTo) navTo = null; else ex.web_enc(EN.PRESETS, (navTo - cur + NC) % NC <= NC / 2 ? 1 : -1);
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
console.log(`scry streak ${ex.web_dr_scry(2)} pool ${ex.web_dr_scry(1)}, trace pos ${ex.web_dr_trace(1)}, char ${ex.web_dr_char()}`);
