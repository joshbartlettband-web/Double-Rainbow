"""Double Rainbow effects-page mockups at the FM-1's 240 x 240: PRISMS (chosen) and POTIONS, with the same info
panel and knobs, and the two side by side at 3x. A design picture only: nothing here runs on the FM-1.
  python3 docs/mockups/effects_mockup.py [OUT_DIR]      (default: this folder; needs Pillow)"""
import math
import random
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
FONT = str(HERE.parents[1] / "assets" / "fonts" / "InterTight[wght].ttf")   # Melodee's screen font (SIL OFL 1.1)
OUT = sys.argv[1] if len(sys.argv) > 1 else str(HERE)


def font(px, wght):
    f = ImageFont.truetype(FONT, px)
    f.set_variation_by_axes([wght])
    return f


F_S, F_SB, F_M, F_L = font(10, 500), font(10, 650), font(13, 650), font(20, 700)

# Rainbow after dark
NIGHT0 = (16, 11, 34)
NIGHT1 = (30, 19, 56)
PLUM = (52, 28, 70)
PANEL = (26, 17, 44)
CARD = (38, 26, 62)
CARD_SEL = (66, 42, 96)
OUTLINE = (34, 18, 12)           # dark brown outline
TEXT = (246, 234, 210)           # warm white
DIM = (156, 140, 180)
GOLD = (255, 205, 96)
MOON = (243, 233, 198)
EMBER, EMBER_D, EMBER_L = (255, 122, 60), (186, 64, 30), (255, 196, 120)
TEAL, TEAL_D, TEAL_L = (63, 224, 197), (24, 128, 120), (170, 255, 236)
VIOLET, VIOLET_D, VIOLET_L = (166, 108, 255), (92, 54, 170), (214, 186, 255)
BLUE, BLUE_D, BLUE_L = (79, 140, 255), (38, 70, 160), (170, 206, 255)
WOOD, WOOD_D, WOOD_L = (122, 72, 40), (78, 44, 24), (170, 110, 62)

FX = [  # name, real name, colors, amount, line
    ("EMBER", "DISTORTION", (EMBER, EMBER_D, EMBER_L), 40),
    ("SHATTER", "STUTTER", (TEAL, TEAL_D, TEAL_L), 0),
    ("TWIN", "CHORUS", (VIOLET, VIOLET_D, VIOLET_L), 65),
    ("CAVERN", "REVERB", (BLUE, BLUE_D, BLUE_L), 50),
]
SEL = 2
INFO = {"TWIN": ["Copies your sound and lets the", "copies drift apart. Wide, dreamy,", "a little seasick if you push it."]}


def night(d, y0, y1, seed):
    for y in range(y0, y1):           # a stepped gradient, dithered at the steps
        t = (y - y0) / max(1, y1 - y0 - 1)
        c = tuple(int(NIGHT0[i] + (NIGHT1[i] - NIGHT0[i]) * t) for i in range(3))
        d.line([(0, y), (239, y)], fill=c)
    r = random.Random(seed)
    for _ in range(26):
        x, y = r.randrange(2, 238), r.randrange(y0 + 2, y1 - 30)
        b = r.choice([MOON, DIM, (120, 108, 150)])
        d.point((x, y), fill=b)
        if b == MOON and r.random() < 0.35:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                d.point((x + dx, y + dy), fill=(110, 96, 140))


def header(d, title):
    d.rectangle([0, 0, 239, 19], fill=NIGHT0)
    d.text((8, 3), title, font=F_M, fill=GOLD)
    t = "VESPER · PROPHET"
    w = d.textlength(t, font=F_S)
    d.text((232 - w, 5), t, font=F_S, fill=DIM)
    d.line([(0, 19), (239, 19)], fill=PLUM)


def info_panel(d):
    name, real, (c, cd, cl), amt = FX[SEL]
    y = 132
    d.rectangle([0, y, 239, 197], fill=PANEL)
    d.line([(0, y), (239, y)], fill=PLUM)
    d.text((8, y + 3), name, font=F_L, fill=cl)
    w = d.textlength(name, font=F_L)
    d.text((14 + w, y + 11), "real name:", font=F_S, fill=DIM)
    w2 = d.textlength("real name:", font=F_S)
    d.text((18 + w + w2, y + 10), real, font=F_SB, fill=TEXT)
    for i, line in enumerate(INFO[name]):
        d.text((8, y + 28 + i * 12), line, font=F_S, fill=TEXT if i < 2 else DIM)


