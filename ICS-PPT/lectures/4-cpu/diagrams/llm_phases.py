#!/usr/bin/env python3
"""Prefill and decode as two shapes of the same matrix product.

Left: the weight matrix multiplies a matrix with one column per prompt
token, so every weight that is read is used n times.  Right: the weight
matrix multiplies a single column, so every weight that is read is used
once.  Run it to refresh ../assets/llm-phases.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, MUTED,
                    ORANGE, brace, cells, save, text)

W, H = 1120, 290
CW = 22            # one matrix cell, in px
ROWS = 5           # rows drawn for every matrix
PANEL_W = 500


def matrix(x, y, cols, fill, stroke):
    out = []
    for r in range(ROWS):
        shapes, _ = cells(x, y + r * CW, [""] * cols, CW, CW, fill, stroke)
        out += shapes
    return out, x + cols * CW


def panel(x, title, ncols, token_label, captions):
    out = [text(x + PANEL_W / 2, 26, title, 17, INK, "bold")]
    y = 56
    mh = ROWS * CW
    total = 5 * CW + 34 + ncols * CW + 34 + ncols * CW
    sx = x + (PANEL_W - total) / 2
    shapes, e = matrix(sx, y, 5, FILL_BLUE, BLUE)
    out += shapes
    out.append(text(sx + 5 * CW / 2, y + mh + 30, "权重矩阵", 15, MUTED))
    out.append(text(e + 17, y + mh / 2 + 7, "×", 20, INK))
    shapes, e2 = matrix(e + 34, y, ncols, FILL_ORANGE, ORANGE)
    out += shapes
    out += brace(e + 34, e2, y + mh + 4, ORANGE, token_label, 14)
    out.append(text(e2 + 17, y + mh / 2 + 7, "=", 20, INK))
    shapes, e3 = matrix(e2 + 34, y, ncols, FILL_GREEN, GREEN)
    out += shapes
    out.append(text((e2 + 34 + e3) / 2, y + mh + 30, "结果", 15, MUTED))
    for i, c in enumerate(captions):
        out.append(text(x + PANEL_W / 2, 236 + i * 26, c, 15, INK if i == 0 else MUTED))
    return out


def build():
    out = panel(20, "prefill：提示的全部 token 一次通过网络", 6, "提示的 n 个 token",
                ["每个权重读出 1 次，用 n 次：矩阵乘矩阵",
                 "算术强度高，速率由算力决定"])
    out += panel(600, "decode：每次生成 1 个 token", 1, "1 个 token",
                 ["每个权重读出 1 次，用 1 次：矩阵乘向量",
                  "算术强度低，速率由带宽决定"])
    return out


if __name__ == "__main__":
    save("llm-phases", W, H, build())
