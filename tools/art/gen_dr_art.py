#!/usr/bin/env python3
"""Double Rainbow's sprites (art/sprites/*.png, made by tools/art/procchar.py) -> a C header (run by tools/build.py
generate()).
  gen_dr_art.py OUT.h
Each sprite: its size, 15 colours (RGB565; index 0 is clear) and its pixels as runs, row-major, a run a symbol:
index << 4 | (length - 1), 1..16 pixels, Huffman-coded (one code for all the sprites: about 30 % smaller). Fails if a sprite has more than 15 colours.
Its glow: the colours that light up with the sound (firmware/src/dr.c, the palette animation), a bit each (bit i:
colour index i + 1), and the rows they are on (only those are redrawn). Chosen as the vivid, bright colours that
are a small part of the picture (eyes, a flame, a bolt, a throat); GLOW below sets them by hand where that misses."""
import colorsys
from dr_eyes import EYES, LID, shut
import sys
sys.path.insert(0, str(__import__('pathlib').Path(__file__).resolve().parent))
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
GLOW = {                                 # by hand, as the sprites' own colours: the part that glows
    "shard": [],                         # (his light sweeps instead, dr.c dr_sweep_px)
    "wick": [(74, 145, 127), (90, 131, 105)],          # the flame's teal heart
    "tock": [(83, 188, 159), (70, 96, 77)],             # the clock face
    "mote": [(225, 208, 61), (211, 213, 129)],          # his eyes
    "tumbler": [(59, 138, 111)],                        # his eyes
    "hush": [],                                         # (her locket shows the Line instead: SCOPE)
    "hermit": [(235, 202, 125), (220, 171, 88)],        # the horn's shine
    "bog": [(180, 251, 70), (205, 234, 133)],           # the throat
}
SCOPE = {                                # a window that shows the Line, the live wave (firmware/src/dr.c dr_scope_px):
    "hush": [(43, 63, 68), (54, 132, 121)],             # her locket's glass and its painted trace
}
SPRITES = ["vesper", "shard", "squall", "wick", "tock", "mote", "murk", "bog", "hush", "tumbler", "glint", "hermit"]          # the cast's order (firmware/src/dr.c DR_CAST)


def rgb565(c):
    r, g, b = c[:3]
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def pal_rgb(pal, px, idx):
    """the palette as the PNG's own colours (dr_eyes.py compares with those)"""
    out = [None] * len(pal)
    for p, v in zip(px, idx):
        if v and out[v - 1] is None:
            out[v - 1] = p[:3]
    return out