def knob_cards(d):
    y = 200
    d.rectangle([0, y, 239, 239], fill=NIGHT0)
    for i, (name, real, (c, cd, cl), amt) in enumerate(FX):
        x = 3 + i * 59
        sel = i == SEL
        d.rectangle([x, y + 3, x + 55, y + 36], fill=CARD_SEL if sel else CARD, outline=cl if sel else PLUM)
        d.text((x + 5, y + 6), name, font=F_SB, fill=cl if sel else DIM)
        txt = f"{amt}%"
        d.text((x + 5, y + 17), txt, font=F_M, fill=TEXT if amt else DIM)
        d.rectangle([x + 5, y + 31, x + 50, y + 32], fill=PLUM)
        if amt:
            d.rectangle([x + 5, y + 31, x + 5 + int(45 * amt / 100), y + 32], fill=c)


# ------------------------------------------------------------------ PRISMS
def gem(d, cx, cy, cols, sel):
    c, cd, cl = cols
    top, mid, bot = cy - 16, cy - 4, cy + 16
    left, right = cx - 11, cx + 11
    if sel:                                  # a dithered halo
        for yy in range(top - 7, bot + 8):
            for xx in range(left - 8, right + 9):
                dd = math.hypot((xx - cx) / 1.0, (yy - cy) / 1.35)
                if 13 < dd < 20 and (xx + yy) % 2 == 0:
                    d.point((xx, yy), fill=cd)
    poly = [(cx - 6, top), (cx + 6, top), (right, mid), (cx, bot), (left, mid)]
    d.polygon(poly, fill=c, outline=OUTLINE)
    d.polygon([(cx - 6, top + 1), (cx, top + 1), (cx - 3, mid), (left + 1, mid)], fill=cl)         # lit facet
    d.polygon([(cx + 1, mid + 1), (right - 1, mid + 1), (cx, bot - 1)], fill=cd)                     # shadow facet
    d.line([(left + 1, mid), (right - 1, mid)], fill=cd)
    d.point((cx - 4, top + 3), fill=(255, 255, 240))
    d.point((cx - 5, top + 4), fill=(255, 255, 240))


def prisms():
    im = Image.new("RGB", (240, 240), NIGHT0)
    d = ImageDraw.Draw(im)
    night(d, 20, 132, 7)
    header(d, "PRISMS")
    by = 72
    xs = [36, 86, 136, 186]
    white = (250, 246, 232)
    # the beam, changing after each crystal
    d.line([(0, by), (xs[0] - 11, by)], fill=white, width=2)                       # clean
    x = xs[0] + 11
    pts = []                                                                         # EMBER: jagged, red-hot
    k = 0
    while x <= xs[1] - 11:
        pts.append((x, by + (-2 if k % 2 else 2)))
        x += 4
        k += 1
    d.line(pts, fill=EMBER_L, width=1)
    d.line([(p[0], p[1] + 1) for p in pts], fill=EMBER, width=1)
    x = xs[1] + 11                                                                   # SHATTER (0 %): passes clean
    d.line([(x, by), (xs[2] - 11, by)], fill=EMBER_L, width=2)
    x = xs[2] + 11                                                                   # TWIN: two drifting beams
    for i in range(xs[3] - 11 - x + 1):
        off = 3 + round(1.5 * math.sin((x + i) / 5))
        d.point((x + i, by - off), fill=VIOLET_L)
        d.point((x + i, by + off), fill=VIOLET_L)
        d.point((x + i, by - off + 1), fill=VIOLET)
        d.point((x + i, by + off - 1), fill=VIOLET)
    x = xs[3] + 11                                                                   # CAVERN: a soft glow, then the rainbow
    for i in range(0, 14):
        for dy in range(-5, 6):
            if abs(dy) <= 1 or (abs(dy) <= 4 and (x + i + dy) % 2 == 0):
                d.point((x + i, by + dy), fill=BLUE_L if abs(dy) <= 1 else BLUE_D)
    bands = [(255, 92, 92), (255, 160, 64), (255, 220, 90), (110, 220, 120), (90, 160, 255), (170, 110, 255)]
    x0 = x + 14
    for j, col in enumerate(bands):
        a = (j - 2.5) * 3.2
        for i in range(0, 240 - x0):
            yy = by + a * i / 7
            d.point((x0 + i, round(yy)), fill=col)
            d.point((x0 + i, round(yy) + 1), fill=col)
    for i, (name, real, cols, amt) in enumerate(FX):
        gem(d, xs[i], by, cols, i == SEL)
        w = d.textlength(name, font=F_SB)
        d.text((xs[i] - w / 2, by + 24), name, font=F_SB, fill=cols[2] if i == SEL else DIM)
        if i == SEL:
            d.polygon([(xs[i] - 3, by - 30), (xs[i] + 3, by - 30), (xs[i], by - 26)], fill=GOLD)
    d.text((8, 116), "SELECT picks a crystal   KNOBS set how much", font=F_S, fill=(120, 108, 150))
    info_panel(d)
    knob_cards(d)
    return im


