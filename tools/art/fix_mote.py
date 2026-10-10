#!/usr/bin/env python3
"""Mote's marble becomes a glass jar stopper, without another Gemini image. The work is done on the source's own
pixel grid (11 source pixels a cell). The ball stays as the stopper's round knob, its swirl cleared to plain glass;
the stopper's tapered plug (frosted, as ground glass is) rises from it, so he holds the stopper up upside down, like
a prize. Reads art/gemini/mote_a.jpg, writes art/gemini/mote_a_fix.png."""
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

over = Image.new("RGBA", (GW, GH), (0, 0, 0, 0))
d = ImageDraw.Draw(over)
d.polygon([(9, 10), (15, 10), (14, 0), (10, 0)], fill=OUT)                    # the plug, outlined: wide at the knob,
d.polygon([(10, 9), (14, 9), (13, 1), (11, 1)], fill=FROST)                    # narrowing to its end; frosted
d.line([(10, 8), (11, 2)], fill=SHINE)                                         # (centred on cell 12: the ball's middle)
d.line([(14, 8), (13, 2)], fill=FROST_D)
d.line([(10, 6), (14, 6)], fill=FROST_D)                                       # a ground-glass ring
d.line([(10, 9), (14, 9)], fill=GLASS_D)                                       # the neck, clear glass
ov = np.asarray(over)
put = ov[..., 3] > 0
cell[put] = ov[..., :3][put]
out = a.copy()
big = np.asarray(Image.fromarray(np.uint8(cell)).resize((GW * G, GH * G), Image.NEAREST))
h, w = min(GH * G, a.shape[0] - Y0), min(GW * G, a.shape[1] - X0)
reg = out[Y0:Y0 + h, X0:X0 + w]
mask = np.zeros((h, w), bool)
mask[:] = np.asarray(Image.fromarray(np.uint8(put | warm | (inside & (L > 215)))).resize((GW * G, GH * G), Image.NEAREST))[:h, :w] > 0
reg[mask] = big[:h, :w][mask]                            # (only the cells changed: the rest keeps its source pixels)
Image.fromarray(np.uint8(out)).save(ROOT / "art/gemini/mote_a_fix.png")
print("art/gemini/mote_a_fix.png")
