"""Double Rainbow character screen mockups at the FM-1's 240 x 240: the three first characters, Vesper's
"under the knobs" view (EDIT held) and her codex page; and the five side by side at 2x. A design picture only.
  python3 docs/mockups/characters_mockup.py [OUT_DIR]      (default: this folder; needs Pillow and art/sprites/*.png)"""
import math
import random
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
FONT = str(ROOT / "assets" / "fonts" / "InterTight[wght].ttf")
OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE


def font(px, wght):
    f = ImageFont.truetype(FONT, px)
    f.set_variation_by_axes([wght])
    return f


F_XS, F_S, F_SB, F_M, F_L = font(9, 600), font(11, 500), font(11, 650), font(14, 650), font(22, 700)
BG, SURF, TEXT, GOLD, TEAL = (16, 11, 34), (38, 26, 62), (246, 234, 210), (255, 205, 96), (63, 224, 197)
DIM, MIDC, PLUM, GROUND = (120, 106, 150), (176, 162, 196), (58, 38, 78), (18, 11, 28)
BOW = [(160, 112, 230), (100, 150, 236), (120, 206, 140), (240, 214, 120), (240, 160, 96), (232, 96, 104)]

CAST = {
    "vesper": dict(name="VESPER", engine="PROPHET", line="Every note is her final scene.", knobs=[
        ("GLOW", 62, "CUTOFF"), ("BITE", 30, "RESO + ENV AMT"), ("BLOOM", 45, "ATTACK + RELEASE"),
        ("DRIFT", 20, "VINTAGE + VIBRATO")]),
    "shard": dict(name="SHARD", engine="FM6", line="Sharp, clear, honest to a fault.", knobs=[
        ("SHINE", 55, "MOD LEVEL"), ("METAL", 25, "MOD RATIO"), ("RING", 70, "DECAY"),
        ("TWIST", 10, "DETUNE")]),
    "squall": dict(name="SQUALL", engine="NOISE", line="Escaped from a jar. Still mad.", knobs=[
        ("GUST", 40, "COLOR"), ("POUR", 65, "DENSITY"), ("RUMBLE", 35, "RESO"), ("SPARK", 15, "CRACKLE")]),
}


def sky(d, y0, y1, seed):
    for y in range(y0, y1):
        t = (y - y0) / max(1, y1 - y0 - 1)
        d.line([(0, y), (239, y)], fill=(int(14 + 22 * t), int(10 + 10 * t), int(32 + 26 * t)))
    r = random.Random(seed)
    for _ in range(30):
        x, y = r.randrange(2, 238), r.randrange(y0 + 2, y1 - 40)
        d.point((x, y), fill=r.choice([(243, 233, 198), (150, 136, 180), (110, 100, 140)]))


def moonbow(d, cx, cy, r0, faint):
    for k, c in enumerate(BOW):
        c = tuple(int(v * faint + 30 * (1 - faint)) for v in c)
        for a in range(0, 1800):
            th = math.pi * a / 1800
            for rr in (r0 + 2 * k, r0 + 2 * k + 1):
                x, y = cx + rr * math.cos(th), cy - rr * math.sin(th)
                if 0 <= x < 240 and 20 <= y < 160:
                    d.point((round(x), round(y)), fill=c)


def header(d, c, i):
    d.rectangle([0, 0, 239, 21], fill=BG)
    d.text((8, 3), c["name"], font=F_M, fill=GOLD)
    chip = c["engine"]
    w = d.textlength(chip, font=F_SB)
    d.rounded_rectangle([196 - w - 10, 4, 196, 18], radius=4, fill=SURF)
    d.text((196 - w - 5, 5), chip, font=F_SB, fill=TEAL)
    d.text((204, 5), f"{i}/3", font=F_S, fill=DIM)
    d.line([(0, 21), (239, 21)], fill=PLUM)


