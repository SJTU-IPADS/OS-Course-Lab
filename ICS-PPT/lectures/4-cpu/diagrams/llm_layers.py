#!/usr/bin/env python3
"""One pass of the model: the input vector walks through 126 layers.

Each layer is a weight matrix times a vector. Each dense layer uses its own weights. This is an unnamed 126-layer example. Colours follow the other part-4
figures: the words so far and the vector blue, the layers (weights) grey,
the word taken orange. Run it to refresh ../assets/llm-layers.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, GREEN, INK, MUTED, ORANGE,
                    arrow, box, brace, save, text)

W, H = 1120, 210
Y, BH = 50, 64
X_IN, W_IN = 20, 118
X_VEC, W_VEC = 168, 118
LW, LGAP = 100, 14
X_L = 316
DOTS_W = 40
X_OUT_W = 104


def build():
    out = []
    out += box(X_IN, Y, W_IN, BH, "到目前为止的词", FILL_BLUE, BLUE, 15, "bold",
               sub="流水线 提高 的 是", sub_size=12)
    out.append(arrow(X_IN + W_IN + 2, Y + BH / 2, X_VEC - 2, Y + BH / 2, INK, 2))
    out += box(X_VEC, Y, W_VEC, BH, "向量", FILL_BLUE, BLUE, 16, "bold",
               sub="每个词一个向量", sub_size=12)
    out.append(arrow(X_VEC + W_VEC + 2, Y + BH / 2, X_L - 2, Y + BH / 2, INK, 2))
    x = X_L
    for k in (1, 2, 3):
        out += box(x, Y, LW, BH, f"第 {k} 层", FILL_GREY, INK, 16, "bold",
                   sub="权重 × 向量", sub_size=12)
        x += LW
        out.append(arrow(x + 2, Y + BH / 2, x + LGAP - 2, Y + BH / 2, INK, 2))
        x += LGAP
    out.append(text(x + DOTS_W / 2, Y + BH / 2 + 8, "⋯", 24, MUTED))
    x += DOTS_W
    out.append(arrow(x + 2, Y + BH / 2, x + LGAP - 2, Y + BH / 2, INK, 2))
    x += LGAP
    out += box(x, Y, LW, BH, "第 126 层", FILL_GREY, INK, 16, "bold",
               sub="权重 × 向量", sub_size=12)
    layers_right = x + LW
    x += LW
    out.append(arrow(x + 2, Y + BH / 2, x + 30 - 2, Y + BH / 2, INK, 2))
    x += 30
    out += box(x, Y, X_OUT_W, BH, "取词", FILL_ORANGE, ORANGE, 16, "bold",
               sub="「吞吐」", sub_size=13)
    out += brace(X_L, layers_right, Y + BH + 10, GREEN,
                 "各层使用各自的权重，向量依次经过 126 层", 15, depth=8)
    return out, x + X_OUT_W


if __name__ == "__main__":
    body, right = build()
    save("llm-layers", W, H, body, left=(right + 20) / 2 - W / 2)
