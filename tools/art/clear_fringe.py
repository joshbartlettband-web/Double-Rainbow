#!/usr/bin/env python3
"""Clear the last of a Gemini plate's magenta background from a sprite (art/sprites/NAME.png): the pinkish-purple
pixels left in gaps (between an arm and a body, say) where the key colour had been mixed in. They become clear.
  python3 tools/art/clear_fringe.py NAME ..."""
import sys
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
for name in sys.argv[1:]:
    p = ROOT / "art" / "sprites" / f"{name}.png"
    a = np.array(Image.open(p).convert("RGBA"))
    r, g, b = (a[..., i].astype(int) for i in range(3))
    bad = (a[..., 3] > 0) & (r > g + 90) & (b > g + 60) & (r > 110) & (b > 90)
    a[bad] = (0, 0, 0, 0)
    Image.fromarray(a).save(p)
    big = p.with_name(f"{name}_8x.png")
    if big.exists():
        Image.fromarray(a).resize((a.shape[1] * 8, a.shape[0] * 8), Image.NEAREST).save(big)
    print(f"{name}: {int(bad.sum())} fringe pixels cleared")
