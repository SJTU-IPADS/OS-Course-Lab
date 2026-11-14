#!/usr/bin/env python3
"""attention-preview: in an attention block the vectors exchange information.

1: a row of six vectors (grey), arrows between many pairs.
2: + the word "model" in "a machine learning model" and in "a fashion
   model": its two vectors start identical and attention moves them in
   different directions (a 2D view).
"""
from tk import *

H = 400
VALS = [[7.2, 5.0, 9.5, 6.4, 4.2], [2.9, 6.1, 4.2, 1.3, 3.0], [6.5, 4.3, 8.9, 3.6, 4.3],
        [9.1, 7.1, 9.9, 1.5, 8.6], [8.0, 5.6, 4.0, 0.7, 6.9], [8.5, 0.1, 3.6, 7.2, 1.7]]
CX0, PITCH, CW, CH, VY = 76, 74, 48, 26, 222
PAIRS = [(0, 1), (0, 2), (1, 2), (1, 3), (2, 3), (0, 4), (3, 4), (2, 5), (4, 5), (1, 5)]


def phrase(x, y, words, tag, links):
    """Chips left to right, `tag` before them, arcs from links[] to the last chip."""
    out = [label(x - 14, y + 25, tag, 20, INK, anchor="end")]
    roles = ["data"] * (len(words) - 1) + ["hl"]
    els, spans = chip_row(x, y, words, roles, size=19, gap=8, h=34)
    out += els
    tx = spans[-1][1]
    for k, i in enumerate(links):
        sx = spans[i][1]
        out.append(curve(sx, y - 3, tx - 8 + 8 * k, y - 4, bend=-22 - 8 * (len(words) - 1 - i),
                         color=INK, width=1.6))
    return out


def frame(n):
    b = []
    b.append(label(CX0 + 2.5 * PITCH, 104, "注意力：向量之间传递信息", 20, INK, "bold"))
    for k, vals in enumerate(VALS):
        els, _ = vector(CX0 + k * PITCH - CW / 2, VY, vals + ["⋮"], cw=CW, ch=CH, size=16)
        b += els
    for i, j in PAIRS:
        outs = sorted(t for s_, t in PAIRS if s_ == i)
        ins = sorted(s_ for s_, t in PAIRS if t == j)
        sx = CX0 + i * PITCH + 2 + 7 * outs.index(j)
        tx = CX0 + j * PITCH - 18 + 7 * ins.index(i)
        b.append(curve(sx, VY - 6, tx, VY - 8, bend=-30 - 24 * (j - i), color=INK, width=1.5))

    if n >= 2:
        b += phrase(620, 46, ["a", "machine", "learning", "model"], "①", [1, 2])
        b += phrase(620, 132, ["a", "fashion", "model"], "②", [1])
        # the 2D view
        x0, y0 = 590, 384
        b += axes(x0, y0, 420, 168)
        s = (680, 306)
        t1, t2 = (846, 240), (846, 352)
        b.append(vec(*s, *t1, GREEN, 2.6))
        b.append(vec(*s, *t2, GREEN, 2.6))
        b.append(dot(*s, 6, DATA))
        b.append(label(s[0] - 12, s[1] + 6, "model", 19, INK, anchor="end"))
        b.append(dot(*t1, 5, GREEN))
        b.append(dot(*t2, 5, GREEN))
        b.append(label(t1[0] + 12, t1[1] + 6, "① 数学模型、算法", 19, GREEN, anchor="start"))
        b.append(label(t2[0] + 12, t2[1] + 6, "② 时尚、走秀", 19, GREEN, anchor="start"))
    save(f"attention-preview-{n}", b, H)


for n in (1, 2):
    frame(n)
