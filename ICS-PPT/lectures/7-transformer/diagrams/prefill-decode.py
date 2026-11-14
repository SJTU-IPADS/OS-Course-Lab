#!/usr/bin/env python3
"""prefill-decode: the two phases of generation and the shape of each.

The per-layer K, V caches of kv-cache.py, three layers from the bottom up.
1: prefill: the prompt "a fluffy blue creature roamed" goes through every
   layer as one block of five positions; this fills the caches, and the
   last position gives the first generated token, "the".
2: decode: "the" goes through the layers alone, one position wide; each
   layer appends its K, V and reads the cache; it gives "verdant", which
   the next decode step takes as input.
"""
from tk import *

H = 400
WORDS = "a fluffy blue creature roamed the verdant".split()
PITCH, X0 = 108, 196
ROW_Y = {1: 292, 2: 198, 3: 104}
CELLS, CH, MW = 4, 11, 13
CHIP_Y = 342
PRED_Y = 10


def cx(i):
    return X0 + i * PITCH


def tint(color_hex, t):
    base = tuple(int(color_hex[k:k + 2], 16) for k in (1, 3, 5))
    r, g, b_ = (round(255 + (c - 255) * t) for c in base)
    return f"#{r:02x}{g:02x}{b_:02x}"


def mvec(x, y, seed, color):
    top, left = y - CELLS * CH / 2, x - MW / 2
    out = [rect(left - 2, top - 2, MW + 4, CELLS * CH + 4, WHITE, color, rx=3, width=1.6)]
    for k in range(CELLS):
        t = 0.15 + 0.6 * abs(pseudo(seed, k, 0))
        out.append(rect(left, top + k * CH, MW, CH, tint(color, t), WHITE, rx=0, width=1))
    return out


def pair(i, layer):
    x, y = cx(i), ROW_Y[layer]
    return (mvec(x - 9, y, f"kv-k-{layer}-{i}", K) +
            mvec(x + 9, y, f"kv-v-{layer}-{i}", V))


def empty_slot(i, layer):
    x, y = cx(i), ROW_Y[layer]
    return [rect(x - 20, y - CELLS * CH / 2 - 4, 40, CELLS * CH + 8, "none", LINE, rx=4,
                 width=1.4, dash="5 4")]


def token(i, word, role, y):
    els, _ = chip(cx(i), y, word, role, size=18, h=34, anchor="middle")
    return els


def frame(n):
    b = []
    for layer, y in ROW_Y.items():
        b.append(text(cx(0) - 58, y + 7, f"第 {layer} 层", 19, INK, anchor="end"))
        b.append(rect(cx(0) - 30, y - 30, cx(5) - cx(0) + 60, 60, FILL_GREY, LINE, rx=8,
                      width=1.4))
        for i in range(5):
            b += pair(i, layer)
        b += pair(5, layer) if n == 2 else empty_slot(5, layer)
    for i in range(5):
        b += token(i, WORDS[i], "data", CHIP_Y)
    top3 = ROW_Y[3] - 30
    band_top, band_bot = top3 - 9, CHIP_Y + 34 + 9

    # prefill: one block of five positions
    pc = ORANGE if n == 1 else MUTED
    b.append(rect(cx(0) - 42, band_top, cx(4) - cx(0) + 84, band_bot - band_top, "none", pc,
                  rx=10, width=2.2, dash=None if n == 1 else "6 5"))
    b.append(text(cx(0) - 38, band_top - 10, "prefill：5 个位置一起计算", 19, pc, "bold",
                  anchor="start"))
    b.append(arrow(cx(4), top3 - 2, cx(4), PRED_Y + 38, INK, 1.8))
    b += token(4, WORDS[5], "upd", PRED_Y)

    if n == 2:
        # decode: one position
        b += token(5, WORDS[5], "upd", CHIP_Y)
        x0 = cx(5) - 40
        b.append(rect(x0, band_top, 80, band_bot - band_top, "none", ORANGE, rx=10, width=2.2))
        lx = x0 + 104
        b.append(text(lx, ROW_Y[2] - 22, "decode：", 19, ORANGE, "bold", anchor="start"))
        b.append(text(lx, ROW_Y[2] + 6, "1 个位置", 19, ORANGE, "bold", anchor="start"))
        b.append(text(lx, ROW_Y[2] + 34, "读取各层缓存", 18, ORANGE, anchor="start"))
        b.append(arrow(cx(5), top3 - 2, cx(5), PRED_Y + 38, INK, 1.8))
        b += token(5, WORDS[6], "upd", PRED_Y)
        # the generated token is the input of the next decode step
        els, _ = chip(cx(6) + 4, CHIP_Y, WORDS[6], "upd", size=18, h=34, anchor="middle",
                      dash="5 4")
        b += els
        b.append(text(cx(6) + 4, CHIP_Y - 14, "下一次 decode", 17, MUTED))
    save(f"prefill-decode-{n}", b, H)


for n in (1, 2):
    frame(n)
