#!/usr/bin/env python3
"""The cast drawn from shapes in code, to compare with the painted sprites: each character is a few ellipses and
polygons drawn at 8x, then snapped to pixels (the most common colour in each 8x8 block) and outlined in the shared
dark brown. -> docs/mockups/code_cast.png (top: the sprites in the firmware; below: drawn in code)."""
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
S = 8                                                    # drawn at 8x, then snapped
OUT, BG = (34, 18, 12), (16, 11, 34)


def snap(im):
    a = np.asarray(im)
    h, w = a.shape[0] // S, a.shape[1] // S
    out = np.zeros((h, w, 4), np.uint8)
    for y in range(h):
        for x in range(w):
            blk = a[y * S:(y + 1) * S, x * S:(x + 1) * S].reshape(-1, 4)
            if (blk[:, 3] > 0).sum() < S * S // 2:
                continue
            blk = blk[blk[:, 3] > 0]
            cols, n = np.unique(blk, axis=0, return_counts=True)
            out[y, x] = cols[n.argmax()]
    solid = out[..., 3] > 0
    pad = np.pad(solid, 1)
    edge = solid & ~(pad[:-2, 1:-1] & pad[2:, 1:-1] & pad[1:-1, :-2] & pad[1:-1, 2:])
    out[edge] = (*OUT, 255)
    return Image.fromarray(out)


