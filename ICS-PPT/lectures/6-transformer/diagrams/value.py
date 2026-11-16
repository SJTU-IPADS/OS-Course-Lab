#!/usr/bin/env python3
"""value: the value matrix maps the embedding of fluffy to a value vector,
which is added to the embedding of creature.

Top: "a fluffy blue creature roamed the verdant forest" (the chips of
query.py and key.py): fluffy in purple (it gives the value vector), creature
in orange (the token being updated, as in query-2), and a purple arc v_2
from fluffy to creature.
Left: W_V (12,288 x 12,288, a square) times E_2 (fluffy's embedding, grey,
under its chip) gives v_2 (purple), a vector as tall as E_2: both live in
the embedding space.
Right: what a value vector stands for, in a panel tied to v_2; under it a 2D
view of the embedding space: v_2 added tip to tail to E_4 (creature); the
sum points to "fluffy creature" (green).

The entries of E_2 and v_2 are the ones the video shows; W_V is shaded
from fixed pseudo-random values.
"""
from tk import *
from attn_data import WORDS, CREATURE

H = 408
FLUFFY = 1

E2 = [9.2, -2.3, 5.8, 0.6, 1.3, 8.4, "⋮", -8.2]
V2 = [-52.4, 89.3, -80.2, -17.8, 7.3, 223.8, "⋮", -41.0]

CELL = 26
GX, GY = 40, 130                   # W_V
EX = GX + 8 * CELL + 34            # E_2 column
VX = EX + 56 + 74                  # v_2 column
BOT = GY + 8 * CELL                # bottom of the three

CS, GAP = 19, 14                   # the sentence: chips of query.py
CHIP_Y = 62
# fluffy's chip stands over E_2
X0 = EX + 28 - (width(WORDS[0], CS) + 20 + GAP + (width(WORDS[1], CS) + 20) / 2)

PX, PY, PW, PH = 600, 120, 400, 80  # the panel
O = (580, 398)                      # origin of the 2D view
A = (O[0] + 100, O[1] - 150)        # E_4
B = (O[0] + 300, O[1] - 176)        # E_4 + v_2


def figure():
    b = []
    # ---- the sentence: fluffy gives v_2 to creature
    roles = ["data"] * len(WORDS)
    roles[FLUFFY], roles[CREATURE] = "v", "hl"
    els, spans = chip_row(X0, CHIP_Y, WORDS, roles, size=CS, gap=GAP)
    b += els
    fx, cx = spans[FLUFFY][1], spans[CREATURE][1]
    b.append(curve(fx, CHIP_Y - 3, cx, CHIP_Y - 4, bend=-52, color=V, width=2))
    b += math((fx + cx) / 2, CHIP_Y - 40, "v_2", 21, V)

    # ---- W_V E_2 = v_2
    b += weights(GX, GY, 8, 8, CELL, CELL, seed="WV")
    b += math(GX + 4 * CELL, BOT + 34, "W_V", 26, V)
    b.append(note(GX + 4 * CELL, BOT + 62, "12,288 × 12,288"))

    els, _ = vector(EX, GY, E2, "data", cw=56, ch=CELL, size=15)
    b += els
    b.append(arrow(fx, CHIP_Y + 38, fx, GY - 8, DATA, 1.8))
    b += math(EX + 28, BOT + 34, "E_2", 24, DATA)
    b.append(note(EX + 28, BOT + 62, "12,288 维"))

    b.append(label(EX + 56 + 37, GY + 4 * CELL + 10, "=", 30))

    els, _ = vector(VX, GY, V2, "v", cw=56, ch=CELL, size=15)
    b += els
    b += math(VX + 28, BOT + 34, "v_2", 24, V)
    b.append(note(VX + 28, BOT + 62, "12,288 维"))

    # ---- the question a value vector answers
    b.append(rect(PX, PY, PW, PH, FILL_V, V, rx=6, width=1.6))
    head = "值向量回答："
    b.append(text(PX + 18, PY + 32, head, 19, V, "bold", anchor="start"))
    b.append(text(PX + 18 + width(head, 19), PY + 32, "若这个词与另一个词相关，", 19, INK,
                  anchor="start"))
    b.append(text(PX + 18, PY + 62, "应向那个词的向量加上什么", 19, INK, anchor="start"))
    b.append(line(VX + 62, GY + 2 * CELL, PX - 4, PY + PH / 2, V, 1.6, dash="5 4"))

    # ---- E_4 + v_2 in the embedding space
    b += axes(O[0], O[1], 400, 186)
    b.append(vec(*O, *A, DATA, 2.6, shrink=2))
    b.append(vec(*A, *B, V, 2.6, shrink=2))
    b.append(vec(*O, *B, UPD, 2.6, shrink=2))
    b.append(text(A[0] - 16, A[1] + 8, "creature", 19, INK, anchor="end"))
    b += math((O[0] + A[0]) / 2 - 14, (O[1] + A[1]) / 2 + 6, "E_4", 21, DATA,
              anchor="end")
    b += math((A[0] + B[0]) / 2, (A[1] + B[1]) / 2 - 16, "v_2", 21, V)
    b.append(text(B[0] - 60, B[1] + 74, "fluffy creature", 19, UPD, "bold",
                  anchor="start"))
    b += math(B[0] - 60, B[1] + 102, "E_4 + v_2", 20, UPD, anchor="start")
    save("value", b, H)


figure()
