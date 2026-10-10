#!/usr/bin/env python3
"""Shard's rainbow, redrawn on the sprite (after procchar.py, tools/art/make_sprites.sh). The flat block of bands on
his chest becomes glass again: each pixel takes the glass colour whose brightness matches the source picture's
there, so the chest's facets come back. Then the light inside the glass: a smaller spectrum low on his belly whose
bands bow round his front, so it follows his shape instead of lying flat on it. Reads and writes art/sprites/shard.png (and the 8x preview).
  fix_shard.py [CX CY R]   the spectrum's centre and half-length, in sprite pixels"""
import sys
from pathlib import Path
import numpy as np
from PIL import Image
sys.path.insert(0, str(Path(__file__).resolve().parent))
from procchar import key, shrink, RAINBOW, OUTLINE   # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
cx, cy, r = (float(v) for v in sys.argv[1:4]) if len(sys.argv) > 3 else (27.0, 70.0, 11.0)

p = ROOT / "art" / "sprites" / "shard.png"
a = np.asarray(Image.open(p).convert("RGBA")).copy()
rgb, solid = a[..., :3].astype(int), a[..., 3] > 0
band = np.zeros(solid.shape, bool)
for c in RAINBOW:
    band |= (rgb == c).all(-1)
outline = (rgb == OUTLINE).all(-1)
glass = solid & ~band & ~outline

# the source at the sprite's size (as procchar.py made it): its brightness gives the facets back
s = np.asarray(Image.open(ROOT / "art/gemini/shard_c.jpg").convert("RGB")).astype(np.float32)
m, _ = key(s, 90)
ys, xs = np.where(m)
s, m = s[ys.min():ys.max() + 1, xs.min():xs.max() + 1], m[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
col, _ = shrink(s, m, (a.shape[1], a.shape[0]))
Ls = col.mean(-1)
pal = sorted({tuple(c) for c in rgb[glass]}, key=lambda c: sum(c))
pl = np.array([sum(c) / 3 for c in pal])
q = Ls[band]                                             # its range stretched to the glass's
q = (q - np.percentile(q, 5)) / max(1.0, np.percentile(q, 95) - np.percentile(q, 5))
q = pl.min() + np.clip(q, 0, 1) * (pl.max() - pl.min())
rgb[band] = np.array(pal)[np.abs(q[:, None] - pl[None]).argmin(1)]

# the spectrum: bands along a gentle curve (a circle centred far up to his left), inside a lens on the belly
yy, xx = np.mgrid[0:solid.shape[0], 0:solid.shape[1]]
BX, BY = cx + 34.0, cy - 30.0
d = np.hypot(xx - BX, yy - BY) - np.hypot(cx - BX, cy - BY)
lens = ((xx - cx) / r) ** 2 + ((yy - cy) / (r * 0.62)) ** 2 < 1
spec = solid & ~outline & lens & (np.abs(d) < 6)
k = np.clip(((d + 6) / 2).astype(int), 0, 5)
for b in range(6):
    rgb[spec & (k == b)] = RAINBOW[b]
a[..., :3] = rgb
Image.fromarray(a).save(p)
Image.fromarray(a).resize((a.shape[1] * 8, a.shape[0] * 8), Image.NEAREST).save(p.with_name("shard_8x.png"))
print(f"shard: the chest's facets back, a spectrum at ({cx:g}, {cy:g}); {len({tuple(c) for c in rgb[solid]})} colours")