def canvas(w, h):
    im = Image.new("RGBA", (w * S, h * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    P = lambda pts: [(x * S, y * S) for x, y in pts]
    E = lambda x0, y0, x1, y1: (x0 * S, y0 * S, x1 * S, y1 * S)
    return im, d, P, E


def vesper():
    im, d, P, E = canvas(100, 112)
    plum, plum_d, silver, cream, fur, cape, cape_d, amber, moon = (122, 70, 110), (84, 44, 82), (196, 178, 196), \
        (232, 222, 214), (246, 240, 236), (150, 36, 72), (104, 22, 52), (255, 186, 60), (240, 232, 200)
    for sx in (-1, 1):                                   # the wings, behind
        cx = 50
        d.polygon(P([(cx, 46), (cx + sx * 46, 30), (cx + sx * 48, 60), (cx + sx * 30, 92), (cx, 70)]), fill=plum, outline=OUT, width=S)
        d.polygon(P([(cx + sx * 8, 52), (cx + sx * 40, 38), (cx + sx * 40, 60), (cx + sx * 26, 84)]), fill=plum_d)
        d.ellipse(E(cx + sx * 30 - 5, 52, cx + sx * 30 + 5, 62), fill=moon)
        d.ellipse(E(cx + sx * 30 - 3 + sx * 2, 51, cx + sx * 30 + 7 + sx * 2 - 4, 60), fill=plum_d)   # a crescent
        for k in range(3):
            d.ellipse(E(cx + sx * (16 + 8 * k) - 1, 72 - 4 * k, cx + sx * (16 + 8 * k) + 1, 74 - 4 * k), fill=silver)
    d.polygon(P([(50, 40), (30, 104), (70, 104)]), fill=cape, outline=OUT, width=S)          # the velvet cape
    d.polygon(P([(50, 48), (40, 104), (52, 104)]), fill=cape_d)
    d.ellipse(E(42, 58, 58, 100), fill=cream, outline=OUT, width=S)                          # the body
    for y in (66, 74, 82, 90):
        d.line(P([(44, y), (56, y)]), fill=silver, width=S)
    d.ellipse(E(32, 36, 68, 62), fill=fur, outline=OUT, width=S)                             # the fluffy collar
    for x in range(34, 68, 6):
        d.ellipse(E(x - 3, 56, x + 3, 64), fill=fur)
    d.ellipse(E(38, 20, 62, 46), fill=cream, outline=OUT, width=S)                           # the head
    d.ellipse(E(40, 28, 49, 37), fill=amber, outline=OUT, width=S)                           # the lantern eyes
    d.ellipse(E(51, 28, 60, 37), fill=amber, outline=OUT, width=S)
    d.ellipse(E(43, 30, 46, 33), fill=(255, 240, 180))
    d.ellipse(E(54, 30, 57, 33), fill=(255, 240, 180))
    for sx in (-1, 1):                                   # the feathered antennae
        d.line(P([(50 + sx * 5, 22), (50 + sx * 16, 4)]), fill=OUT, width=2 * S)
        for k in range(4):
            y, x = 18 - 4 * k, 50 + sx * (7 + 2.5 * k)
            d.line(P([(x, y), (x + sx * 4, y - 3)]), fill=silver, width=S)
    d.ellipse(E(36, 100, 44, 108), fill=cream, outline=OUT, width=S)                         # feet
    d.ellipse(E(56, 100, 64, 108), fill=cream, outline=OUT, width=S)
    return snap(im)


def shard():
    im, d, P, E = canvas(84, 108)
    g1, g2, g3, g4, hi = (150, 190, 232), (176, 206, 240), (128, 160, 214), (196, 196, 236), (226, 244, 252)
    RB = [(236, 72, 72), (250, 150, 50), (250, 222, 70), (90, 206, 96), (70, 140, 240), (156, 96, 230)]
    tail = [(46, 74), (70, 40), (82, 52), (80, 76), (64, 92), (48, 88)]                     # the tail, behind
    d.polygon(P(tail), fill=g2, outline=OUT, width=S)
    d.polygon(P([(70, 40), (82, 52), (66, 70)]), fill=g4)
    d.polygon(P([(66, 70), (80, 76), (64, 92)]), fill=g3)
    d.polygon(P([(52, 76), (66, 70), (64, 92), (48, 88)]), fill=hi)
    d.polygon(P([(26, 50), (46, 50), (48, 84), (24, 84)]), fill=g1, outline=OUT, width=S)   # the body
    d.polygon(P([(26, 50), (36, 50), (30, 84), (24, 84)]), fill=g2)
    d.polygon(P([(36, 62), (46, 56), (48, 72)]), fill=g3)
    for i, c in enumerate(RB):                           # the spectrum low on his belly, bowing with it
        d.line(P([(30 + 1.6 * i, 68), (32 + 1.6 * i, 73), (36 + 1.6 * i, 78)]), fill=c, width=2 * S, joint="curve")
    for x0 in (26, 38):                                  # legs
        d.polygon(P([(x0, 82), (x0 + 8, 82), (x0 + 7, 104), (x0 + 1, 104)]), fill=g3, outline=OUT, width=S)
    d.polygon(P([(26, 52), (16, 70), (22, 72), (30, 58)]), fill=g2, outline=OUT, width=S)   # arms, hand on hip
    d.polygon(P([(46, 52), (56, 66), (48, 70), (44, 60)]), fill=g2, outline=OUT, width=S)
    head = [(10, 30), (22, 20), (44, 20), (56, 30), (44, 42), (34, 50), (22, 44)]
    d.polygon(P(head), fill=g1, outline=OUT, width=S)
    d.polygon(P([(22, 20), (44, 20), (34, 32)]), fill=hi)
    d.polygon(P([(10, 30), (22, 44), (34, 50), (24, 34)]), fill=g2)
    d.polygon(P([(44, 42), (56, 30), (40, 32)]), fill=g3)
    d.polygon(P([(18, 22), (16, 2), (30, 20)]), fill=g4, outline=OUT, width=S)              # the ears
    d.polygon(P([(36, 20), (48, 2), (50, 24)]), fill=g4, outline=OUT, width=S)
    d.polygon(P([(20, 18), (18, 8), (26, 18)]), fill=g3)
    d.polygon(P([(40, 18), (47, 8), (47, 20)]), fill=g3)
    d.line(P([(18, 29), (25, 30)]), fill=OUT, width=2 * S)                                    # sharp eyes
    d.line(P([(33, 30), (41, 29)]), fill=OUT, width=2 * S)
    d.ellipse(E(29, 38, 33, 42), fill=OUT)                                                    # the nose
    return snap(im)


def squall():
    im, d, P, E = canvas(100, 96)
    c1, c2, c3, c4, glow, glow2 = (92, 100, 140), (122, 132, 172), (66, 70, 106), (150, 160, 196), (255, 236, 150), \
        (250, 204, 56)
    puffs = [(14, 30, 46, 62), (30, 14, 66, 50), (54, 20, 88, 54), (8, 44, 40, 72), (66, 40, 96, 70), (28, 40, 76, 76)]
    for e in puffs:
        d.ellipse(E(*e), fill=c1, outline=OUT, width=S)
    for e in puffs:                                      # (then the inside, over the inner outlines)
        x0, y0, x1, y1 = e
        d.ellipse(E(x0 + 1, y0 + 1, x1 - 1, y1 - 1), fill=c1)
    d.ellipse(E(34, 18, 60, 36), fill=c4)                # the lit tops
    d.ellipse(E(18, 34, 38, 46), fill=c2)
    d.ellipse(E(60, 24, 82, 36), fill=c2)
    d.ellipse(E(20, 60, 84, 76), fill=c3)                # the dark underside
    d.polygon(P([(32, 46), (46, 50), (44, 54), (30, 50)]), fill=glow, outline=OUT, width=S)   # the eyes, sulking
    d.polygon(P([(70, 46), (56, 50), (58, 54), (72, 50)]), fill=glow, outline=OUT, width=S)
    d.line(P([(44, 62), (52, 60), (60, 62)]), fill=OUT, width=2 * S)                          # the frown
    bolt = [(66, 66), (78, 66), (72, 78), (80, 78), (62, 96), (68, 82), (60, 82)]
    d.polygon(P(bolt), fill=glow2, outline=OUT, width=S)
    d.polygon(P([(68, 68), (74, 68), (69, 78)]), fill=glow)
    return snap(im)


def main():
    painted = [Image.open(ROOT / "art" / "sprites" / f"{n}.png").convert("RGBA") for n in ("vesper", "shard", "squall")]
    drawn = [vesper(), shard(), squall()]
    for n, im in zip(("vesper", "shard", "squall"), drawn):
        im.save(ROOT / "docs" / "mockups" / f"code_{n}.png")
    Z, cw, ch = 3, 130, 130
    sheet = Image.new("RGBA", (3 * cw * Z, 2 * ch * Z), (*BG, 255))
    for row, ims in enumerate((painted, drawn)):
        for i, im in enumerate(ims):
            big = im.resize((im.width * Z, im.height * Z), Image.NEAREST)
            sheet.alpha_composite(big, (i * cw * Z + (cw * Z - big.width) // 2, row * ch * Z + (ch * Z - big.height) // 2))
    sheet.convert("RGB").save(ROOT / "docs" / "mockups" / "code_cast.png")
    print("docs/mockups/code_cast.png")


if __name__ == "__main__":
    main()
