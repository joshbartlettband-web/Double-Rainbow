# The tour video's compositor: run in the folder record_tour.mjs wrote to (frames.rgb, audio.wav, captions.json).
#   python3 compose_tour.py video OUT.mp4      the video (ffmpeg)
#   python3 compose_tour.py still FRAME OUT.png  one frame, to check
# After Rainbow mode's (felucca-rainbow tools/video), in Double Rainbow's colors (landing/index.html).
import json, pathlib, subprocess, sys
from PIL import Image, ImageDraw, ImageFont
FONT = str(pathlib.Path(__file__).resolve().parents[2] / "assets" / "fonts" / "InterTight[wght].ttf")
def font(size, w):
    f = ImageFont.truetype(FONT, size); f.set_variation_by_name(w); return f
W, H = 1080, 1350
BG, PANEL, INK, SOFT, DIM = (16, 11, 34), (38, 26, 62), (246, 234, 210), (210, 196, 226), (156, 140, 184)
GOLD, TEAL = (255, 205, 96), (63, 224, 197)
BOW = [(160, 112, 230), (100, 150, 236), (120, 206, 140), (240, 214, 120), (240, 160, 96), (232, 96, 104)]
LETTER = {0: "C", 2: "D", 4: "E", 5: "F", 7: "G", 9: "A", 11: "B"}
T1, T2, T3, TL, TK = font(64, b"ExtraBold"), font(34, b"Medium"), font(26, b"SemiBold"), font(40, b"Bold"), font(22, b"Bold")
meta = json.load(open("captions.json")); caps = meta["captions"]; fps = meta["fps"]; n = meta["frames"]; KEYS = meta["keys"]
ITEMS = ["30 characters, each a real synth engine", "Prophet-5, DX7 FM, CZ-1, the 808 and more",
         "Four knobs each; EDIT shows what they move", "The Keeper's ledger, a page per character",
         "A beat for every character; tap the tempo", "An arp for every character, with HUMAN",
         "WEAVE and CAST: chords and their numerals", "SCRY trains your ear, TRACE teaches songs",
         "Free, and it runs in your browser too"]
SX, SY, SS = 108, 196, 864                    # the FM-1's screen: 240 px at 3.6x
KX0, KX1, KY0, KY1 = 40, 1040, 1106, 1270     # the keyboard: F3 .. G5, 16 white keys

def keys_layout():
    whites, blacks, wi = [], [], 0
    ww = (KX1 - KX0) / 16
    for k in range(27):
        pc = (53 + k) % 12
        if pc in LETTER:
            whites.append((k, pc, KX0 + wi * ww, KX0 + (wi + 1) * ww)); wi += 1
        else:
            x = KX0 + wi * ww
            blacks.append((k, pc, x - ww * 0.32, x + ww * 0.32))
    return whites, blacks
WHITES, BLACKS = keys_layout()

