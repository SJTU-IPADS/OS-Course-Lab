#!/usr/bin/env python3
"""context-size: the data GPT-3 processes is an array of 2,048 columns.

One frame. A row of tokens over a grid of grey columns, one column per token:
2,048 columns across (the context length), 12,288 numbers down. Each column
starts as a column plucked from W_E. Tokens before the window are faded:
they take no part in this prediction.
"""
from tk import *

H = 300
X0 = 262            # left edge of the window
PITCH, CW = 16, 13  # column pitch and width
NROW, RH = 12, 13   # rows of the grid and their height
GY = 84             # top of the grid
GROUP = 18          # columns drawn on each side of the gap
GAP = 44


def colx(j):
    """Left edge of drawn column j (0 .. 2 * GROUP - 1)."""
    return X0 + j * PITCH + (GAP if j >= GROUP else 0)


def main():
    b = []
    ncol = 2 * GROUP
    right = colx(ncol - 1) + CW
    bottom = GY + NROW * RH
    # the tokens: outside the window faded, inside one per column
    for k in range(10):
        x = 40 + k * PITCH * 1.1
        b.append(rect(x, 46, CW, 16, WHITE, LINE, rx=3, width=1.2, dash="2 2"))
    for j in range(ncol):
        x = colx(j)
        b.append(rect(x, 46, CW, 16, FILL_DATA, DATA, rx=3, width=1.2))
        b.append(line(x + CW / 2, 64, x + CW / 2, GY - 4, LINE, 1))
    b.append(text(X0 + GROUP * PITCH + GAP / 2 - 2, 60, "⋯", 18, MUTED))
    b.append(line(X0 - 14, 30, X0 - 14, bottom + 6, ORANGE, 1.6, dash="5 4"))
    b.append(note(40, 96, "窗口之外的文本", anchor="start"))
    b.append(note(40, 120, "不参与这次预测", anchor="start"))
    # the array of data
    for j in range(ncol):
        x = colx(j)
        for i in range(NROW):
            shade = 0.25 + 0.5 * (pseudo("ctx", i, j) + 1) / 2
            g = round(255 - shade * 70)
            b.append(rect(x, GY + i * RH, CW, RH, f"#{g - 8:02x}{g - 2:02x}{g:02x}", WHITE,
                          rx=0, width=0.8))
        b.append(rect(x, GY, CW, NROW * RH, "none", DATA, rx=1, width=1))
    for i in range(0, NROW, 2):
        b.append(text(X0 + GROUP * PITCH + GAP / 2 - 2, GY + (i + 1) * RH + 5, "⋯", 17,
                      MUTED))
    b += brace(X0, right, bottom + 8, INK, "2,048 列（上下文长度）", size=19)
    b += vbrace(right + 10, GY, bottom, INK, "12,288 维", size=19)
    # every column starts as a column of W_E
    b.append(rect(X0 - 3, GY - 3, CW + 6, NROW * RH + 6, "none", ORANGE, rx=2, width=2.4))
    wx, wy = 92, 162
    b += weights(wx, wy, 6, 10, 10, 10, seed="ctx-WE")
    b += math(wx - 10, wy + 38, "W_E", 24, anchor="end")
    b.append(arrow(wx + 104, wy + 30, X0 - 6, wy + 30, ORANGE, 2))
    b.append(label((wx + 104 + X0 - 6) / 2 - 6, wy + 20, "取出", 17, ORANGE))
    b.append(note(wx + 44, wy + 92, "每一列最初取自嵌入矩阵"))
    save("context-size", b, H)


main()
