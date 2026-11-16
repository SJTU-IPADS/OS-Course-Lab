#!/usr/bin/env python3
"""mlp-preview: every vector goes through the same MLP, on its own.

Six vectors (grey), each through its own copy of one MLP, no exchange
between columns; the results (green). A close-up of the last column: questions
asked of the vector, the answers as yes/no bars, and the update they produce,
added back to the vector.
"""
from tk import *

H = 400
X = [[7.2, 5.0, 9.5, 6.4], [2.9, 6.1, 4.2, 1.3], [6.5, 4.3, 8.9, 3.6],
     [9.1, 7.1, 9.9, 1.5], [8.0, 5.6, 4.0, 0.7], [5.4, 7.1, 6.0, 5.4]]
UPD = [[0.4, -0.8, 0.2, 1.1], [-0.3, 0.5, 1.4, -0.6], [0.9, 0.1, -1.0, 0.7],
       [-0.5, 1.2, -0.4, 0.8], [1.1, -0.4, 0.6, 0.2], [0.6, -1.2, 0.3, 2.1]]
CX0, PITCH, CW, CH = 66, 74, 46, 22
IY, BY, OY = 24, 168, 248          # inputs, MLP boxes, outputs
QS = [("是英语吗？", 0.92, "是"), ("与动物有关吗？", 0.08, "否"),
      ("是名词吗？", 0.85, "是"), ("带有否定吗？", 0.04, "否")]


def figure():
    b = []
    for k in range(6):
        cx = CX0 + k * PITCH
        els, _ = vector(cx - CW / 2, IY, X[k] + ["⋮"], cw=CW, ch=CH, size=15)
        b += els
        b.append(arrow(cx, IY + 5 * CH + 6, cx, BY - 4, INK, 1.8))
        hl = k == 5
        b.append(rect(cx - 28, BY, 56, 44, FILL_BLUE, ORANGE if hl else BLUE, rx=6,
                      width=2.4 if hl else 1.6))
        b.append(label(cx, BY + 19, "多层", 15, INK, "bold"))
        b.append(label(cx, BY + 37, "感知机", 15, INK, "bold"))
        b.append(arrow(cx, BY + 50, cx, OY - 4, INK, 1.8))
        out = [round(a + u, 1) for a, u in zip(X[k], UPD[k])]
        els, _ = vector(cx - CW / 2, OY, out + ["⋮"], "upd", cw=CW, ch=CH, size=15)
        b += els
    b.append(label(CX0 + 2.5 * PITCH, 392, "同一个多层感知机", 19, INK))

    px, py, pw, ph = 530, 22, 490, 372
    bx = CX0 + 5 * PITCH + 28
    b.append(line(bx + 2, BY, px, py + 8, ORANGE, 1.4, dash="5 5"))
    b.append(line(bx + 2, BY + 44, px, py + ph - 8, ORANGE, 1.4, dash="5 5"))
    b.append(rect(px, py, pw, ph, WHITE, ORANGE, rx=8, width=1.6))
    # the vector
    vx, vy = px + 24, 110
    els, _ = vector(vx, vy, X[5] + ["⋮"], cw=CW, ch=26, size=16)
    b += els
    b += math(vx + CW / 2, vy - 16, "x", 22)
    # questions, wired to every entry of the vector
    qx = px + 150
    qy = [92, 140, 188, 236]
    for i in range(4):
        for j, y in enumerate(qy):
            b.append(line(vx + CW + 3, vy + 13 + 26 * i, qx - 12, y - 6,
                          weight_fill(pseudo("mlpq", i, j) * 1.4), 1.3))
    for (q, v, ans), y in zip(QS, qy):
        b.append(circle(qx - 8, y - 6, 4, WHITE, MUTED, 1.4))
        b.append(label(qx + 2, y, q, 18, INK, anchor="start"))
        ax = qx + 138
        b.append(rect(ax, y - 15, 60, 16, WHITE, LINE, rx=2, width=1.2))
        b.append(rect(ax, y - 15, max(3, 60 * v), 16, FILL_DATA, DATA, rx=2, width=1.2))
        b.append(label(ax + 70, y, ans, 17, INK, anchor="start"))
    b.append(label(qx + 60, 274, "⋮", 20, MUTED))
    # the update, added back
    ux = px + pw - 24 - CW
    b.append(arrow(qx + 234, 165, ux - 8, 165, INK, 2))
    els, _ = vector(ux, vy, UPD[5] + ["⋮"], "upd", cw=CW, ch=26, size=16)
    b += els
    b.append(label(ux + CW / 2, vy - 12, "更新量", 17, GREEN, "bold"))
    b.append(curve(ux + CW / 2, vy + 5 * 26 + 8, vx + CW / 2, vy + 5 * 26 + 8, bend=120,
                   color=GREEN, width=2))
    b.append(label(px + pw / 2 + 10, 352, "＋ 加回原来的向量", 18, GREEN, "bold"))
    save("mlp-preview", b, H)


figure()