def base():
    im = Image.new("RGB", (W, H), BG); d = ImageDraw.Draw(im)
    for i, c in enumerate(BOW):
        d.rectangle([i * W // 6, 0, (i + 1) * W // 6, 12], fill=c)
    d.rounded_rectangle([SX - 22, SY - 22, SX + SS + 22, SY + SS + 22], radius=34, fill=PANEL)
    d.rounded_rectangle([KX0 - 16, KY0 - 14, KX1 + 16, KY1 + 14], radius=22, fill=PANEL)
    d.text((W // 2, H - 34), "Double Rainbow  ·  built on Melodee  ·  M-VAVE FM-1  ·  free and open source", font=T3, fill=DIM, anchor="mm")
    return im
BASE = base()

def keyboard(d, held, lit):
    for k, pc, x0, x1 in WHITES:
        on = (held >> k) & 1
        d.rounded_rectangle([x0 + 2, KY0, x1 - 2, KY1], radius=8, fill=TEAL if on else (236, 228, 214))
        if (lit >> k) & 1 and not on:
            d.rounded_rectangle([x0 + 2, KY0, x1 - 2, KY1], radius=8, outline=GOLD, width=6)
        d.text(((x0 + x1) / 2, KY1 - 22), LETTER[pc], font=TK, fill=BG if on else DIM, anchor="mm")
    for k, pc, x0, x1 in BLACKS:
        on = (held >> k) & 1
        d.rounded_rectangle([x0, KY0, x1, KY0 + 96], radius=6, fill=TEAL if on else (28, 20, 44))
        if (lit >> k) & 1 and not on:
            d.rounded_rectangle([x0, KY0, x1, KY0 + 96], radius=6, outline=GOLD, width=4)

def fade(c, a, bg=BG):
    return tuple(int(bg[i] + (c[i] - bg[i]) * a) for i in range(3))

def frames(start=0, stop=None):
    raw = open("frames.rgb", "rb"); stop = stop or n
    raw.seek(start * 240 * 240 * 3)
    for f in range(start, stop):
        t = f * 1000 / fps
        cur = max((c for c in caps if c[0] <= t), key=lambda c: c[0])
        a = min(1.0, (t - cur[0]) / 250)
        scr = Image.frombytes("RGB", (240, 240), raw.read(240 * 240 * 3)).resize((SS, SS), Image.NEAREST)
        im = BASE.copy(); im.paste(scr, (SX, SY)); d = ImageDraw.Draw(im)
        title, sub = cur[1], cur[2]
        held, lit = KEYS[f]
        keyboard(d, held, lit)
        if sub in ("__LIST__", "__END__"):                       # the summary: a card over the screen
            ov = Image.new("RGBA", (SS, SS), (16, 11, 34, int(210 * a))); im.paste(ov, (SX, SY), ov); d = ImageDraw.Draw(im)
            d.text((W // 2, 104), title, font=T1, fill=fade(GOLD, a), anchor="mm")
            if sub == "__LIST__":
                d.text((W // 2, 168), "and the whole Melodee synth, one button combo away", font=T2, fill=fade(SOFT, a), anchor="mm")
                for i, item in enumerate(ITEMS):
                    ai = max(0.0, min(1.0, (t - cur[0] - 900 - i * meta["list_gap"]) / 300))
                    if ai <= 0:
                        continue
                    y = SY + 64 + i * 96
                    d.ellipse([SX + 48, y - 14, SX + 76, y + 14], fill=fade(BOW[i % 6], ai, PANEL))
                    d.text((SX + 100 + int((1 - ai) * 30), y), item, font=TL, fill=fade(INK, ai, PANEL), anchor="lm")
            else:
                d.text((W // 2, 168), "a sound, a character or a song: tell us in the comments", font=T2, fill=fade(SOFT, a), anchor="mm")
                d.text((W // 2, SY + SS // 2 - 60), "Try it in your browser", font=T1, fill=fade(INK, a, PANEL), anchor="mm")
                d.text((W // 2, SY + SS // 2 + 20), "or put it on an FM-1, free", font=T2, fill=fade(SOFT, a, PANEL), anchor="mm")
                d.text((W // 2, SY + SS // 2 + 90), "joshbartlettband-web.github.io/Double-Rainbow", font=T3, fill=fade(TEAL, a, PANEL), anchor="mm")
        else:
            d.text((W // 2, 104), title, font=T1, fill=fade(GOLD, a), anchor="mm")
            d.text((W // 2, 168), sub, font=T2, fill=fade(SOFT, a), anchor="mm")
        yield im

mode = sys.argv[1]
if mode == "video":
    ff = subprocess.Popen(["ffmpeg", "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}",
                           "-r", str(fps), "-i", "-", "-i", "audio.wav", "-c:v", "libx264", "-preset", "slow", "-crf", "20",
                           "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "160k", "-shortest", "-movflags", "+faststart",
                           sys.argv[2] if len(sys.argv) > 2 else "double-rainbow.mp4"], stdin=subprocess.PIPE)
    for im in frames():
        ff.stdin.write(im.tobytes())
    ff.stdin.close(); ff.wait()
elif mode == "still":
    next(frames(start=int(sys.argv[2]), stop=int(sys.argv[2]) + 1)).save(sys.argv[3])
