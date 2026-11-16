#!/usr/bin/env python3
"""full-pattern-training: training with the attention pattern as it stands.

Keys run down the left, queries across the top, as in attention-pattern.
The grid is pattern(causal=False): every column is a softmax over all 8 keys
and each cell is shaded by its weight. No cell is restricted, so nothing is
red. Each column leads by a green curve to its prediction target, the word
after it, drawn as a green chip as in training-targets; the word after forest
is not in the passage and is drawn as "…". The chips are wider than the
columns, so they stand in one row spread under the grid, in column order.

CELL is 32 and the labels are a size under those of attention-pattern, so
that the grid, the curves and the chips fit the page at full size.
"""
from tk import *
from attn_data import WORDS, N, column

H = 406
CELL = 32
GX, GY = 425, 80
TS = 16                          # target chips: font size, gap, top edge
TGAP = 6
TY = GY + N * CELL + 42


def mix(c1, c2, t):
    a = [int(c1[i:i + 2], 16) for i in (1, 3, 5)]
    b_ = [int(c2[i:i + 2], 16) for i in (1, 3, 5)]
    return "#%02x%02x%02x" % tuple(round(x + (y - x) * t) for x, y in zip(a, b_))


def round_col(ws):
    """Hundredths that add up to exactly 100 (largest remainder)."""
    raw = [w * 100 for w in ws]
    out = [int(r) for r in raw]
    for i in sorted(range(len(ws)), key=lambda i: raw[i] - out[i], reverse=True)[:100 - sum(out)]:
        out[i] += 1
    return out


def shade(h):
    """Fill of a cell holding weight h/100, as in attention-pattern."""
    return mix(WHITE, DATA, 0.08 + 0.85 * h / 100)


b = []
for j, w in enumerate(WORDS):
    cx = GX + (j + .5) * CELL
    b.append(f'<text x="{cx:.1f}" y="{GY - 33:.1f}" font-family="{FONT}" font-size="16" '
             f'fill="{INK}" text-anchor="start" transform="rotate(-35 {cx:.1f} {GY - 33:.1f})">'
             f'{w}</text>')
    b += math(cx, GY - 10, f"q_{j + 1}", 17, Q)
for i, w in enumerate(WORDS):
    cy = GY + (i + .5) * CELL + 6
    b.append(text(GX - 44, cy, w, 16, INK, anchor="end"))
    b += math(GX - 21, cy, f"k_{i + 1}", 17, K)

cols = [round_col(column(q, causal=False)) for q in range(N)]
b += grid(GX, GY, N, N, CELL, fills=[[shade(cols[j][i]) for j in range(N)] for i in range(N)])

# the prediction target of every column: the word after it
targets = WORDS[1:] + ["…"]
tw = [width(t, TS) + 14 for t in targets]
tx = GX + N * CELL / 2 - (sum(tw) + TGAP * (N - 1)) / 2
b.append(text(tx - 12, TY + 14 + 6, "预测目标", 17, GREEN, "bold", anchor="end"))
y0, y1 = GY + N * CELL + 4, TY - 4
for j in range(N):
    cx, tc = GX + (j + .5) * CELL, tx + tw[j] / 2
    ym = (y0 + y1) / 2
    b.append(path(f"M {cx:.1f} {y0:.1f} C {cx:.1f} {ym:.1f} {tc:.1f} {ym:.1f} {tc:.1f} {y1:.1f}",
                  GREEN, 1.8))
    b += chip(tx, TY, targets[j], "upd", TS, pad=7, h=28)[0]
    tx += tw[j] + TGAP

save("full-pattern-training", b, H)
