#!/usr/bin/env python3
"""One vector per position, and the vector of the last position predicts the next token.

Bottom row: the tokens of the input. Middle row: the vector of each position, one per
token. Top row: the token that each vector predicts, which is the token of the next
position; the vector of position i predicts token i+1. Under the last position the
predicted token does not exist yet, so its cell is dashed and empty, and the arrow
into it is the thick one. The figure uses no colour: the point is the position, not the
content of the token.
Run it to refresh ../assets/kv-position.svg.
"""

from kv_kit import *
from kv_matrix import WORDS

W, H = 800, 196
N = 4
LX = 132                                  # right edge of the row labels
X0, CW, PITCH = 150, 84, 120              # first column: left edge, cell width, pitch
TOP, MIDDLE, BOTTOM = 14, 88, 148         # top edges of the three rows
TH, VH = 32, 26                           # token cell height, vector cell height
PREDICTED = WORDS[1:N + 1]                # the token predicted at each position


def cx(i):
    return X0 + i * PITCH + CW / 2


def figure():
    out = [text(LX, TOP + TH / 2 + 5, "预测的 token", 14, MUTED, "bold", "end"),
           text(LX, MIDDLE + VH / 2 + 5, "各位置的向量", 14, MUTED, "bold", "end"),
           text(LX, BOTTOM + TH / 2 + 5, "输入的 token", 14, MUTED, "bold", "end")]
    for i in range(N):
        last = i == N - 1
        # input token
        out += cell(X0 + i * PITCH, BOTTOM, CW, TH, "", "mid")
        out.append(text(cx(i), BOTTOM + TH / 2 + 6, WORDS[i], 17, INK, "bold"))
        # token -> vector
        out.append(arrow(cx(i), BOTTOM - 3, cx(i), MIDDLE + VH + 4, MUTED, 1.6))
        # vector of the position
        out += cell(X0 + i * PITCH, MIDDLE, CW, VH, f"x{i + 1}", "mid", size=13)
        # vector -> predicted token
        out.append(arrow(cx(i), MIDDLE - 3, cx(i), TOP + TH + 4,
                         INK if last else MUTED, 2 if last else 1.2))
        # predicted token
        out += cell(X0 + i * PITCH, TOP, CW, TH, "", "empty" if last else "mid")
        out.append(text(cx(i), TOP + TH / 2 + 6, PREDICTED[i], 17, INK, "bold"))
    out.append(text(X0 + (N - 1) * PITCH + CW + 14, TOP + TH / 2 + 5, "生成时要预测的 token",
                    14, MUTED, "normal", "start"))
    return out


if __name__ == "__main__":
    save("kv-position", W, H, figure())
