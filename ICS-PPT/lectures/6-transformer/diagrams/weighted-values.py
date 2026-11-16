#!/usr/bin/env python3
"""weighted-values: each column of the attention pattern weights the value
vectors, and their sum updates the embedding at the top of the column.

Keys (here: their value vectors v_i) are rows, queries (the embeddings E_j
being updated) are columns; the weights are attn_data.pattern(causal=False),
the unmasked pattern of the attention-pattern page.
1: the pattern, two decimals, near-zero entries written 0; v_1..v_8 at the
   left of the rows. Every frame names the grid under it (注意力模式), and
   the rows of frames 3 and 4 stand below that name.
2: the creature column: each v_i scaled by its weight (fluffy 0.42, blue
   0.58, the rest about 0), drawn as scaled purple columns.
3: their sum Delta E_4 (green) under the column, and E'_4 = E_4 + Delta E_4.
4: every column does the same: Delta E_1..Delta E_8 and E'_1..E'_8.
"""
from tk import *
from attn_data import WORDS, N, CREATURE, pattern

H = 420
CELL = 36         # smaller than the 40 of attention-pattern, so that the
                  # grid's name and the two rows of frame 4 fit in H
GX, GY = 236, 40
NAME_Y = GY + N * CELL + 26    # baseline of the grid's name
ROW_Y = NAME_Y + 32            # baselines of Delta E_j, then E'_j 24 px lower
P = pattern(causal=False)
GW = 14           # width of a value-vector glyph
GH = 30           # its full height
SCALED = "#c9c0e4"  # a scaled value vector: PURPLE, lighter


def fmt(w):
    return "0" if w < 0.005 else f"{w:.2f}"


def glyph(x, y, h, color, fill, segs=4):
    """A small column vector of height h, standing on y (its bottom)."""
    out = [rect(x, y - h, GW, h, fill, color, rx=1, width=1.4)]
    if h >= 12:
        for s in range(1, segs):
            yy = y - h + h * s / segs
            out.append(line(x + 2, yy, x + GW - 2, yy, color, 0.8))
    return out


def frame(n):
    b = []
    # columns: the embeddings being updated
    for j in range(N):
        b += math(GX + (j + .5) * CELL, GY - 12, f"E_{j + 1}", 19,
                  ORANGE if (n in (2, 3) and j == CREATURE) else DATA)
    # rows: the word and its value vector
    for i, w in enumerate(WORDS):
        cy = GY + (i + .5) * CELL
        b.append(text(GX - 76, cy + 7, w, 18, INK, anchor="end"))
        b += math(GX - 50, cy + 7, f"v_{i + 1}", 19, V)
        b += glyph(GX - 28, cy + GH / 2, GH, V, FILL_V)
    b += math(GX - 76, GY - 12, "v_i = W_V E_i", 20, V, anchor="end")

    fills = [[WHITE] * N for _ in range(N)]
    texts = [[fmt(P[i][j]) for j in range(N)] for i in range(N)]
    colors = [[INK if P[i][j] >= 0.005 else LINE for j in range(N)] for i in range(N)]
    if n in (2, 3):
        for i in range(N):
            fills[i][CREATURE] = FILL_ORANGE
            texts[i][CREATURE] = ""
    b += grid(GX, GY, N, N, CELL, fills=fills)
    for i in range(N):
        for j in range(N):
            if texts[i][j]:
                b.append(text(GX + (j + .5) * CELL, GY + (i + .5) * CELL + 5.5, texts[i][j],
                              15, colors[i][j]))

    cx = GX + (CREATURE + .5) * CELL
    if n in (2, 3):
        b.append(rect(GX + CREATURE * CELL, GY, CELL, N * CELL, "none", ORANGE, rx=2,
                      width=2.4))
        for i in range(N):
            w = P[i][CREATURE]
            bottom = GY + (i + .5) * CELL + GH / 2
            # the full v_i dashed, the scaled part filled
            b.append(rect(cx - GW / 2, bottom - GH, GW, GH, "none", V, rx=1, width=1,
                          dash="3 2"))
            if w < 0.005:
                b.append(line(cx - GW / 2, bottom, cx + GW / 2, bottom, V, 2))
            else:
                b.append(rect(cx - GW / 2, bottom - GH * w, GW, GH * w, SCALED, V, rx=1,
                              width=1.4))

    # right-hand side
    rx = GX + N * CELL + 56
    if n == 2:
        b += math(rx, GY + 2 * CELL + 6, "0.42 v_2 + 0.58 v_3 + ⋯", 22, V, anchor="start")
        b.append(text(rx, GY + 2 * CELL + 42, "其余各项的权重接近 0", 17, MUTED,
                      anchor="start"))
    if n >= 3:
        b += math(rx, GY + 2 * CELL + 6, "ΔE_4 = 0.42 v_2 + 0.58 v_3 + ⋯", 22, UPD,
                  anchor="start")
        # E_4 + Delta E_4 = E'_4 as columns
        top = GY + 3 * CELL + 10
        ex = rx + 10
        vals = [None] * 5
        els, vh = vector(ex, top, vals, "data", cw=36, ch=24)
        b += els
        b.append(label(ex + 70, top + vh / 2 + 10, "+", 28))
        els, _ = vector(ex + 104, top, vals, "upd", cw=36, ch=24)
        b += els
        b.append(label(ex + 174, top + vh / 2 + 10, "=", 28))
        els, _ = vector(ex + 208, top, vals, "upd", cw=36, ch=24)
        b += els
        j = "4"
        b += math(ex + 18, top + vh + 34, f"E_{j}", 21, DATA)
        b += math(ex + 122, top + vh + 34, f"ΔE_{j}", 21, UPD)
        b += math(ex + 226, top + vh + 34, f"E′_{j}", 21, UPD)

    # under the grid: its name, then the update and the new vector of each
    # column
    b.append(text(GX + N * CELL / 2, NAME_Y, "注意力模式", 20, INK, "bold"))
    cols = [CREATURE] if n == 3 else (range(N) if n == 4 else [])
    for j in cols:
        x = GX + (j + .5) * CELL
        b += math(x, ROW_Y, f"ΔE_{j + 1}", 19, UPD)
        b += math(x, ROW_Y + 24, f"E′_{j + 1}", 19, UPD)
    if n == 4:
        b.append(text(GX - 76, ROW_Y, "更新量", 17, UPD, anchor="end"))
        b.append(text(GX - 76, ROW_Y + 24, "新的向量", 17, UPD, anchor="end"))
        b.append(text(rx, ROW_Y + 20, "一个注意力头", 22, INK, "bold", anchor="start"))
    save(f"weighted-values-{n}", b, H)


for n in (1, 2, 3, 4):
    frame(n)
