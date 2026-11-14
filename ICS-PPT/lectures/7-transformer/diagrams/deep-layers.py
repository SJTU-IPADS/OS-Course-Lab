#!/usr/bin/env python3
"""deep-layers: the vectors pass through many attention and MLP blocks.

1: a stack, bottom to top: the embeddings, then attention, MLP, attention,
   MLP, ..., 96 layers in all; one row of vectors between blocks. Labels on
   the top row: what higher layers may encode (the video's list).
2: the video's poem example, Frost, "The Road Not Taken": in the line "I took
   the one less traveled by" the vector of "one" gains, layer by layer, "of
   two roads", "symbolizing choice", "contrasting the original with the
   familiar" (tip to tail, green), with Chinese glosses. Its column in the
   stack is marked.
"""
from tk import *

H = 420
X0, DX = 150, 54            # first column of the stack, spacing
NCOL = 6
VW, VH = 14, 24             # one vector
ONE = 3                     # the column of "one" in frame 2
ROWS = {"emb": 340, "r1": 282, "r2": 224, "r3": 166, "r4": 108, "top": 54}
BANDS = [("注意力", 312), ("MLP", 254), ("注意力", 196), ("MLP", 138)]
BAND_H = 22
LEFT, RIGHT = X0 - 22, X0 + (NCOL - 1) * DX + VW + 22

TAGS = [("情感", 0), ("语气", 1), ("是否为诗", 3), ("相关的科学事实", 5)]


def vectors(y, role, mark=False):
    out = []
    for k in range(NCOL):
        x = X0 + k * DX
        stroke, fill = ROLE[role]
        if mark and k == ONE:
            stroke, fill = ORANGE, FILL_ORANGE
        out.append(rect(x, y, VW, VH, fill, stroke, rx=2, width=1.6))
        for s in (1, 2):
            out.append(line(x + 2, y + s * VH / 3, x + VW - 2, y + s * VH / 3, stroke, 0.8))
    return out


def stack(mark):
    b = []
    b += vectors(ROWS["emb"], "data", mark)
    b.append(text(LEFT - 12, ROWS["emb"] + 18, "嵌入", 18, INK, anchor="end"))
    for name, y in BANDS:
        b.append(rect(LEFT, y, RIGHT - LEFT, BAND_H, WHITE, LINE, rx=5, width=1.4))
        b.append(text((LEFT + RIGHT) / 2, y + 17, name, 17, INK))
    for key in ("r1", "r2", "r3", "r4"):
        b += vectors(ROWS[key], "data", mark)
    b.append(text((LEFT + RIGHT) / 2, ROWS["top"] + VH + 22, "⋮", 20, MUTED))
    b += vectors(ROWS["top"], "data", mark)
    # layers
    for k, (lo, hi) in enumerate(((BANDS[0][1], BANDS[1][1]), (BANDS[2][1], BANDS[3][1]))):
        x = LEFT - 10
        b.append(path(f"M {x} {lo + BAND_H} L {x - 7} {lo + BAND_H} L {x - 7} {hi} L {x} {hi}",
                      MUTED, 1.4, head=False))
        b.append(text(x - 14, (lo + BAND_H + hi) / 2 + 6, f"第 {k + 1} 层", 17, MUTED,
                      anchor="end"))
    b.append(text(LEFT - 12, ROWS["top"] + VH + 20, "共 96 层", 17, MUTED, anchor="end"))
    # direction of flow
    b.append(arrow(RIGHT + 22, ROWS["emb"] + VH, RIGHT + 22, ROWS["top"], MUTED, 1.8))
    # what higher layers may encode
    x = X0 - 30
    for tag, col in TAGS:
        els, w = chip(x, 8, tag, "data", size=17, pad=7, h=28)
        b += els
        cx = X0 + col * DX + VW / 2
        b.append(line(min(max(cx, x + 8), x + w - 8), 36, cx, ROWS["top"] - 2, MUTED, 1.2))
        x += w + 8
    return b


def poem():
    b = []
    px = 540
    b.append(text(px, 32, "Frost《The Road Not Taken》", 17, MUTED, anchor="start"))
    b.append(text(px, 64, "Two roads diverged in a wood, and I—", 18, MUTED, anchor="start"))
    words = "I took the one less traveled by,".split()
    els, spans = chip_row(px, 78, words, ["hl" if w == "one" else "data" for w in words],
                          size=18, gap=6, h=32, pad=7)
    b += els
    # the vector of "one", then three additions, tip to tail
    S = (560, 396)
    P = [(626, 300), (742, 238), (836, 316), (968, 210)]
    b.append(vec(*S, *P[0], DATA, 2.6, shrink=2))
    b.append(text(P[0][0] - 12, P[0][1] + 8, "one", 19, INK, anchor="end"))
    for a, c in zip(P, P[1:]):
        b.append(vec(*a, *c, UPD, 2.6, shrink=2))
    b.append(text(P[1][0] - 12, P[1][1] - 26, "of two roads", 18, UPD, anchor="end"))
    b.append(text(P[1][0] - 12, P[1][1] - 4, "两条路中的一条", 17, MUTED, anchor="end"))
    b.append(text(P[2][0], P[2][1] + 30, "symbolizing choice", 18, UPD))
    b.append(text(P[2][0], P[2][1] + 52, "象征选择", 17, MUTED))
    b.append(text(P[3][0] + 10, P[3][1] - 62, "contrasting the original", 18, UPD, anchor="end"))
    b.append(text(P[3][0] + 10, P[3][1] - 40, "with the familiar", 18, UPD, anchor="end"))
    b.append(text(P[3][0] + 10, P[3][1] - 18, "与熟悉之路相对", 17, MUTED, anchor="end"))
    return b


def frame(n):
    b = stack(mark=(n == 2))
    if n == 2:
        b += poem()
        b.append(text(X0 + ONE * DX + VW / 2, ROWS["emb"] + VH + 24, "one", 18, ORANGE, "bold"))
    save(f"deep-layers-{n}", b, H)


for n in (1, 2):
    frame(n)
