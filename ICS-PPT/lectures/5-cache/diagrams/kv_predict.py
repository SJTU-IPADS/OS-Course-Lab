#!/usr/bin/env python3
"""How the next token is predicted from the last token's vector, with one attention layer.

The four token vectors x1..x4 enter one attention block; only the lines that carry
information from the earlier tokens into the last lane are drawn. Of the outputs only the
last lane's is used: the vector of the last token plus its attention output (x4+o4).
The output layer holds one vector per token of the vocabulary; the inner product of each
with that vector is turned into a probability by softmax (one small arrow per row), and
the most likely token is the next token. The probabilities of the three candidates and
their inner products are schematic.
Run it to refresh ../assets/kv-predict.svg.
"""

from kv_kit import *
from kv_matrix import WORDS

W, H = 1000, 212
N = 4
LANES = [80 + 28 * i for i in range(N)]   # one lane per token
TOP, BOTTOM = 38, 184                     # the two purple blocks
XX, XW, XH = 64, 70, 20                   # the token vectors
LX, LW = 176, 128                         # the attention block
HX, HW = 340, 80                          # the output of the last lane
VX, VW = 480, 150                         # the output layer
DX = 666                                  # the inner products
PX, BAR = 724, 190                        # the probability bars: left edge, length of 1
NX, NW = 912, 62                          # the next token
CANDIDATES = [("适合", 0.52), ("我们", 0.24), ("可以", 0.12)]
SCORES = [5.1, 4.3, 3.6]                  # inner products of the three candidates
HEAD = LANES[0] - 18


def block(x, w, name):
    return [rect(x, TOP, w, BOTTOM - TOP, *KINDS["param"], rx=6, width=1.6),
            text(x + w / 2, TOP + 21, name, 15, INK, "bold")]


def figure():
    last = LANES[-1]
    out = [text(XX + XW / 2, HEAD, "token 的向量", 14, MUTED, "bold"),
           text(DX, HEAD, "内积", 14, MUTED, "bold"),
           text(PX + 60, HEAD, "softmax 后的概率", 14, MUTED, "bold")]
    out += block(LX, LW, "注意力")
    for i, y in enumerate(LANES):
        out.append(text(XX - 8, y + 5, WORDS[i], 14, INK, "normal", "end"))
        out += [rect(XX, y - XH / 2, XW, XH, *KINDS["mid"], rx=2, width=1.3),
                mono(XX + XW / 2, y + 4.3, f"x{i + 1}", 12, INK, "bold", "middle")]
        out.append(arrow(XX + XW + 3, y, LX - 4, y, MUTED, 1.6))
        out.append(line(LX, y, LX + LW, y, MUTED, 1.6))
        if y != last:
            out.append(line(LX + 20, y, LX + LW - 20, last, INK, 1.4))
    out.append(arrow(LX + LW + 2, last, HX - 5, last, MUTED, 1.6))
    out.append(rect(HX, last - XH / 2, HW, XH, FILL_GREY, INK, rx=2, width=2.2))
    out.append(mono(HX + HW / 2, last + 4.3, "x4+o4", 12, INK, "bold", "middle"))
    out.append(text(HX + HW / 2, last + XH / 2 + 17, "「好」的向量", 14, INK, "bold"))

    out += block(VX, VW, "输出层")
    out.append(text(VX + VW / 2, BOTTOM + 18, "词表中每个 token 一个向量", 14, MUTED, "bold"))
    for i, (word, prob) in enumerate(CANDIDATES):
        y = LANES[i]
        best = i == 0
        out.append(arrow(HX + HW + 5, last, VX - 5, y, INK if best else MUTED,
                         2 if best else 1.4))
        out.append(text(VX + 58, y + 5, word, 14, INK, "bold" if best else "normal", "end"))
        out.append(rect(VX + 66, y - XH / 2, XW, XH, WHITE, PURPLE, rx=2, width=1.4))
        out.append(mono(DX, y + 4.6, f"{SCORES[i]:.1f}", 14, INK, "bold", "middle"))
        fill, stroke = KINDS["new"] if best else KINDS["mid"]
        out.append(arrow(DX + 18, y, PX - 8, y, MUTED, 1.4))
        out.append(rect(PX, y - 6, prob * BAR, 12, fill, stroke, rx=2, width=1.2))
        out.append(mono(PX + prob * BAR + 8, y + 4.4, f"{prob:.2f}", 13, INK,
                        "bold" if best else "normal"))
    for x in (VX + 66 + XW / 2, DX, PX + 14):
        out.append(text(x, last + 6, "⋯", 16, MUTED, "bold"))

    top = LANES[0]
    out.append(arrow(PX + CANDIDATES[0][1] * BAR + 46, top, NX - 6, top, ORANGE, 2.2))
    out += cell(NX, top - 15, NW, 30, "", "new")
    out.append(text(NX + NW / 2, top + 6, WORDS[N], 16, INK, "bold"))
    out.append(text(NX + NW / 2, top - 24, "下一个 token", 14, MUTED, "bold"))
    return out


if __name__ == "__main__":
    save("kv-predict", W, H, figure())
