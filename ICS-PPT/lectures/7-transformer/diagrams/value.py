#!/usr/bin/env python3
"""value: the value matrix maps the embedding of fluffy to a value vector.

1: W_V (12,288 x 12,288, a square) times E_2 (fluffy, grey) gives v_2
   (purple), a vector as tall as E_2: both live in the embedding space.
2: a 2D view of the embedding space: v_2 added tip to tail to E_4
   (creature); the sum points to "fluffy creature" (green).
3: what a value vector stands for, in a panel tied to v_2.

The entries of E_2 and v_2 are the ones the video shows; W_V is shaded
from fixed pseudo-random values.
"""
from tk import *

H = 400

E2 = [9.2, -2.3, 5.8, 0.6, 1.3, 8.4, "⋮", -8.2]
V2 = [-52.4, 89.3, -80.2, -17.8, 7.3, 223.8, "⋮", -41.0]

CELL = 30
GX, GY = 60, 86                    # W_V
EX = GX + 8 * CELL + 34            # E_2 column
VX = EX + 56 + 74                  # v_2 column
BOT = GY + 8 * CELL                # bottom of the three

O = (612, 382)                     # origin of the 2D view
A = (O[0] + 100, O[1] - 170)       # E_4
B = (O[0] + 300, O[1] - 200)       # E_4 + v_2


def frame(n):
    b = []
    # ---- 1: W_V E_2 = v_2
    b += weights(GX, GY, 8, 8, CELL, CELL, seed="WV")
    b += math(GX + 4 * CELL, BOT + 34, "W_V", 26, V)
    b.append(note(GX + 4 * CELL, BOT + 62, "12,288 × 12,288"))

    els, _ = vector(EX, GY, E2, "data", cw=56, ch=CELL, size=15)
    b += els
    els, _ = chip(EX + 28, 20, "fluffy", "data", anchor="middle")
    b += els
    b.append(arrow(EX + 28, 58, EX + 28, GY - 8, DATA, 1.8))
    b += math(EX + 28, BOT + 34, "E_2", 24, DATA)
    b.append(note(EX + 28, BOT + 62, "12,288 维"))

    b.append(label(EX + 56 + 37, GY + 4 * CELL + 10, "=", 30))

    els, _ = vector(VX, GY, V2, "v", cw=56, ch=CELL, size=15)
    b += els
    b += math(VX + 28, BOT + 34, "v_2", 24, V)
    b.append(note(VX + 28, BOT + 62, "12,288 维"))

    # ---- 2: E_4 + v_2 in the embedding space
    if n >= 2:
        b += axes(O[0], O[1], 400, 200)
        b.append(vec(*O, *A, DATA, 2.6, shrink=2))
        b.append(vec(*A, *B, V, 2.6, shrink=2))
        b.append(vec(*O, *B, UPD, 2.6, shrink=2))
        b.append(text(A[0] - 16, A[1] + 8, "creature", 19, INK, anchor="end"))
        b += math((O[0] + A[0]) / 2 - 14, (O[1] + A[1]) / 2 + 6, "E_4", 21, DATA,
                  anchor="end")
        b += math((A[0] + B[0]) / 2, (A[1] + B[1]) / 2 - 16, "v_2", 21, V)
        b.append(text(B[0] - 60, B[1] + 80, "fluffy creature", 19, UPD, "bold",
                      anchor="start"))
        b += math(B[0] - 60, B[1] + 108, "E_4 + v_2", 20, UPD, anchor="start")

    # ---- 3: the question a value vector answers
    if n >= 3:
        px, py, pw, ph = 612, 22, 400, 112
        b.append(rect(px, py, pw, ph, FILL_V, V, rx=6, width=1.6))
        b.append(text(px + 18, py + 34, "值向量回答：", 19, V, "bold", anchor="start"))
        b.append(text(px + 18, py + 66, "若这个词与另一个词相关，", 19, INK, anchor="start"))
        b.append(text(px + 18, py + 94, "应向那个词的嵌入加上什么", 19, INK, anchor="start"))
        b.append(line(VX + 62, GY + 2 * CELL, px - 4, py + ph / 2, V, 1.6, dash="5 4"))
    save(f"value-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
