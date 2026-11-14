#!/usr/bin/env python3
"""weights-data: a weight matrix (blue/red entries) times a data vector (grey).

Frame 1 shows the product. Frame 2 picks one row: the weighted sum that
gives one output component.
"""
from tk import *

H = 380


def frame(n):
    b = []
    rows, cols, cell = 8, 10, 26
    mx, my = 120, 40
    hl = 2  # the row whose weighted sum is shown in frame 2
    b += weights(mx, my, rows, cols, cell, cell, seed="wd",
                 active=(lambda i, j: i == hl) if n == 2 else None, dim=(n == 2))
    b += math(mx + cols * cell / 2, my + rows * cell + 40, "W", 28)
    b.append(label(mx + cols * cell / 2, my + rows * cell + 72, "权重：训练得到", 18, BLUE))

    # the data vector, grey
    vx = mx + cols * cell + 50
    vals = [0.8, -1.2, 0.4, 2.1, -0.3, 1.0, 0.6, -0.9, 1.5, 0.2]
    els, vh = vector(vx, my - 26, vals, "data", cw=56, ch=26, size=15)
    b += els
    b += math(vx + 28, my - 26 + vh + 34, "x", 28)
    b.append(label(vx + 28, my - 26 + vh + 64, "数据：本次输入", 18, DATA))

    b.append(label(vx + 100, my + rows * cell / 2 + 10, "=", 34, INK))

    # the output vector
    ox = vx + 140
    out = [1.9, -0.4, 3.2, 0.7, -1.6, 0.5, 2.4, -0.8]
    shown = out if n == 1 else [None if i != hl else out[i] for i in range(rows)]
    els, _ = vector(ox, my, [v if v is not None else "" for v in shown], "data",
                    cw=56, ch=cell, size=15)
    b += els
    b += math(ox + 28, my + rows * cell + 40, "Wx", 28)

    if n == 2:
        # the row and the sum it stands for
        y = my + hl * cell
        b.append(rect(mx - 4, y - 3, cols * cell + 8, cell + 6, "none", ORANGE, rx=3, width=2.4))
        b.append(rect(ox - 4, y - 3, 64, cell + 6, "none", ORANGE, rx=3, width=2.4))
        b += math(ox + 100, y + 22, "= w_{3,1}x_1 + w_{3,2}x_2 + ⋯ + w_{3,10}x_{10}", 24,
                  INK, anchor="start")
        b.append(label(ox + 100, y + 64, "输出的每个分量是输入分量的加权和", 18, MUTED,
                       anchor="start"))
    save(f"weights-data-{n}", b, H)


for n in (1, 2):
    frame(n)
