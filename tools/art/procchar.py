#!/usr/bin/env python3
"""A Gemini character concept (art/gemini/*.jpg, on a flat background) -> an FM-1 sprite.
  python3 tools/art/procchar.py SRC.jpg NAME [MAX_H] [MAX_W] [KEY_TOL] [DETAIL_COLOURS]
      -> art/sprites/NAME.png (+ NAME_8x.png to look at)
Keys the background (the border's colour, only where it touches the border, so the same colour inside the figure
stays), drops specks, scales into MAX_H x MAX_W with alpha-weighted area averaging (no magenta fringe), cuts the
colours to 15 (index 0 is clear: 4 bits a pixel; small vivid details keep their own colours), and redraws the silhouette's edge in the dark brown outline every
Double Rainbow sprite shares. Prints the 4-bit run-length size the firmware would store."""
import sys
from pathlib import Path
import numpy as np
from PIL import Image
from scipy import ndimage as ndi

ROOT = Path(__file__).resolve().parents[2]
OUTLINE = (34, 18, 12)                      # the shared dark brown outline
NCOL = 15                                   # + clear = 16: 4 bits a pixel


def key(a, tol):
    h, w, _ = a.shape
    border = np.concatenate([a[0], a[-1], a[:, 0], a[:, -1]])
    bg = np.median(border, axis=0)
    near = np.abs(a - bg).sum(axis=2) < tol
    lab, _ = ndi.label(near)
    edge = set(np.unique(np.concatenate([lab[0], lab[-1], lab[:, 0], lab[:, -1]]))) - {0}
    fg = ~np.isin(lab, list(edge))
    fg = ndi.binary_opening(fg, iterations=1)
    lab, n = ndi.label(fg)
    sizes = ndi.sum(fg, lab, range(1, n + 1))
    keep = [i + 1 for i in range(n) if sizes[i] >= 150]     # specks (and the corner sparkle) go, raindrops stay
    return np.isin(lab, keep), bg


def shrink(a, m, size):
    """alpha-weighted area average: the background's colour never bleeds into the edge"""
    al = m.astype(np.float32)
    rgb = Image.fromarray(np.uint8(a * al[..., None]))
    pre = np.asarray(rgb.resize(size, Image.BOX)).astype(np.float32)
    alp = np.asarray(Image.fromarray(np.uint8(al * 255)).resize(size, Image.BOX)).astype(np.float32) / 255
    col = pre / np.maximum(alp[..., None], 1e-3)
    return np.clip(col, 0, 255), alp


def rle4(idx):
    """runs of one index: a byte (index << 4 | run - 1, runs of 1..16) per run, row-major"""
    flat, n, i = idx.ravel(), 0, 0
    while i < len(flat):
        j = i
        while j < len(flat) and flat[j] == flat[i] and j - i < 16:
            j += 1
        n += 1
        i = j
    return n


def main():
    src, name = sys.argv[1], sys.argv[2]
    mh = int(sys.argv[3]) if len(sys.argv) > 3 else 110
    mw = int(sys.argv[4]) if len(sys.argv) > 4 else 120
    tol = int(sys.argv[5]) if len(sys.argv) > 5 else 90
    ndet = int(sys.argv[6]) if len(sys.argv) > 6 else 4      # colours kept for small vivid details
    a = np.asarray(Image.open(src).convert("RGB")).astype(np.float32)
    m, bg = key(a, tol)
    ys, xs = np.where(m)
    a, m = a[ys.min():ys.max() + 1, xs.min():xs.max() + 1], m[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    s = min(mh / m.shape[0], mw / m.shape[1])
    size = (max(1, round(m.shape[1] * s)), max(1, round(m.shape[0] * s)))
    col, alp = shrink(a, m, size)
    solid = alp >= 0.5
    edge = solid & ~ndi.binary_erosion(solid, structure=np.ones((3, 3)), border_value=0)
    col[edge] = OUTLINE
    # the colours: OUTLINE, the figure's own (median cut), then up to DETAIL_COLOURS for small vivid details that
    # pick missed (glowing eyes, a rainbow): the pixels far from every colour so far, one colour per hue band
    px = col[solid & ~edge]

    def cut(p, k):
        q = Image.fromarray(np.uint8(p[None, :, :])).quantize(k, method=Image.Quantize.MEDIANCUT).getpalette()[:3 * k]
        return [q[i:i + 3] for i in range(0, len(q), 3)]
    pal = np.array([OUTLINE] + cut(px, NCOL - 1 - ndet), dtype=np.float32)
    far = px[((px[:, None, :] - pal[None]) ** 2).sum(-1).min(-1) > 38 ** 2]
    if len(far) >= 3:                       # one colour per hue band (30 degrees), the most used bands first
        hsv = np.asarray(Image.fromarray(np.uint8(far[None])).convert("HSV"))[0].astype(int)
        band = hsv[:, 0] * 12 // 256
        counts = np.bincount(band, minlength=12)
        det = [far[band == b].mean(0) for b in np.argsort(-counts)[:ndet] if counts[b] >= 3]
        if det:
            pal = np.concatenate([pal, np.array(det, dtype=np.float32)])
    d = ((col[..., None, :] - pal[None, None]) ** 2).sum(-1)
    idx = d.argmin(-1) + 1
    idx[edge] = 1
    idx[~solid] = 0
    out = np.zeros((size[1], size[0], 4), np.uint8)
    out[..., :3] = pal[np.maximum(idx - 1, 0)].astype(np.uint8)
    out[..., 3] = (idx > 0) * 255
    od = ROOT / "art" / "sprites"
    od.mkdir(parents=True, exist_ok=True)
    im = Image.fromarray(out)
    im.save(od / f"{name}.png")
    im.resize((size[0] * 8, size[1] * 8), Image.NEAREST).save(od / f"{name}_8x.png")
    print(f"{name}: {size[0]} x {size[1]} px, {len(np.unique(idx)) - 1} colours, background {bg.astype(int)}, "
          f"{len(far)} px of vivid detail, 4-bit RLE {rle4(idx)} B (+ 45 B palette), raw 4-bit {(size[0] * size[1] + 1) // 2} B")


if __name__ == "__main__":
    main()
