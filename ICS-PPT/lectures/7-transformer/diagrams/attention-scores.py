#!/usr/bin/env python3
"""attention-scores: the grid of key-query dot products.

Keys run down the left, queries across the top (the video's layout).
1: one dot per pair, its size the dot product; negative products are hollow
red rings.  2: the creature column picked out: fluffy and blue give large
positive products.
"""
from tk import *
from attn_data import WORDS, N, SCORES, CREATURE

H = 430
CELL = 40
GX, GY = 330, 92


def frame(n):
    b = []
    # queries across the top: word, then q_i
    for j, w in enumerate(WORDS):
        cx = GX + (j + .5) * CELL
        b.append(f'<text x="{cx:.1f}" y="{GY - 40:.1f}" font-family="{FONT}" font-size="18" '
                 f'fill="{INK}" text-anchor="start" transform="rotate(-35 {cx:.1f} {GY - 40:.1f})">'
                 f'{w}</text>')
        b += math(cx, GY - 12, f"q_{j + 1}", 19, Q)
    # keys down the left: word, then k_i
    for i, w in enumerate(WORDS):
        cy = GY + (i + .5) * CELL + 7
        b.append(text(GX - 52, cy, w, 18, INK, anchor="end"))
        b += math(GX - 24, cy, f"k_{i + 1}", 19, K)

    fills = None
    if n == 2:
        fills = [[FILL_ORANGE if j == CREATURE else WHITE for j in range(N)] for i in range(N)]
    b += grid(GX, GY, N, N, CELL, fills=fills)
    b += dots_in_grid(GX, GY, SCORES, CELL, color=INK if n == 1 else MUTED)
    if n == 2:
        for i in (1, 2):
            cx, cy = GX + (CREATURE + .5) * CELL, GY + (i + .5) * CELL
            b.append(circle(cx, cy, CELL * 0.42 * (SCORES[i][CREATURE] / 4.6) ** .5, ORANGE,
                            ORANGE, 1.2))

    # right-hand side: what one cell holds
    rx = GX + N * CELL + 60
    b += math(rx, GY + 30, "k_i · q_j", 26, INK, anchor="start")
    b.append(text(rx, GY + 66, "第 i 个键与第 j 个查询的点积", 18, MUTED, anchor="start"))
    b.append(dot(rx + 12, GY + 118, 14, INK))
    b.append(text(rx + 40, GY + 125, "较大的正值", 18, INK, anchor="start"))
    b.append(dot(rx + 12, GY + 160, 4, INK))
    b.append(text(rx + 40, GY + 167, "接近零", 18, INK, anchor="start"))
    b.append(circle(rx + 12, GY + 202, 10, WHITE, RED, 1.6))
    b.append(text(rx + 40, GY + 209, "负值", 18, RED, anchor="start"))
    if n == 2:
        b.append(text(rx, GY + 262, "creature 的查询与", 19, ORANGE, "bold", anchor="start"))
        b.append(text(rx, GY + 290, "fluffy、blue 的键方向一致", 19, ORANGE, "bold",
                      anchor="start"))
    save(f"attention-scores-{n}", b, H)


for n in (1, 2):
    frame(n)
