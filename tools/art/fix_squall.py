#!/usr/bin/env python3
"""Squall's colours, fixed in the source without another Gemini image: the eye-whites (brighter than anything on
the cloud, inside two boxes) a pale glowing yellow, the lightning bolt (the box at the lower right) three yellows by
its own light. Reads art/gemini/squall_c.jpg, writes art/gemini/squall_c_fix.png (tools/art/make_sprites.sh)."""
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
a = np.asarray(Image.open(ROOT / "art/gemini/squall_c.jpg").convert("RGB")).astype(np.int32)
L = a.mean(2)
magenta = (a[..., 0] > 200) & (a[..., 1] < 90) & (a[..., 2] > 200)
out = a.copy()
yy, xx = np.mgrid[0:a.shape[0], 0:a.shape[1]]
for x0, x1, y0, y1 in ((255, 380, 420, 500), (450, 595, 420, 500)):      # the eyes (source pixels)
    box = (xx >= x0) & (xx < x1) & (yy >= y0) & (yy < y1) & ~magenta
    out[box & (L >= 165)] = (255, 236, 150)                                  # the whites: a glow
    out[box & (L >= 148) & (L < 165)] = (226, 182, 92)                       # their soft edge
bolt = (xx >= 712) & (xx < 912) & (yy >= 642) & (yy < 812) & ~magenta & (L >= 60)
for lo, hi, c in ((60, 110, (206, 140, 26)), (110, 150, (250, 204, 56)), (150, 256, (255, 240, 150))):
    out[bolt & (L >= lo) & (L < hi)] = c
Image.fromarray(np.uint8(out)).save(ROOT / "art/gemini/squall_c_fix.png")
print("art/gemini/squall_c_fix.png")
