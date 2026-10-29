#!/usr/bin/env python3
"""Hollow's eyes: their gold is the cork's highlight too, so the glow (gen_dr_art.py GLOW) would light the cork. The
eyes (rows 45 and down) get a gold of their own; the two nearest other colours merge to keep 15."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
EYE, NEW = (195, 154, 90), (250, 200, 104)
p = ROOT / "art" / "sprites" / "hollow.png"
im = Image.open(p).convert("RGBA")
px = im.load()
cols = sorted({px[x, y][:3] for y in range(im.height) for x in range(im.width) if px[x, y][3]})
if len(cols) >= 15:                                        # merge the closest pair (not the eye's gold)
    pairs = [(sum((a[i] - b[i]) ** 2 for i in range(3)), a, b) for a in cols for b in cols if a < b and EYE not in (a, b)]
    _, a, b = min(pairs)
    for y in range(im.height):
        for x in range(im.width):
            if px[x, y][3] and px[x, y][:3] == b:
                px[x, y] = a + (255,)
for y in range(45, im.height):
    for x in range(im.width):
        if px[x, y][3] and px[x, y][:3] == EYE:
            px[x, y] = NEW + (255,)
im.save(p)
im.resize((im.width * 8, im.height * 8), Image.NEAREST).save(p.with_name("hollow_8x.png"))
print("hollow: its eyes their own gold")
