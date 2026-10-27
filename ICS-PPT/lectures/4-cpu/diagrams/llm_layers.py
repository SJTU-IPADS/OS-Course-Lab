#!/usr/bin/env python3
"""One step of the model: the vector of the input word through 28 layers.

Left to right: the input word (blue, as the prompt tokens of
llm_autoregressive.py), its vector of 2048 numbers, layers 1, 2, ..., 28
(each multiplies its own weight matrix by the vector), the scores of the
candidate words and the word taken (orange). Above every layer sits its own
weight matrix; the bracket over them gives the total that is read once per
generated word. The distribution is the step-5 one of llm_autoregressive.py.
Run it to refresh ../assets/llm-layers.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE,
                    WHITE, arrow, box, brace, line, rect, save, text)

W, H = 1120, 196
X0 = 66                             # left edge of the drawing, centres it on the canvas
LY, LH = 112, 64                    # layer boxes: top, height
MY = LY + LH / 2                    # the horizontal line everything sits on
WY, WH = 46, 40                     # weight matrices: top, height
LW, LGAP = 112, 40                  # layer box width, gap between layers
LAYERS = ["第 1 层", "第 2 层", None, "第 28 层"]   # None: the dots between
DOTS_W = 36
DIST = [("吞吐", 1.0), ("速度", 0.22), ("效率", 0.15)]


def matrix(x, y, w, h):
    """A weight matrix: a grey block ruled into a grid."""
    out = [rect(x, y, w, h, FILL_GREY, INK, rx=3, width=1.4)]
    for k in range(1, 4):
        out.append(line(x + k * w / 4, y + 3, x + k * w / 4, y + h - 3, LINE, 1))
    out.append(line(x + 3, y + h / 2, x + w - 3, y + h / 2, LINE, 1))
    return out


def vector(x, y, n=5, cw=14, ch=26):
    return [rect(x + k * cw, y, cw, ch, FILL_BLUE, BLUE, rx=1, width=1.1) for k in range(n)]


def build():
    out = []
    x = X0
    out.append(text(x + 30, MY - 34, "输入的词", 14, MUTED))
    out += box(x, MY - 22, 60, 44, "是", FILL_BLUE, BLUE, 17, "normal")
    x += 60
    out.append(arrow(x + 4, MY, x + 26, MY, INK, 2))
    x += 30
    out += vector(x, MY - 13)
    out.append(text(x + 35, MY + 34, "2048 个数", 13, MUTED))
    out.append(text(x + 35, MY - 22, "向量", 14, MUTED))
    x += 70
    left = x + 34
    for k, name in enumerate(LAYERS):
        out.append(arrow(x + 4, MY, x + LGAP - 4, MY, INK, 2))
        x += LGAP
        if name is None:
            out.append(text(x + DOTS_W / 2, MY + 6, "…", 20, INK, "bold"))
            out.append(text(x + DOTS_W / 2, WY + WH / 2 + 6, "…", 20, MUTED, "bold"))
            x += DOTS_W
            continue
        out += matrix(x + 16, WY, LW - 32, WH)
        out.append(arrow(x + LW / 2, WY + WH + 2, x + LW / 2, LY - 3, MUTED, 1.4))
        out += box(x, LY, LW, LH, name, WHITE, INK, 16, "bold", sub="权重 × 向量", sub_size=13)
        x += LW
    right = x - 16
    out += brace(left, right, WY - 8, INK,
                 "每一层有自己的权重矩阵：生成一个词，28 层的权重各读一遍",
                 15, depth=8, below=False)
    out.append(arrow(x + 4, MY, x + 30, MY, INK, 2))
    x += 36
    out.append(text(x + 70, MY - 34, "每个候选词的分数", 14, MUTED))
    for k, (word, p) in enumerate(DIST):
        by = MY - 21 + k * 14
        tone = ORANGE if k == 0 else MUTED
        out.append(text(x + 30, by + 11, word, 13, tone, anchor="end"))
        out.append(rect(x + 36, by + 1, 96 * p, 11, FILL_ORANGE if k == 0 else LINE, tone,
                        rx=2, width=1))
    x += 142
    out.append(arrow(x, MY, x + 40, MY, INK, 2))
    out.append(text(x + 20, MY - 8, "取出", 13, MUTED))
    x += 44
    out += box(x, MY - 22, 76, 44, "吞吐", FILL_ORANGE, ORANGE, 16, "bold")
    return out


if __name__ == "__main__":
    save("llm-layers", W, H, build())
