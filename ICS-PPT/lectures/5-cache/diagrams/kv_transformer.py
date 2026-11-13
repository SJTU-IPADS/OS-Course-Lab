#!/usr/bin/env python3
"""The data flow of a Transformer: token vectors pass through L layers.

Each layer is an attention block followed by a feed-forward block. In the attention block
the vector of a token takes information from the tokens before it (the slanted lines run
from an earlier token to a later one only); in the feed-forward block every vector goes
through the same transformation alone. The last vector of the last layer goes to the
output layer, which gives the next token. Blocks that hold weights are purple, the token
vectors are grey. Run it to refresh ../assets/kv-transformer.svg.
"""

from kv_kit import *
from kv_matrix import WORDS
from svgkit import box

W, H, LEFT = 950, 216, -8
N = 4
LANES = [82 + 26 * i for i in range(N)]   # one lane per token
XX, XW, XH = 66, 72, 18                   # the token vectors
BW, TOP, BOTTOM = 108, 40, 178            # a block: width, top, bottom
A1, F1, AL, FL = 176, 306, 470, 600       # left edges: attention and feed-forward, layers 1 and L
OUT_X, OUT_W = 746, 80                    # the output layer
NX, NW = 858, 62                          # the next token


def block(x, name):
    out = [rect(x, TOP, BW, BOTTOM - TOP, *KINDS["param"], rx=6, width=1.6),
           text(x + BW / 2, TOP + 21, name, 15, INK, "bold")]
    return out


def attention(x):
    """Every lane runs through; lane j also feeds each later lane i."""
    out = block(x, "注意力")
    for i, y in enumerate(LANES):
        out.append(line(x, y, x + BW, y, MUTED, 1.6))
        for earlier in LANES[:i]:
            out.append(line(x + 18, earlier, x + BW - 18, y, MUTED, 1.1))
    return out


def feed_forward(x):
    """Every lane goes through the same small box, alone."""
    out = block(x, "前馈网络")
    for y in LANES:
        out.append(line(x, y, x + BW, y, MUTED, 1.6))
        out.append(rect(x + BW / 2 - 15, y - 8, 30, 16, WHITE, PURPLE, rx=3, width=1.4))
    return out


def figure():
    out = []
    out.append(text(XX + XW / 2, LANES[0] - 20, "token 的向量", 14, MUTED, "bold"))
    for i, y in enumerate(LANES):
        out.append(text(XX - 8, y + 5, WORDS[i], 14, INK, "normal", "end"))
        out += [rect(XX, y - XH / 2, XW, XH, *KINDS["mid"], rx=2, width=1.3),
                mono(XX + XW / 2, y + 4.3, f"x{i + 1}", 12, INK, "bold", "middle")]
        out.append(arrow(XX + XW + 3, y, A1 - 4, y, MUTED, 1.6))
        out.append(arrow(A1 + BW + 2, y, F1 - 4, y, MUTED, 1.6))
        out.append(line(F1 + BW + 2, y, F1 + BW + 16, y, MUTED, 1.6))
        out.append(arrow(AL - 16, y, AL - 4, y, MUTED, 1.6))
        out.append(arrow(AL + BW + 2, y, FL - 4, y, MUTED, 1.6))
    out += attention(A1) + feed_forward(F1) + attention(AL) + feed_forward(FL)
    out.append(text((F1 + BW + AL) / 2, (LANES[0] + LANES[-1]) / 2 + 6, "⋯", 22, MUTED, "bold"))
    out += brace(A1, F1 + BW, TOP - 6, INK, "第 1 层", 15, below=False)
    out += brace(AL, FL + BW, TOP - 6, INK, "第 L 层", 15, below=False)
    out += brace(A1, FL + BW, BOTTOM + 8, MUTED, "共 L 层，结构相同，权重不同", 14)

    last = LANES[-1]
    out.append(arrow(FL + BW + 2, last, OUT_X - 5, last, MUTED, 1.6))
    out += box(OUT_X, last - 17, OUT_W, 34, "输出层", fill=FILL_PURPLE, stroke=PURPLE, size=15)
    out.append(arrow(OUT_X + OUT_W + 2, last, NX - 6, last, ORANGE, 2.2))
    out += cell(NX, last - 15, NW, 30, "", "new")
    out.append(text(NX + NW / 2, last + 6, WORDS[N], 16, INK, "bold"))
    out.append(text(NX + NW / 2, last - 26, "下一个 token", 14, MUTED, "bold"))

    # The legend stands in the free corner above the output layer, one entry per row.
    for row, item in enumerate([("param", "含有模型参数（权重）"), ("mid", "token 的向量")]):
        out += legend(OUT_X, TOP + 22 + 26 * row, [item])
    return out


if __name__ == "__main__":
    save("kv-transformer", W, H, figure(), left=LEFT)
