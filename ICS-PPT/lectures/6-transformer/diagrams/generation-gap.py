#!/usr/bin/env python3
"""generation-gap: the shortcut learned in training has nothing to read at
generation time.

Two rows, the first four chips at the same x in both; creature is the query
(orange) in both.
训练时: the whole passage; roamed (green, the target of creature) sends its
   information to creature along a red arc, and creature reads its answer
   from it.
推理时: only the prefix "a fluffy blue creature" exists. The token to be
   generated is a dashed empty chip "?" and nothing follows it; the arc from
   it is dashed grey and crossed out in red, since there is no token there to
   read.
"""
from tk import *
from attn_data import WORDS, CREATURE

H = 300
CS, CGAP = 20, 16
X0 = 214
R1, R2 = 60, 206                 # chip tops of the two rows
ROAMED = CREATURE + 1


def row_label(y, s):
    return text(X0 - 30, y + 18 + 7, s, 20, INK, "bold", anchor="end")


b = []

# ---- training: the whole passage, the answer flows into creature
roles = ["data"] * len(WORDS)
roles[CREATURE], roles[ROAMED] = "q", "upd"
els, spans = chip_row(X0, R1, WORDS, roles, CS, CGAP)
b += els
cc, rc = spans[CREATURE][1], spans[ROAMED][1]
b.append(curve(rc, R1 - 6, cc + 4, R1 - 6, bend=-44, color=RED, width=2.4))
b.append(text(cc, R1 + 36 + 30, "读出 roamed，预测正确", 18, GREEN, "bold"))
b.append(row_label(R1, "训练时"))

# ---- generation: the prefix only; the next token does not exist yet
els, spans2 = chip_row(X0, R2, WORDS[:CREATURE + 1], roles[:CREATURE + 1], CS, CGAP)
b += els
gl = spans2[CREATURE][2] + CGAP
gw = 56
b.append(rect(gl, R2, gw, 36, WHITE, MUTED, rx=4, width=1.6, dash="5 4"))
b.append(text(gl + gw / 2, R2 + 18 + 7, "?", 20, MUTED, "bold"))
b.append(text(gl + gw + 14, R2 + 18 + 6, "待生成", 17, MUTED, anchor="start"))
gc = gl + gw / 2
b.append(curve(gc, R2 - 6, cc + 4, R2 - 6, bend=-44, color=MUTED, width=2, dash="6 4"))
mx, my = (gc + cc) / 2, R2 - 6 - 22          # the top of the arc
d = 8
b.append(line(mx - d, my - d, mx + d, my + d, RED, 3))
b.append(line(mx - d, my + d, mx + d, my - d, RED, 3))
b.append(text(cc, R2 + 36 + 30, "没有可读的下一个 token", 18, RED, "bold"))
b.append(row_label(R2, "推理时"))

save("generation-gap", b, H)
