#!/usr/bin/env python3
"""Mote's marble becomes a glass jar stopper, without another Gemini image. The work is done on the source's own
pixel grid (11 source pixels a cell). The ball stays as the stopper's round knob, its swirl cleared to plain glass;
the stopper's tapered plug (frosted, as ground glass is) rises from it, so he holds the stopper up upside down, like
a prize. The plug leans with the ball: he holds it tilted, so its top is left of its middle. Reads art/gemini/mote_a.jpg, writes art/gemini/mote_a_fix.png."""
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
G, X0, Y0, GW, GH = 11, 440, 14, 26, 30                  # the grid, and the region it covers (cells)
OUT = (40, 16, 18)
GLASS, GLASS_D, GLASS_L, SHINE, FROST, FROST_D = (128, 170, 212), (96, 132, 182), (186, 222, 240), (240, 250, 252), \
    (204, 216, 230), (156, 174, 196)
a = np.asarray(Image.open(ROOT / "art/gemini/mote_a.jpg").convert("RGB")).astype(np.int32)
cell = a[Y0 + 5:Y0 + GH * G:G, X0 + 5:X0 + GW * G:G][:GH, :GW].copy()     # each cell's centre colour
r, g, b = cell[..., 0], cell[..., 1], cell[..., 2]
cy, cx = np.mgrid[0:GH, 0:GW]
L = cell.mean(-1)
inside = (((cx - 13.5) / 5.2) ** 2 + ((cy - 17.0) / 7.2) ** 2 < 1)       # the ball's glass, not his claws
warm = inside & (r > b + 15) & (r > 120)
cell[warm & (L >= 150)] = GLASS_L
cell[warm & (L < 150)] = GLASS
cell[inside & (L > 215)] = SHINE

# the plug: it comes out of the ball's top, which is not above its middle (he holds it tilted: the top is at cell
# 10.5, row 8.5, the middle at 13.5, 17), so it leans along that axis; its collar lies across the axis, seated on the
# curve. Built in the axis's own frame: u along it (0 at the collar), v across it
ax, ay = -3.0, -8.5
n = np.hypot(ax, ay)
ax, ay = ax / n, ay / n
bx, by = 10.5, 8.6
dx, dy = cx - bx, cy - by
u = dx * ax + dy * ay
v = dx * -ay + dy * ax                                   # (+ to his left as we see it .. - to the right)
Lp, w0, w1 = 7.6, 2.9, 1.9
wid = w0 - (w0 - w1) * np.clip(u, 0, Lp) / Lp
plug = (u >= 0.4) & (u <= Lp) & (np.abs(v) <= wid)
collar = (u > -0.6) & (u < 0.9) & (np.abs(v) <= w0 + 0.7)
body = plug | collar
pad = np.pad(body, 1)
edge = body & ~(pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
put = body.copy()
cell[body] = FROST
cell[collar] = GLASS_D
cell[collar & (u > 0.2)] = GLASS                          # (the collar's upper face, catching light)
cell[plug & (v < -0.45 * wid)] = SHINE                    # lit down one side
cell[plug & (v > 0.5 * wid)] = FROST_D                    # in shade down the other
cell[plug & (np.abs(u - Lp * 0.62) < 0.5)] = FROST_D      # a ground-glass ring
cell[edge] = OUT
out = a.copy()
big = np.asarray(Image.fromarray(np.uint8(cell)).resize((GW * G, GH * G), Image.NEAREST))
h, w = min(GH * G, a.shape[0] - Y0), min(GW * G, a.shape[1] - X0)
reg = out[Y0:Y0 + h, X0:X0 + w]
mask = np.zeros((h, w), bool)
mask[:] = np.asarray(Image.fromarray(np.uint8(put | warm | (inside & (L > 215)))).resize((GW * G, GH * G), Image.NEAREST))[:h, :w] > 0
reg[mask] = big[:h, :w][mask]                            # (only the cells changed: the rest keeps its source pixels)
Image.fromarray(np.uint8(out)).save(ROOT / "art/gemini/mote_a_fix.png")
print("art/gemini/mote_a_fix.png")