# ------------------------------------------------------------------ POTIONS
def flask(d, kind, cx, base, cols, amt, sel, seed):
    c, cd, cl = cols
    glass = (190, 200, 230)
    r = random.Random(seed)
    if sel:                                   # candle-warm aura
        for yy in range(base - 62, base + 2):
            for xx in range(cx - 24, cx + 25):
                dd = math.hypot(xx - cx, (yy - (base - 26)) * 0.8)
                if 22 < dd < 27 and (xx + yy) % 2 == 0:
                    d.point((xx, yy), fill=cd)
    if kind == "round":                       # round-bottom flask
        body = [cx - 15, base - 30, cx + 15, base]
        neck = [cx - 4, base - 46, cx + 4, base - 28]
    elif kind == "tall":                      # a tall vial
        body = [cx - 7, base - 46, cx + 7, base]
        neck = [cx - 5, base - 52, cx + 5, base - 46]
    elif kind == "twin":                      # two bulbs, one neck: TWIN
        body = None
        neck = [cx - 4, base - 50, cx + 4, base - 34]
    else:                                     # a wide jar
        body = [cx - 15, base - 34, cx + 15, base]
        neck = [cx - 11, base - 40, cx + 11, base - 34]
    lvl = amt / 100
    if kind == "twin":
        bulbs = [(cx - 9, base - 12), (cx + 9, base - 12)]
        for bx, byy in bulbs:
            d.ellipse([bx - 11, byy - 12, bx + 11, byy + 12], fill=(40, 34, 70), outline=OUTLINE)
        d.rectangle(neck, fill=(40, 34, 70), outline=OUTLINE)
        top = base - 24 + int(24 * (1 - lvl)) if lvl else base + 1
        for bx, byy in bulbs:
            for yy in range(max(top, byy - 11), byy + 12):
                half = int(math.sqrt(max(0, 11 ** 2 - (yy - byy) ** 2)))
                if half > 1:
                    d.line([(bx - half + 1, yy), (bx + half - 1, yy)], fill=c if yy > top else cl)
        for bx, byy in bulbs:
            d.ellipse([bx - 11, byy - 12, bx + 11, byy + 12], outline=OUTLINE)
            d.point((bx - 6, byy - 6), fill=(255, 255, 240))
            d.point((bx - 5, byy - 7), fill=(255, 255, 240))
        d.rectangle(neck, outline=OUTLINE)
        d.rectangle([neck[0], neck[1] - 5, neck[2], neck[1]], fill=WOOD_L, outline=OUTLINE)   # cork
    else:
        if kind == "round":
            d.ellipse(body, fill=(40, 34, 70), outline=OUTLINE)
        else:
            d.rectangle(body, fill=(40, 34, 70), outline=OUTLINE)
        d.rectangle(neck, fill=(40, 34, 70), outline=OUTLINE)
        if lvl:
            top = body[1] + int((body[3] - body[1]) * (1 - lvl))
            for yy in range(top, body[3]):
                if kind == "round":
                    cy, rr = (body[1] + body[3]) / 2, (body[3] - body[1]) / 2
                    half = int(math.sqrt(max(0, rr ** 2 - (yy - cy) ** 2)))
                    xa, xb = cx - half + 1, cx + half - 1
                else:
                    xa, xb = body[0] + 1, body[2] - 1
                if xb > xa:
                    d.line([(xa, yy), (xb, yy)], fill=cl if yy == top else c)
            for _ in range(int(5 * lvl) + 1):  # bubbles
                bx = r.randrange(cx - 6, cx + 6)
                byy = r.randrange(top + 3, body[3] - 2) if body[3] - 2 > top + 3 else top + 2
                d.point((bx, byy), fill=cl)
        if kind == "round":
            d.ellipse(body, outline=OUTLINE)
        else:
            d.rectangle(body, outline=OUTLINE)
        d.rectangle(neck, outline=OUTLINE)
        d.rectangle([neck[0] - (1 if kind == "jar" else 0), neck[1] - 5, neck[2] + (1 if kind == "jar" else 0), neck[1]],
                    fill=WOOD_L, outline=OUTLINE)
        d.line([(body[0] + 3, body[1] + 4), (body[0] + 3, body[1] + 9)], fill=glass)    # glint
        if not lvl:
            d.text((cx - 5, (body[1] + body[3]) / 2 - 6), "...", font=F_S, fill=(110, 100, 140))


