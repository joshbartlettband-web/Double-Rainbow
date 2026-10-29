"""Each character's eyes, for the blink (firmware/src/dr.c): boxes in sprite pixels, x0 y0 x1 y1 inclusive, read off
the sprites by hand (tools/art/make_sprites.sh rebuilds the sprites at the same size, so they stay put). Tock has none:
his eye is a clock. gen_dr_art.py packs them; eyes_preview() draws every face open and shut to check them."""
EYES = {
    "vesper": [(21, 28, 27, 32), (32, 28, 39, 32)],
    "shard": [(12, 31, 18, 35), (29, 31, 35, 35)],
    "squall": [(16, 37, 30, 42), (40, 37, 55, 42)],
    "wick": [(12, 45, 17, 50), (23, 45, 29, 50)],
    "tock": [],
    "mote": [(35, 45, 46, 55), (56, 46, 64, 55)],
    "murk": [(40, 27, 53, 34), (60, 27, 69, 34)],
    "bog": [(39, 16, 48, 25), (67, 15, 73, 23)],
    "hush": [(40, 16, 48, 23), (58, 16, 63, 23)],
    "tumbler": [(15, 19, 22, 23), (26, 19, 35, 23)],
    "glint": [(68, 7, 73, 12)],
    "hermit": [(26, 46, 31, 52), (36, 46, 41, 52)],
    "burrow": [(52, 15, 60, 22)],
    "pip": [(38, 23, 44, 29)],
    "echo": [(5, 20, 10, 26), (18, 20, 23, 26)],
    "fork": [(21, 43, 26, 51), (31, 43, 37, 51)],
    "ants": [(56, 25, 61, 31), (14, 54, 19, 59)],
    "rabbit": [(20, 40, 28, 45)],
    "top": [(27, 25, 33, 31), (38, 23, 44, 29)],
    "cairn": [(11, 29, 17, 36), (34, 29, 41, 36)],
    "stray": [],
    "tuppence": [(22, 25, 27, 31)],
    "gloam": [(36, 34, 37, 38), (58, 34, 59, 38)],
    "tinker": [(45, 34, 50, 40), (55, 34, 60, 40)],
}
OUTLINE = (34, 18, 12)
LID = {"hermit": (231, 89, 58)}          # by hand where the colour round the eye is not the lid (Hermit: his horn)


def shut(idx, w, h, pal, boxes, lid_rgb=None):
    """the eyes shut, as the firmware draws them: inside each box every pixel but the figure's own edge becomes the lid
    (the commonest colour just around the box), and a line of outline across its middle row"""
    from collections import Counter
    out = list(idx)
    lids = []
    for x0, y0, x1, y1 in boxes:
        ring = Counter()
        for y in range(max(0, y0 - 2), min(h, y1 + 3)):
            for x in range(max(0, x0 - 2), min(w, x1 + 3)):
                if (x0 <= x <= x1 and y0 <= y <= y1) or not idx[y * w + x]:
                    continue
                if pal[idx[y * w + x] - 1] != OUTLINE:
                    ring[idx[y * w + x]] += 1
        lid = ring.most_common(1)[0][0] if ring else 1
        if lid_rgb is not None and lid_rgb in pal:
            lid = pal.index(lid_rgb) + 1
        lids.append(lid)
        mid = (y0 + y1 + 1) // 2
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                v = idx[y * w + x]
                edge = any(not (0 <= x + dx < w and 0 <= y + dy < h) or not idx[(y + dy) * w + x + dx]
                           for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
                if v and (pal[v - 1] != OUTLINE or not edge):    # (a pupil in the outline's colour goes too)
                    out[y * w + x] = lid
        for x in range(x0, x1 + 1):
            if idx[mid * w + x]:
                out[mid * w + x] = 1 + pal.index(OUTLINE) if OUTLINE in pal else 1
    return out, lids


def eyes_preview(out_png):
    from pathlib import Path
    from PIL import Image
    root = Path(__file__).resolve().parents[2]
    tiles = []
    for name, boxes in EYES.items():
        im = Image.open(root / "art" / "sprites" / f"{name}.png").convert("RGBA")
        w, h = im.size
        px = list(im.getdata())
        pal, idx = [], []
        for p in px:
            if p[3] < 128:
                idx.append(0)
                continue
            if p[:3] not in pal:
                pal.append(p[:3])
            idx.append(pal.index(p[:3]) + 1)
        closed, _ = shut(idx, w, h, pal, boxes)
        for data in (idx, closed):
            t = Image.new("RGB", (w, h), (16, 11, 34))
            t.putdata([pal[v - 1] if v else (16, 11, 34) for v in data])
            ys = [b[1] for b in boxes] or [10]
            cy = max(0, min(ys) - 14)
            tiles.append(t.crop((0, cy, w, min(h, cy + 40))).resize((w * 3, 120), Image.NEAREST))
    W = 6 * 300
    sheet = Image.new("RGB", (W, 4 * 130), (0, 0, 0))
    for i, t in enumerate(tiles):
        r, c = divmod(i // 2, 6)
        sheet.paste(t.crop((0, 0, 145, 120)) if t.width > 145 else t, (c * 300 + (i % 2) * 148, r * 130))
    sheet.save(out_png)


if __name__ == "__main__":
    import sys
    eyes_preview(sys.argv[1])
