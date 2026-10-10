// SPDX-License-Identifier: GPL-3.0-only
// Double Rainbow's checks (firmware/src/dr.c) on the browser build, driven like a player: the panel, the keys, the
// knobs; what the screen, the LEDs and the audio do.
//   node web/emu/dr_test.mjs build/emu/melodee.wasm
// Checks: it starts in Double Rainbow on Vesper; every character sounds, on its own engine, and the three are
// about as loud as each other; PRESETS goes round the cast; a knob moves its value, the real setting under it and
// the screen; EDIT held and the codex change the screen; every text fits its room; OCT shifts the keys; HOME +
// SAVE let go early stays, held 3 s goes to the full Melodee with track 1's own sound back, and the keys still play.
import fs from "fs";

const bytes = fs.readFileSync(process.argv[2] || "build/emu/melodee.wasm");
const B = { FX: 0, SCL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11, OCTDN: 12, OCTUP: 13 };
const EN = { SELECT: 0, ALGO: 1, PRESETS: 2, K1: 3, K2: 4, K3: 5, K4: 6 };
const ENGINE = [19, 12, 11];                       // PROPHET, FM6, NOISE
const P5_CUTOFF = 17, P_E0 = 104;                  // (prophet_patch.c; core.h: P_E0, checked below)
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

const loud = [];
for (let c = 0; c < 3; c++) {
  if (c) turn(EN.PRESETS, 1);
  render(300);
  const l = level(phrase);
  loud.push(l.db);
  check(`${["VESPER", "SHARD", "SQUALL"][c]} (PRESETS ${c ? "turned" : "at power-on"}): engine ${ex.web_engine(0)}, sounds (peak ${l.peak.toFixed(3)})`,
        ex.web_dr_char() === c && ex.web_engine(0) === ENGINE[c] && l.peak > 0.02);
}
const spread = Math.max(...loud) - Math.min(...loud);
check(`the three are about as loud as each other (RMS ${loud.map((x) => x.toFixed(1)).join(" ")} dB: ${spread.toFixed(1)} dB apart)`, spread < 3);
turn(EN.PRESETS, 1);
check("PRESETS goes round: after SQUALL, VESPER again", ex.web_dr_char() === 0 && ex.web_engine(0) === ENGINE[0]);
turn(EN.PRESETS, -1);
check("and back: SQUALL", ex.web_dr_char() === 2);
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
        brighter.bright - bright.bright > 3);
}

{ // EDIT held, the codex
  const cards = rows(screen(), 198, 240);
  ex.web_buttons(1 << B.EDIT); render(200);
  const held = rows(screen(), 198, 240), heldView = ex.web_dr_view();
  ex.web_buttons(0); render(200);
  check("EDIT held: the cards show what the knobs move; let go: back", held !== cards && (heldView & 4) && rows(screen(), 198, 240) === cards);
  const scene = rows(screen(), 38, 240);
  tap(B.SCL);
  const page0 = rows(screen(), 38, 240);
  turn(EN.PRESETS, 1);
  const page1 = rows(screen(), 38, 240), v1 = ex.web_dr_view();
  turn(EN.PRESETS, -1);
  tap(B.SCL);
  check("SCL: the codex, PRESETS its second page, SCL again: the character", page0 !== scene && page1 !== page0 &&
        (v1 & 3) === 3 && ex.web_dr_view() === 0 && rows(screen(), 38, 240) === scene && ex.web_dr_char() === 0);
  check("the keys still play in the codex and with EDIT held", (() => {
    tap(B.SCL); const a = level(() => { ex.web_keys(1 << 9); render(400); ex.web_keys(0); render(100); }); tap(B.SCL);
    ex.web_buttons(1 << B.EDIT); const b = level(() => { ex.web_keys(1 << 9); render(400); ex.web_keys(0); render(100); });
    ex.web_buttons(0); render(100);
    return a.peak > 0.02 && b.peak > 0.02;
  })());
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
  const l = level(() => { ex.web_keys(1 << 7); render(400); ex.web_keys(0); render(200); });
  check(`and the keys play Melodee (peak ${l.peak.toFixed(3)})`, l.peak > 0.02);
}
console.log(fails ? `dr: ${fails} FAILED` : "dr: all ok");
process.exit(fails ? 1 : 0);
