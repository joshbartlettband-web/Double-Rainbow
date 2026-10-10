#!/usr/bin/env python3
"""The landing page's pictures (landing/), from the emulator's screenshots (tools/landing/shots.mjs) and the sprites:
  screen-*.png  the FM-1's screen at 2x (480 x 480), pixels kept sharp
  cast.png      the seven characters with their engines and lines (the "meet the cast" poster)
  card.png      the link preview (1200 x 630)
  make_landing.py RAW_DIR
"""
import struct
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "landing"
FONT = str(ROOT / "assets" / "fonts" / "InterTight[wght].ttf")
BG, PANEL, TEXT, GOLD, TEAL, DIM = (16, 11, 34), (38, 26, 62), (246, 234, 210), (255, 205, 96), (63, 224, 197), (176, 162, 196)
CAST = [("vesper", "VESPER", "PROPHET", "Every note is her final scene."),
        ("shard", "SHARD", "FM6", "Sharp, clear, honest to a fault."),
        ("squall", "SQUALL", "NOISE", "Escaped from a jar. Still mad."),
        ("wick", "WICK", "VOICE", "Sings when nervous. So, always."),
        ("tock", "TOCK", "DRUM", "Keeps time. Keeps grudges."),
        ("mote", "MOTE", "LOFI", "Small. Loud. Probably your fault."),
        ("murk", "MURK", "PHASE", "Bends the light. Bends the rules.")]
SHOTS = ["splash", "char-0", "char-1", "char-2", "char-3", "char-4", "char-5", "char-6", "edit", "codex", "cast",
         "scry", "trace"]


def font(px, wt):
    f = ImageFont.truetype(FONT, px)
    f.set_variation_by_axes([wt])
    return f


def screen(raw):
    px = struct.unpack(">57600H", raw.read_bytes())
    im = Image.new("RGB", (240, 240))
    im.putdata([((v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31) for v in px])
    return im


def sprite(name, z):
    im = Image.open(ROOT / "art" / "sprites" / f"{name}.png").convert("RGBA")
    return im.resize((im.width * z, im.height * z), Image.NEAREST)


def cast_poster():
    W, cols, cw, ch = 1080, 3, 340, 470
    rows = (len(CAST) + cols - 1) // cols
    im = Image.new("RGB", (W, 150 + rows * (ch + 20) + 20), BG)
    d = ImageDraw.Draw(im)
    d.text((W // 2, 64), "DOUBLE RAINBOW", font=font(60, 800), fill=GOLD, anchor="mm")
    d.text((W // 2, 116), "The cast: each one a sound the Jarkeeper caught in a jar", font=font(28, 400), fill=DIM, anchor="mm")
    for i, (key, name, eng, line) in enumerate(CAST):
        r, c = divmod(i, cols)
        n_row = min(cols, len(CAST) - r * cols)
        x0 = (W - n_row * cw - (n_row - 1) * 20) // 2 + c * (cw + 20)
        y0 = 150 + r * (ch + 20)
        d.rounded_rectangle((x0, y0, x0 + cw, y0 + ch), 22, fill=PANEL)
        sp = sprite(key, 3)
        im.paste(sp, (x0 + (cw - sp.width) // 2, y0 + 330 - sp.height), sp)
        d.text((x0 + cw // 2, y0 + 370), name, font=font(40, 700), fill=GOLD, anchor="mm")
        d.text((x0 + cw // 2, y0 + 410), eng, font=font(24, 600), fill=TEAL, anchor="mm")
        d.text((x0 + cw // 2, y0 + 444), line, font=font(21, 400), fill=TEXT, anchor="mm")
    im.save(OUT / "cast.png", optimize=True)


def card():
    W, H = 1200, 630
    im = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(im)
    bows = [(160, 112, 230), (100, 150, 236), (120, 206, 140), (240, 214, 120), (240, 160, 96), (232, 96, 104)]
    for k, col in enumerate(bows):                      # a moonbow: the primary, then the fainter reversed second
        r = 380 + k * 10
        d.arc((W // 2 - r, 720 - r, W // 2 + r, 720 + r), 180, 360, fill=col, width=10)
        r2 = 470 + k * 10
        c2 = tuple(int(v * .45 + b * .55) for v, b in zip(bows[5 - k], BG))
        d.arc((W // 2 - r2, 720 - r2, W // 2 + r2, 720 + r2), 180, 360, fill=c2, width=10)
    d.rectangle((0, 560, W, H), fill=(18, 11, 28))
    xs = [190, 360, 600, 840, 1010]
    for x, key in zip(xs, ["vesper", "shard", "squall", "tock", "murk"]):
        sp = sprite(key, 3)
        im.paste(sp, (x - sp.width // 2, 570 - sp.height if key not in ("squall", "murk") else 196), sp)
    d.text((W // 2, 70), "DOUBLE RAINBOW", font=font(72, 800), fill=GOLD, anchor="mm")
    d.text((W // 2, 128), "Free firmware for the M-VAVE FM-1. Try it in your browser.", font=font(30, 500), fill=TEXT,
           anchor="mm")
    im.save(OUT / "card.png", optimize=True)


def main():
    raw = Path(sys.argv[1])
    OUT.mkdir(exist_ok=True)
    for s in SHOTS:
        screen(raw / f"{s}.raw").resize((480, 480), Image.NEAREST).save(OUT / f"screen-{s}.png", optimize=True)
    cast_poster()
    card()
    print(f"landing: {len(SHOTS)} screens, cast.png, card.png")


if __name__ == "__main__":
    main()