def pack(name):
    im = Image.open(ROOT / "art" / "sprites" / f"{name}.png").convert("RGBA")
    w, h = im.size
    px = list(im.get_flattened_data()) if hasattr(im, 'get_flattened_data') else list(im.getdata())
    pal = []
    idx = []
    for p in px:
        if p[3] < 128:
            idx.append(0)
            continue
        c = rgb565(p)
        if c not in pal:
            pal.append(c)
        idx.append(pal.index(c) + 1)
    if len(pal) > 15:
        raise SystemExit(f"{name}: {len(pal)} colours, at most 15 (tools/art/procchar.py)")
    runs, i = [], 0
    while i < len(idx):
        j = i
        while j < len(idx) and idx[j] == idx[i] and j - i < 16:
            j += 1
        runs.append(idx[i] << 4 | (j - i - 1))
        i = j
    glow = 0
    n = [idx.count(i + 1) for i in range(len(pal))]
    solid = sum(n)
    for i, c in enumerate(pal):
        r, g, b = ((c >> 11) & 31) / 31, ((c >> 5) & 63) / 63, (c & 31) / 31
        hh, ss, vv = colorsys.rgb_to_hsv(r, g, b)
        if ss >= 0.45 and vv >= 0.62 and n[i] <= solid * 0.22:
            glow |= 1 << i
    if name in GLOW:
        glow = sum(1 << pal.index(rgb565(c)) for c in GLOW[name])
    rows = [k // w for k, v in enumerate(idx) if v and (glow >> (v - 1)) & 1]
    g0, g1 = (min(rows), max(rows)) if rows else (0, 0)
    # the eyes shut (tools/art/dr_eyes.py): each box's pixels as the blink draws them, 4 bits each (0: as they are)
    boxes = EYES.get(name, [])
    lid = LID.get(name)
    closed, _ = shut(idx, w, h, [tuple(pp) for pp in pal_rgb(pal, px, idx)], boxes, tuple(lid) if lid else None)
    patch = []
    for x0, y0, x1, y1 in boxes:
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                k = y * w + x
                patch.append(closed[k] if closed[k] != idx[k] else 0)
    if len(patch) & 1:
        patch.append(0)
    shutb = [patch[i] << 4 | patch[i + 1] for i in range(0, len(patch), 2)]
    scope, box = 0, (0, 0, 0, 0)
    if name in SCOPE:
        scope = sum(1 << pal.index(rgb565(c)) for c in SCOPE[name])
        pts = [(k % w, k // w) for k, v in enumerate(idx) if v and (scope >> (v - 1)) & 1]
        box = (min(p[0] for p in pts), min(p[1] for p in pts), max(p[0] for p in pts), max(p[1] for p in pts))
    return w, h, pal + [0] * (15 - len(pal)), runs, glow, g0, g1, boxes, shutb, scope, box


HC_MAXLEN = 15


def huff_lengths(count):
    """code lengths for the symbols (Huffman), none over HC_MAXLEN (the decoder's limit)"""
    import heapq
    heap = [(c, i, [sym]) for i, (sym, c) in enumerate(sorted(count.items()))]
    heapq.heapify(heap)
    length = {sym: 0 for sym in count}
    k = len(heap)
    while len(heap) > 1:
        a, b = heapq.heappop(heap), heapq.heappop(heap)
        for sym in a[2] + b[2]:
            length[sym] += 1
        heapq.heappush(heap, (a[0] + b[0], k, a[2] + b[2]))
        k += 1
    if max(length.values()) > HC_MAXLEN:
        raise SystemExit(f"dr art: a Huffman code of {max(length.values())} bits (the decoder takes {HC_MAXLEN})")
    return {sym: max(1, n) for sym, n in length.items()}


def huff_code(streams):
    """one canonical Huffman code for every sprite's runs: (the table: the number of codes of each length 1..15,
    then the symbols in code order; [each sprite's bytes, MSB first, 2 spare bytes at the end])"""
    count = {}
    for st in streams:
        for sym in st:
            count[sym] = count.get(sym, 0) + 1
    length = huff_lengths(count)
    order = sorted(count, key=lambda sym: (length[sym], sym))
    code, c, prev = {}, 0, length[order[0]]
    for sym in order:
        c <<= length[sym] - prev
        prev = length[sym]
        code[sym] = c
        c += 1
    table = [sum(1 for sym in order if length[sym] == n) for n in range(1, HC_MAXLEN + 1)] + order
    out = []
    for st in streams:
        acc, nb, b = 0, 0, bytearray()
        for sym in st:
            acc, nb = (acc << length[sym]) | code[sym], nb + length[sym]
            while nb >= 8:
                nb -= 8
                b.append((acc >> nb) & 255)
        if nb:
            b.append((acc << (8 - nb)) & 255)
        out.append(bytes(b) + b"\0\0")
    return table, out


def huff_decode(table, data, n):
    """the decoder, as firmware/src/dr.c dr_run_next does it (the self-check)"""
    out, pos = [], 0
    while len(out) < n:
        code = first = idx = 0
        for ln in range(HC_MAXLEN):
            code |= (data[pos >> 3] >> (7 - (pos & 7))) & 1
            pos += 1
            if code - first < table[ln]:
                break
            idx += table[ln]
            first = (first + table[ln]) << 1
            code <<= 1
        out.append(table[HC_MAXLEN + idx + code - first])
    return out


def main():
    out = ["/* generated by tools/art/gen_dr_art.py from art/sprites/: Double Rainbow's characters */", "#pragma once",
           "#include <stdint.h>",
           "typedef struct { uint8_t w, h; uint16_t pal[15]; uint16_t n; const uint8_t *run; uint16_t glow; uint8_t g0, g1;",
           "                 uint8_t ne, eye[2][4]; const uint8_t *shut;   /* eye: x0 y0 x1 y1; shut: 4 bits a pixel */",
           "                 uint16_t scope; uint8_t sc[4]; } dr_sprite_t;   /* the colours of a window on the Line; its box */",
           "/* run: the runs Huffman-coded, one code for all (DR_HUFF: the number of codes of each length 1..15, then the",
           " * symbols in code order), MSB first; n runs */"]
    packs = [pack(name) for name in SPRITES]
    table, coded = huff_code([pk[3] for pk in packs])
    for pk, data in zip(packs, coded):
        if huff_decode(table, data, len(pk[3])) != pk[3]:
            raise SystemExit("dr art: the Huffman self-check failed")
    out.append(f"static const uint8_t DR_HUFF[{len(table)}] = {{" + ", ".join(map(str, table)) + "};")
    total, raw = len(table), 0
    for name, pk, data in zip(SPRITES, packs, coded):
        w, h, pal, runs, glow, g0, g1, boxes, shutb, scope, box = pk
        total += len(data) + 30 + 20 + len(shutb)
        raw += len(runs)
        out.append(f"static const uint8_t DR_SHUT_{name.upper()}[{max(1, len(shutb))}] = {{" + (", ".join(map(str, shutb)) or "0") + "};")
        lit = [f"#{(c >> 11) << 3:02x}{((c >> 5) & 63) << 2:02x}{(c & 31) << 3:02x}" for i, c in enumerate(pal) if (glow >> i) & 1]
        print(f"  {name}: {len(runs)} runs in {len(data)} B; glow {' '.join(lit) or '-'} (rows {g0}..{g1})")
        out.append(f"static const uint8_t DR_RUN_{name.upper()}[{len(data)}] = {{")
        for i in range(0, len(data), 24):
            out.append("    " + ", ".join(str(b) for b in data[i:i + 24]) + ",")
        out.append("};")
    out.append(f"static const dr_sprite_t DR_SPRITES[{len(SPRITES)}] = {{")
    for name, pk in zip(SPRITES, packs):
        w, h, pal, runs, glow, g0, g1, boxes, shutb, scope, box = pk
        eyes = ", ".join("{" + ", ".join(map(str, b)) + "}" for b in (boxes + [(0, 0, 0, 0)] * 2)[:2])
        out.append(f"    {{{w}, {h}, {{" + ", ".join(f"0x{c:04x}" for c in pal) +
                   f"}}, {len(runs)}, DR_RUN_{name.upper()}, 0x{glow:04x}, {g0}, {g1}, {len(boxes)}, {{{eyes}}}, DR_SHUT_{name.upper()},"
                   f" 0x{scope:04x}, {{{', '.join(map(str, box))}}}}},")
    out += ["};", f"/* {total} B in all (the runs {raw} B before the Huffman code) */", ""]
    Path(sys.argv[1]).write_text("\n".join(out))
    print(f"dr art: {len(SPRITES)} sprites, {total} B (runs {raw} B coded to {sum(map(len, coded))} B) -> {sys.argv[1]}")


if __name__ == "__main__":
    main()
