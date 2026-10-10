// SPDX-License-Identifier: GPL-3.0-only
// The landing page's screenshots, taken from the browser build (the firmware itself, in WebAssembly):
//   node tools/landing/shots.mjs build/emu/melodee.wasm OUT_DIR    -> OUT_DIR/*.raw (240 x 240 RGB565, big-endian)
// tools/landing/make_landing.py turns them into the PNGs in landing/.
import fs from "fs";
const [wasm, out] = process.argv.slice(2);
const { instance } = await WebAssembly.instantiate(fs.readFileSync(wasm), {});
const ex = instance.exports, mem = ex.memory;
ex._initialize(); ex.web_nor_erase(); ex.web_master(1023); ex.web_boot();
const render = (ms) => { const n = Math.round(ms * 44.1); for (let k = 0; k < n; k += 128) ex.web_render(Math.min(128, n - k)); };
const turn = (role, n) => { for (let i = 0; i < Math.abs(n); i++) { ex.web_enc(role, Math.sign(n)); render(20); } render(150); };
const tap = (b) => { ex.web_buttons(1 << b); render(80); ex.web_buttons(0); render(150); };
const shot = (name) => fs.writeFileSync(`${out}/${name}.raw`, Buffer.from(new Uint16Array(mem.buffer, ex.web_screen(), 57600).slice().buffer));
const B = { SCL: 1, EDIT: 4, PLAY: 10, OCTUP: 13 }, EN = { SELECT: 0, ALGO: 1, PRESETS: 2 };
render(300); shot("splash");                           // (the startup screen shows for 430 ms)
render(1600);
const play = { 0: [7, 11, 14], 1: [16], 2: [9], 3: [11], 4: [7], 5: [14], 6: [4] };   // a chord for Vesper, a note each
for (let c = 0; c < 7; c++) {
  if (c) turn(EN.PRESETS, 1);
  render(600);
  ex.web_keys(play[c].reduce((m, k) => m | 1 << k, 0)); render(c === 4 ? 120 : 450); shot(`char-${c}`); ex.web_keys(0); render(1500);
}
turn(EN.PRESETS, 1); render(500);                                  // Vesper again
ex.web_buttons(1 << B.EDIT); render(250); shot("edit"); ex.web_buttons(0); render(250);
turn(EN.PRESETS, 6); render(500);                                  // Murk's codex: the ledger
tap(B.SCL); shot("codex"); tap(B.SCL); turn(EN.PRESETS, -5); render(500);   // Shard
turn(EN.ALGO, 2); render(800);                                     // CAST: a black key holds a chord
ex.web_keys(1 << 15 | 1 << 19); render(500); shot("cast"); ex.web_keys(0); render(1500);
turn(EN.ALGO, 1); render(300); tap(B.OCTUP); render(1800);          // SCRY, INTERVALS
for (let i = 0; i < 3; i++) {                                      // a right answer, its name shown
  const k = ex.web_dr_scry(8) - 53; ex.web_keys(1 << k); render(150); ex.web_keys(0); render(120);
  if (i < 2) render(1500);
}
shot("scry");
turn(EN.ALGO, 1); render(600);                                     // TRACE: the first song, a few notes in
for (let i = 0; i < 5; i++) { const k = ex.web_dr_trace(4); ex.web_keys(1 << k); render(90); ex.web_keys(0); render(60); }
render(400); shot("trace");