def cards(d, knobs, under=False):
    y = 198
    d.rectangle([0, y, 239, 239], fill=BG)
    for i, (n, v, real) in enumerate(knobs):
        x = 3 + i * 59
        d.rounded_rectangle([x, y + 2, x + 55, y + 38], radius=4, fill=SURF)
        d.text((x + 5, y + 4), n, font=F_SB, fill=GOLD if not under else MIDC)
        if under:
            words = real.split(" + ")
            for j, wd in enumerate(words[:2]):
                d.text((x + 5, y + 16 + j * 10), ("+ " if j else "") + wd, font=F_XS, fill=TEXT)
        else:
            d.text((x + 5, y + 16), f"{v}%", font=F_M, fill=TEXT)
            d.rectangle([x + 5, y + 33, x + 50, y + 34], fill=PLUM)
            d.rectangle([x + 5, y + 33, x + 5 + int(45 * v / 100), y + 34], fill=GOLD)


def home(key, i, under=False):
    c = CAST[key]
    im = Image.new("RGB", (240, 240), BG)
    d = ImageDraw.Draw(im)
    sky(d, 22, 198, i)
    moonbow(d, 120, 214, 104, 0.32)
    for x in range(240):                                  # the hill the character stands on
        top = 178 - int(10 * math.exp(-((x - 120) / 70) ** 2)) + int(2 * math.sin(x / 13))
        d.line([(x, top), (x, 197)], fill=GROUND)
        d.point((x, top), fill=PLUM)
    d.text((8, 25), c["line"], font=F_S, fill=MIDC)
    sp = Image.open(ROOT / "art" / "sprites" / f"{key}.png")
    bx = 120 - sp.width // 2
    by = 172 - sp.height if key != "squall" else 70        # Squall floats
    im.paste(sp, (bx, by), sp)
    if key == "vesper" and not under:                     # a note being played, as Melodee's HOME shows it
        d.text((10, 44), "C4", font=F_L, fill=TEXT)
    header(d, c, i)
    if under:
        d.rounded_rectangle([40, 150, 200, 170], radius=4, fill=SURF)
        t = "EDIT held: what the knobs move"
        d.text((120 - d.textlength(t, font=F_S) / 2, 153), t, font=F_S, fill=TEAL)
    cards(d, c["knobs"], under)
    return im


def codex():
    im = Image.new("RGB", (240, 240), BG)
    d = ImageDraw.Draw(im)
    header(d, CAST["vesper"], 1)
    sp = Image.open(ROOT / "art" / "sprites" / "vesper.png")
    small = sp.resize((sp.width // 2, sp.height // 2), Image.NEAREST)
    im.paste(small, (240 - small.width - 8, 28), small)
    d.text((8, 28), "CODEX", font=F_SB, fill=TEAL)
    d.text((8, 42), "PROPHET-5", font=F_L, fill=GOLD)
    d.text((8, 68), "Sequential, 1978", font=F_S, fill=MIDC)
    body = ["The first synth that could", "remember its sounds. Two",
            "oscillators feed one filter", "that trims the brightness", "away, and that is where",
            "the warmth comes from."]
    for j, ln in enumerate(body):
        d.text((8, 88 + j * 14), ln, font=F_S, fill=TEXT)
    d.rounded_rectangle([6, 178, 233, 214], radius=4, fill=SURF)
    d.text((12, 181), "TRY THIS", font=F_SB, fill=TEAL)
    d.text((12, 196), "Hold a chord, turn GLOW up slowly.", font=F_S, fill=TEXT)
    d.text((8, 222), "PRESETS: next page", font=F_S, fill=DIM)
    d.text((160, 222), "SCL: back", font=F_S, fill=DIM)
    return im


def board(screens, scale=2):
    pad, top = 16, 40
    W = len(screens) * (240 * scale + pad) + pad
    out = Image.new("RGB", (W, 240 * scale + top + pad), (8, 6, 16))
    d = ImageDraw.Draw(out)
    lab = font(22, 650)
    for i, (im, t) in enumerate(screens):
        x = pad + i * (240 * scale + pad)
        d.text((x, 10), t, font=lab, fill=GOLD)
        out.paste(im.resize((240 * scale, 240 * scale), Image.NEAREST), (x, top))
    return out


screens = [(home("vesper", 1), "VESPER (a note playing)"), (home("shard", 2), "SHARD"), (home("squall", 3), "SQUALL"),
           (home("vesper", 1, under=True), "EDIT held"), (codex(), "CODEX (SCL)")]
for im, t in screens:
    im.save(OUT / f"char_{t.split()[0].lower()}_240.png")
board(screens).save(OUT / "characters_mockups.png")
print("ok")
