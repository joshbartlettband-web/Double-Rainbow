#!/usr/bin/env python3
"""One Gemini image, logged with its cost (after Fretbound's art/gemini_test/gen_img.py).
  python3 tools/art/gen_img.py OUT.jpg "prompt" [aspect] [ref.png ...]
Model: env GEM (default gemini-3.1-flash-image), size env GEM_SIZE (default 1K). Every call (and its tokens and
cost) goes to art/gemini/cost_log.csv, so the running total is always known. Sources are kept as q90 JPEGs."""
import base64, csv, io, json, os, subprocess, sys, time
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
LOG = ROOT / "art" / "gemini" / "cost_log.csv"
# USD per image at the standard tier (ai.google.dev/gemini-api/docs/pricing, October 2026)
PRICE = {("gemini-3.1-flash-image", "512"): 0.045, ("gemini-3.1-flash-image", "1K"): 0.067,
         ("gemini-3.1-flash-image", "2K"): 0.101, ("gemini-3.1-flash-lite-image", "1K"): 0.0336,
         ("gemini-3-pro-image", "1K"): 0.134, ("gemini-3-pro-image", "2K"): 0.134}


def total():
    if not LOG.exists():
        return 0.0
    return sum(float(r["usd"]) for r in csv.DictReader(LOG.open()))


def main():
    out, prompt = sys.argv[1], sys.argv[2]
    aspect = sys.argv[3] if len(sys.argv) > 3 else "1:1"
    refs = sys.argv[4:]
    model, size = os.environ.get("GEM", "gemini-3.1-flash-image"), os.environ.get("GEM_SIZE", "1K")
    cap = float(os.environ.get("GEM_CAP", "3.0"))
    if total() + PRICE.get((model, size), 0.15) > cap:
        sys.exit(f"stopped: the next image would pass the ${cap:.2f} cap (spent ${total():.3f})")
    parts = [{"text": prompt}]
    for r in refs:
        b = io.BytesIO()
        Image.open(r).convert("RGB").save(b, "PNG")
        parts.append({"inlineData": {"mimeType": "image/png", "data": base64.b64encode(b.getvalue()).decode()}})
    body = {"contents": [{"parts": parts}],
            "generationConfig": {"responseModalities": ["IMAGE"], "imageConfig": {"aspectRatio": aspect, "imageSize": size}}}
    url = f"https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent"
    r = subprocess.run(["curl", "-sS", "-X", "POST", url, "-H", "Content-Type: application/json", "-d", "@-"],
                       input=json.dumps(body), capture_output=True, text=True)
    d = json.loads(r.stdout or "{}")
    usage = d.get("usageMetadata", {})
    saved = False
    for p in (d.get("candidates") or [{}])[0].get("content", {}).get("parts", []):
        if "inlineData" in p:
            im = Image.open(io.BytesIO(base64.b64decode(p["inlineData"]["data"]))).convert("RGB")
            Path(out).parent.mkdir(parents=True, exist_ok=True)
            im.save(out, quality=90)
            saved = True
            break
    usd = PRICE.get((model, size), 0.15) if saved else 0.0
    new = not LOG.exists()
    with LOG.open("a", newline="") as f:
        w = csv.writer(f)
        if new:
            w.writerow(["time", "file", "model", "size", "prompt_tokens", "output_tokens", "usd", "ok"])
        w.writerow([time.strftime("%Y-%m-%d %H:%M"), os.path.relpath(out, ROOT) if out.startswith(str(ROOT)) else out,
                    model, size, usage.get("promptTokenCount", 0), usage.get("candidatesTokenCount", 0), usd, int(saved)])
    if not saved:
        sys.exit(f"FAILED {out}: {str(d)[:400]}")
    print(f"saved {out} {im.size}  tokens {usage}  this ${usd:.3f}  total ${total():.3f}")


if __name__ == "__main__":
    main()