def potions():
    im = Image.new("RGB", (240, 240), NIGHT0)
    d = ImageDraw.Draw(im)
    night(d, 20, 132, 11)
    header(d, "POTIONS")
    for yy in range(21, 80):                  # a candle's warm light, top left: dense near it, sparse further
        for xx in range(0, 62):
            dd = math.hypot(xx - 14, yy - 34)
            if dd < 16 and (xx + yy) % 2 == 0 or 16 <= dd < 30 and xx % 2 == 0 and yy % 2 == 0 \
                    or 30 <= dd < 42 and xx % 4 == 0 and yy % 4 == 0:
                d.point((xx, yy), fill=(82, 44, 58) if dd < 16 else (64, 36, 60))
    d.rectangle([11, 36, 17, 50], fill=MOON, outline=OUTLINE)              # the candle
    d.polygon([(14, 27), (17, 33), (14, 35), (11, 33)], fill=GOLD, outline=EMBER_D)
    d.point((14, 31), fill=(255, 250, 220))
    base = 100
    d.rectangle([0, base + 1, 239, base + 6], fill=WOOD, outline=OUTLINE)  # the shelf
    d.line([(1, base + 2), (238, base + 2)], fill=WOOD_L)
    for x in (30, 210):
        d.polygon([(x - 3, base + 7), (x + 3, base + 7), (x, base + 13)], fill=WOOD_D, outline=OUTLINE)
    kinds = ["round", "tall", "twin", "jar"]
    xs = [36, 86, 136, 192]
    for i, (name, real, cols, amt) in enumerate(FX):
        flask(d, kinds[i], xs[i], base, cols, amt, i == SEL, i)
        w = d.textlength(name, font=F_SB)
        tx, ty = xs[i] - w / 2, base + 15
        if i == SEL:
            d.rectangle([tx - 3, ty - 1, tx + w + 3, ty + 11], fill=(238, 222, 184), outline=OUTLINE)   # a parchment tag
            d.text((tx, ty), name, font=F_SB, fill=OUTLINE)
        else:
            d.text((tx, ty), name, font=F_SB, fill=DIM)
    info_panel(d)
    knob_cards(d)
    return im


def board(a, b, scale=3):
    pad, top = 24, 54
    W = 2 * 240 * scale + 3 * pad
    H = 240 * scale + top + pad + 30
    out = Image.new("RGB", (W, H), (12, 9, 22))
    d = ImageDraw.Draw(out)
    big = font(30, 700)
    sub = font(18, 500)
    for i, (im, label) in enumerate(((a, "PRISMS"), (b, "POTIONS"))):
        x = pad + i * (240 * scale + pad)
        d.rectangle([x - 4, top - 4, x + 240 * scale + 3, top + 240 * scale + 3], fill=(4, 3, 8))
        out.paste(im.resize((240 * scale, 240 * scale), Image.NEAREST), (x, top))
        d.text((x, 12), label, font=big, fill=GOLD)
    d.text((pad, top + 240 * scale + 12), "Double Rainbow effects page, mockup at the FM-1's 240 x 240 (shown 3x). "
           "Same info panel and knobs; only the picture differs.", font=sub, fill=DIM)
    return out


a, b = prisms(), potions()
a.save(f"{OUT}/prisms_240.png")
b.save(f"{OUT}/potions_240.png")
board(a, b).save(f"{OUT}/effects_mockups.png")
print("ok")
