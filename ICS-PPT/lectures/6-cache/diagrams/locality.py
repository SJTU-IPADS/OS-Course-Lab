#!/usr/bin/env python3
"""The two kinds of locality as reference patterns: address against time.

Left, temporal locality: a variable referenced in every iteration is one
address referenced again and again (the accumulator `sum`). Right, spatial
locality: `w[i]` is referenced for i = 0, 1, ..., 7, so consecutive references
go to neighbouring addresses. Each reference is an orange dot (the data being
requested).
Run it to refresh ../assets/locality.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE, arrow, circle, line, mono,
                    rect, save, text)

W, H = 960, 300
PW, PH = 380, 190
TOPS = 52


def axes(x0, title, caption):
    out = []
    out.append(text(x0 + PW / 2, 22, title, 18, INK, "bold"))
    out.append(arrow(x0, TOPS + PH, x0 + PW, TOPS + PH, MUTED, 1.8))
    out.append(arrow(x0, TOPS + PH, x0, TOPS - 6, MUTED, 1.8))
    out.append(text(x0 + PW + 4, TOPS + PH + 22, "时间", 14, MUTED, anchor="end"))
    out.append(text(x0 + 8, TOPS - 8, "地址", 14, MUTED, anchor="start"))
    out.append(text(x0 + PW / 2, TOPS + PH + 46, caption, 16, INK))
    return out


def build():
    out = []
    # temporal: eight references to the same address
    x0 = 90
    out += axes(x0, "时间局部性", "同一个地址被反复引用")
    ys = TOPS + PH - 100
    out.append(line(x0, ys, x0 + PW - 20, ys, LINE, 1.2, dash="4 4"))
    out.append(mono(x0 - 10, ys + 5, "sum", 15, INK, "bold", "end"))
    for t in range(8):
        out.append(circle(x0 + 34 + t * 41, ys, 8, FILL_ORANGE, ORANGE, 2))
    # spatial: eight references to neighbouring addresses
    x0 = 90 + PW + 80
    out += axes(x0, "空间局部性", "相邻的地址在相近的时间被引用")
    for t in range(8):
        y = TOPS + PH - 20 - t * 20
        out.append(mono(x0 - 10, y + 5, f"w[{t}]", 14, INK, "bold", "end"))
        out.append(circle(x0 + 34 + t * 41, y, 8, FILL_ORANGE, ORANGE, 2))
    return out


if __name__ == "__main__":
    save("locality", W, H, build(), left=13)
