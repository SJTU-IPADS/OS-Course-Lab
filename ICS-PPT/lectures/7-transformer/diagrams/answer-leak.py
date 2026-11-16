#!/usr/bin/env python3
"""answer-leak: with later words free to act on earlier ones, the answer
reaches the column that has to predict it.

Left: the passage as chips. creature is the query (orange); its prediction
target is roamed (green), the word right after it. A red arc carries roamed's
information into creature's embedding, and under creature its target is the
same word, roamed: what the column has to predict is what flowed into it.

Right: the pattern that shortcut amounts to, in the layout of
attention-pattern (keys down the left, queries across the top). The cells
where the key comes after the query (row > column) are red, as in
causal-mask frame 1. In every column the weight sits on the next token
(row = column + 1): those cells are dark red and hold 1.00, every other cell
of the column holds 0. forest has no next token in the passage, and its
column is drawn as an even spread (1/8 each). The creature column is
outlined in orange and the roamed key is green, so the dark cell where they
meet is the arc of the left panel. The legend stands in the free corner
above the key labels.

CELL is 34, so that both panels fit across the slide; the labels have the
sizes of full-pattern-training.
"""
from tk import *
from attn_data import WORDS, N, CREATURE

H = 358
CELL = 34
GX, GY = 703, 82
ROAMED = CREATURE + 1

CS, CPAD, CGAP = 16, 7, 6         # the passage chips
LX = 34
CY = 190                          # their top; the panel is centred on the grid


def mix(c1, c2, t):
    a = [int(c1[i:i + 2], 16) for i in (1, 3, 5)]
    b_ = [int(c2[i:i + 2], 16) for i in (1, 3, 5)]
    return "#%02x%02x%02x" % tuple(round(x + (y - x) * t) for x, y in zip(a, b_))


def shade(h):
    """Fill of a cell holding weight h/100, as in attention-pattern."""
    return mix(WHITE, DATA, 0.08 + 0.85 * h / 100)


b = []

# ---- left: the passage, the leak into creature, creature's target
roles = ["data"] * N
roles[CREATURE], roles[ROAMED] = "q", "upd"
els, spans = chip_row(LX, CY, WORDS, roles, CS, CGAP, 34, CPAD)
b += els
cl, cc, cr = spans[CREATURE]
rl, rc, rr = spans[ROAMED]
b.append(curve(rc, CY - 6, cc + 4, CY - 6, bend=-46, color=RED, width=2.4))
b.append(text((cc + rc) / 2, CY - 58, "roamed 的信息加到 creature 的嵌入上", 17, RED, "bold"))
ty = CY + 34 + 40
b.append(arrow(cc, CY + 34 + 6, cc, ty - 6, GREEN, 2))
els, tw = chip(cc, ty, "roamed", "upd", CS, CPAD, 34, anchor="middle")
b += els
b.append(text(cc - tw / 2 - 12, ty + 23, "预测目标", 17, GREEN, "bold", anchor="end"))

# ---- right: the pattern of the shortcut
for j, w in enumerate(WORDS):
    cx = GX + (j + .5) * CELL
    hot = j == CREATURE
    b.append(f'<text x="{cx:.1f}" y="{GY - 34:.1f}" font-family="{FONT}" font-size="16" '
             f'font-weight="{"bold" if hot else "normal"}" fill="{Q if hot else INK}" '
             f'text-anchor="start" transform="rotate(-35 {cx:.1f} {GY - 34:.1f})">{w}</text>')
    b += math(cx, GY - 10, f"q_{j + 1}", 17, Q)
for i, w in enumerate(WORDS):
    cy = GY + (i + .5) * CELL + 6
    hot = i == ROAMED
    b.append(text(GX - 44, cy, w, 16, GREEN if hot else INK, "bold" if hot else "normal",
                  anchor="end"))
    b += math(GX - 21, cy, f"k_{i + 1}", 17, K)


def fill(i, j):
    if i == j + 1:
        return RED
    if i > j:
        return FILL_RED
    return shade(100 / N if j == N - 1 else 0)


b += grid(GX, GY, N, N, CELL, fills=[[fill(i, j) for j in range(N)] for i in range(N)])
for j in range(N - 1):
    b.append(text(GX + (j + .5) * CELL, GY + (j + 1.5) * CELL + 5, "1.00", 14, WHITE, "bold"))
b.append(rect(GX + CREATURE * CELL, GY, CELL, N * CELL, "none", ORANGE, rx=0, width=2.4))

# the legend, in the free corner above the key labels
lx = GX - 196
for k, (f, s) in enumerate(((FILL_RED, "键在查询之后"), (RED, "下一个 token 的键"))):
    ly = 26 + 30 * k
    b.append(rect(lx, ly - 17, 22, 22, f, LINE, rx=0, width=1))
    b.append(text(lx + 30, ly, s, 17, INK, anchor="start"))

save("answer-leak", b, H)
