#!/usr/bin/env python3
"""The next four characters, proposed (concepts in art/gemini/*_a.jpg) -> docs/mockups/new_cast.png.
The magenta key is swapped for the night sky so they can be judged on Double Rainbow's colours."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
FONT = str(ROOT / "assets" / "fonts" / "InterTight[wght].ttf")
CAST = [
    ("WICK", "VOICE", "A candle who sings vowels",
     "Sings when she's nervous. She is always nervous.",
     "The Jarkeeper's night-light, the one light allowed in the room. She saw who broke jar twelve."),
    ("TOCK", "DRUM", "A clockwork beetle, the rhythm section",
     "Keeps time. Keeps grudges.",
     "The clock on the Jarkeeper's shelf. He has counted every night since jar one."),
    ("MOTE", "LOFI", "An 8-bit gremlin made of jar dust",
     "Small. Loud. Probably your fault.",
     "Lives between the shelves and steals the round glass stoppers off the jars. They look like marbles to him. It is why some jars hum."),
    ("MURK", "CZ-1", "A deep-sea jellyfish that bends waves",
     "Bends the light. Bends the rules.",
     "Jar one, the oldest. The only sound the Jarkeeper ever let go on purpose. Nobody knows why."),
]
BG, PANEL, GOLD, TEAL, TEXT, DIM = (16, 11, 34), (38, 26, 62), (255, 205, 96), (63, 224, 197), (246, 234, 210), (170, 150, 190)


def keyed(path):
    im = Image.open(path).convert("RGB").resize((360, 360), Image.LANCZOS)
    px = im.load()
    for y in range(im.height):
        for x in range(im.width):
            r, g, b = px[x, y]
            if r > 170 and b > 170 and g < 110:          # the magenta key
                px[x, y] = PANEL
    return im


def wrap(d, text, font, w):
    out, line = [], ""
    for word in text.split():
        t = (line + " " + word).strip()
        if d.textlength(t, font=font) > w and line:
            out.append(line)
            line = word
        else:
            line = t
    return out + [line]


def main():
    W, H = 4 * 420 + 20, 690
    sheet = Image.new("RGB", (W, H), BG)
    d = ImageDraw.Draw(sheet)
    f = lambda px, wt: ImageFont.truetype(FONT, px, layout_engine=ImageFont.Layout.BASIC) if not wt else \
        (lambda ft: (ft.set_variation_by_axes([wt]), ft)[1])(ImageFont.truetype(FONT, px))
    title, name, tag, body, small = f(34, 700), f(40, 700), f(20, 600), f(19, 400), f(17, 400)
    d.text((20, 18), "Double Rainbow: four more jars (proposed)", font=title, fill=GOLD)
    for i, (n, eng, what, line, lore) in enumerate(CAST):
        x = 20 + i * 420
        d.rounded_rectangle((x, 80, x + 400, H - 20), 14, fill=PANEL)
        src = ROOT / "art" / "gemini" / f"{n.lower()}_a.jpg"
        if src.with_name(f"{n.lower()}_a_fix.png").exists():      # (Mote: the stopper, tools/art/fix_mote.py)
            src = src.with_name(f"{n.lower()}_a_fix.png")
        sheet.paste(keyed(src), (x + 20, 96))
        d.text((x + 20, 470), n, font=name, fill=GOLD)
        tw = d.textlength(eng, font=tag)
        d.rounded_rectangle((x + 380 - tw - 20, 478, x + 380, 508), 6, fill=BG)
        d.text((x + 380 - tw - 10, 481), eng, font=tag, fill=TEAL)
        d.text((x + 20, 522), what, font=small, fill=DIM)
        y = 556
        for ln in wrap(d, '"' + line + '"', body, 360):
            d.text((x + 20, y), ln, font=body, fill=TEXT)
            y += 26
        y += 12
        for ln in wrap(d, lore, small, 360):
            d.text((x + 20, y), ln, font=small, fill=TEAL)
            y += 23
    sheet.save(ROOT / "docs" / "mockups" / "new_cast.png")


if __name__ == "__main__":
    main()
