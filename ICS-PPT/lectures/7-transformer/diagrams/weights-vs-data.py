#!/usr/bin/env python3
"""weights-vs-data: one weight matrix, two inputs.

One frame. The same W (blue/red) multiplies the vector x taken from one
sentence and the vector x' taken from another (grey); the two products Wx
and Wx' differ (grey). The products are computed from the drawn entries.
"""
from tk import *

H = 380
N, CELL = 6, 30
WX0, WY0 = 430, 62
X1 = [2.1, -0.4, 1.3, 0.8, -1.5, 0.6]
X2 = [-0.7, 1.8, 0.2, -1.1, 0.9, 1.6]
S1 = ["To date, the cleverest thinker", "of all time was"]
S2 = ["Behold, a wild pi creature,", "foraging in its native"]
CW = 48


def product(x):
    return [round(sum(pseudo("wvd", i, j) * x[j] for j in range(N)), 1) for i in range(N)]


def sentence(cx, cy, lines, size=17, lh=22):
    w = max(width(s, size) for s in lines) + 28
    h = lh * len(lines) + 14
    out = [rect(cx - w / 2, cy - h / 2, w, h, FILL_DATA, DATA, rx=6, width=1.4)]
    for i, s in enumerate(lines):
        out.append(label(cx, cy - h / 2 + 7 + lh * (i + .5) + size * 0.36, s, size, INK))
    return out


b = []
b += weights(WX0, WY0, N, N, CELL, CELL, seed="wvd")
b += math(WX0 + N * CELL / 2, WY0 + N * CELL + 36, "W", 28)
b.append(label(WX0 + N * CELL / 2, WY0 + N * CELL + 72, "权重：训练得到，决定模型的行为", 18, BLUE,
               "bold"))

LX, RX = 72, 680              # left edges of the input and output vectors
for vy, x, s_cy, lines, name in ((112, X1, 52, S1, "x"), (248, X2, 340, S2, "x′")):
    b += sentence(LX + 3 * CW, s_cy, lines)
    els, _ = hvector(LX, vy, x, cw=CW, ch=30, size=16)
    b += els
    b += math(LX - 14, vy + 22, name, 24, INK, anchor="end")
    b.append(arrow(LX + 3 * CW, s_cy + (31 if name == "x" else -31),
                   LX + 3 * CW, vy + (-7 if name == "x" else 37), MUTED, 1.6))
    wy = WY0 + (2 if name == "x" else 4) * CELL
    b.append(arrow(LX + N * CW + 10, vy + 15, WX0 - 10, wy, INK, 2))
    b.append(arrow(WX0 + N * CELL + 10, wy, RX - 10, vy + 15, INK, 2))
    els, _ = hvector(RX, vy, product(x), cw=CW, ch=30, size=16)
    b += els
    b += math(RX + N * CW + 14, vy + 22, "W" + name, 24, INK, anchor="start")
b.append(label(LX + 3 * CW, 192, "数据：本次运行处理的输入", 18, DATA, "bold"))
save("weights-vs-data", b, H)
