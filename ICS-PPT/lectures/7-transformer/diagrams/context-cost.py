#!/usr/bin/env python3
"""context-cost: the attention pattern grows with the square of the context.

1: an 8 × 8 attention grid, 64 entries.
2: beside it, a 16 × 16 grid at the same cell size, 256 entries: its four
   quarters are each as large as the 8 × 8 grid. GPT-3's context of 2048
   tokens gives 4,194,304 entries, for every head in every layer.

The examples are parallel, so the figure is drawn once, in its final state;
frame() keeps the step numbers only to say which parts are drawn.
"""
from PIL import ImageFont

from tk import *

H = 380
C = 20                                 # cell size of both grids
BASE = 332                             # bottom edge of both grids
SX = 116                               # small grid
BX = SX + 8 * C + 96                   # large grid


SERIF = ImageFont.truetype("/usr/share/fonts/truetype/liberation/LiberationSerif-Regular.ttf", 100)
SERIF_I = ImageFont.truetype("/usr/share/fonts/truetype/liberation/LiberationSerif-Italic.ttf", 100)


def math_then_text(x, y, sym, rest, words, size=20, weight="normal"):
    """n (italic) and a formula, then a CJK phrase, on one baseline."""
    out = math(x, y, sym + rest, size, INK, anchor="start")
    w = (SERIF_I.getlength(sym) + SERIF.getlength(rest)) * size / 100
    out.append(text(x + w + size * 0.25, y, words, size, INK, weight, anchor="start"))
    return out


def mix(c1, c2, t):
    a = [int(c1[i:i + 2], 16) for i in (1, 3, 5)]
    b_ = [int(c2[i:i + 2], 16) for i in (1, 3, 5)]
    return "#%02x%02x%02x" % tuple(round(x + (y - x) * t) for x, y in zip(a, b_))


def attn_grid(x, y, n, seed):
    out = []
    for i in range(n):
        for j in range(n):
            t = 0.05 + 0.3 * ((pseudo(seed, i, j) + 1) / 2) ** 2
            out.append(rect(x + j * C, y + i * C, C, C, mix(WHITE, DATA, t), LINE, rx=0,
                            width=0.8))
    out.append(rect(x, y, n * C, n * C, "none", INK, rx=0, width=1.6))
    return out


def frame(n):
    b = []
    top = BASE - 8 * C
    b += attn_grid(SX, top, 8, "ctx8")
    b.append(text(SX + 4 * C, top - 12, "8 个查询", 17, Q))
    b.append(text(SX - 10, top + 4 * C + 6, "8 个键", 17, K, anchor="end"))
    b += math(SX + 4 * C, BASE + 34, "n = 8 → 64", 22, INK)
    if n >= 2:
        top2 = BASE - 16 * C
        b += attn_grid(BX, top2, 16, "ctx16")
        mid = 8 * C
        b.append(line(BX + mid, top2, BX + mid, BASE, INK, 1.6))
        b.append(line(BX, top2 + mid, BX + 16 * C, top2 + mid, INK, 1.6))
        b += math(BX + mid, BASE + 34, "n = 16 → 256", 22, INK)
        tx = BX + 16 * C + 48
        b += math_then_text(tx, 140, "n", "", "加倍，元素数变为 4 倍", 20, "bold")
        b.append(text(tx, 216, "GPT-3", 18, MUTED, "bold", anchor="start"))
        b += math_then_text(tx, 248, "n", " = 2048 → 4,194,304", "个元素", 20)
        b.append(text(tx, 278, "（每个头、每层）", 18, INK, anchor="start"))
    save("context-cost", b, H)


frame(2)
