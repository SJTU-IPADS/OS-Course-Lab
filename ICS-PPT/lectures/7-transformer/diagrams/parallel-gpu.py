#!/usr/bin/env python3
"""parallel-gpu: every key-query dot product can be computed at once.

The 8x8 grid of attention-scores (keys rows, queries columns), every cell
washed orange and holding one core: each core computes one dot product
k_i . q_j, all of them in the same step, since no cell depends on another.
"""
from tk import *
from attn_data import WORDS, N

H = 420
CELL = 40
GX, GY = 226, 92


def core(cx, cy, s=18, color=ORANGE):
    """A processor core: a small square with two pins on each side."""
    out = [rect(cx - s / 2, cy - s / 2, s, s, WHITE, color, rx=2, width=1.6)]
    for t in (-s / 5, s / 5):
        out.append(line(cx + t, cy - s / 2, cx + t, cy - s / 2 - 4, color, 1.4))
        out.append(line(cx + t, cy + s / 2, cx + t, cy + s / 2 + 4, color, 1.4))
        out.append(line(cx - s / 2, cy + t, cx - s / 2 - 4, cy + t, color, 1.4))
        out.append(line(cx + s / 2, cy + t, cx + s / 2 + 4, cy + t, color, 1.4))
    return out


b = []
for j, w in enumerate(WORDS):
    cx = GX + (j + .5) * CELL
    b.append(f'<text x="{cx:.1f}" y="{GY - 40:.1f}" font-family="{FONT}" font-size="18" '
             f'fill="{INK}" text-anchor="start" transform="rotate(-35 {cx:.1f} {GY - 40:.1f})">'
             f'{w}</text>')
    b += math(cx, GY - 12, f"q_{j + 1}", 19, Q)
for i, w in enumerate(WORDS):
    cy = GY + (i + .5) * CELL + 7
    b.append(text(GX - 52, cy, w, 18, INK, anchor="end"))
    b += math(GX - 24, cy, f"k_{i + 1}", 19, K)

b += grid(GX, GY, N, N, CELL, fills=[[FILL_ORANGE] * N for _ in range(N)])
for i in range(N):
    for j in range(N):
        b += core(GX + (j + .5) * CELL, GY + (i + .5) * CELL)

rx = GX + N * CELL + 60
b += core(rx + 14, GY + 34, 24)
b.append(text(rx + 44, GY + 41, "一个核心", 19, INK, anchor="start"))
b.append(text(rx, GY + 92, "一格一个点积", 19, INK, anchor="start"))
b += math(rx + 122, GY + 92, "k_i · q_j", 22, INK, anchor="start")
b.append(text(rx, GY + 120, "由一个核心计算", 19, INK, anchor="start"))
b.append(text(rx, GY + 178, "64 个点积互不依赖，", 19, INK, anchor="start"))
b.append(text(rx, GY + 206, "全部同时计算", 19, INK, anchor="start"))
b.append(text(rx, GY + 262, "注意力的大部分计算可以并行，适合 GPU", 19, ORANGE, "bold",
              anchor="start"))
save("parallel-gpu", b, H)
