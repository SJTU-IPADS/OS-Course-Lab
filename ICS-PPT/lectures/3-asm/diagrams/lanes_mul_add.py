#!/usr/bin/env python3
"""Eight lanes, eight multipliers, eight adders, all driven by one instruction.

Top rows are the operands of vpmulld: w, loaded into %ymm2 by vmovdqu, and x
straight from memory. The products land in %ymm0 and meet the running sums
in %ymm1 at vpaddd. Registers are those of the listing on page 64.
Run it to refresh ../assets/lanes-mul-add.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, arrow, circle, line, mono,
                    rect, save, text)

W, H = 1120, 470
X0, CW = 190, 112


def row(y, fmt, fill, stroke, size=14):
    out = []
    for k in range(8):
        x = X0 + (7 - k) * CW
        out.append(rect(x + 6, y, CW - 12, 36, fill, stroke, rx=3, width=1.4))
        out.append(mono(x + CW / 2, y + 23, fmt(k), size, INK, anchor="middle"))
    return out


def build():
    out = []
    labels = [(28, "%ymm2", "w[i..i+7]"), (84, "内存", "x[i..i+7]")]
    for y, a, b in labels:
        out.append(mono(20, y + 16, a, 17, INK, "bold"))
        out.append(mono(20, y + 34, b, 13, MUTED))
    out += row(28, lambda k: f"w[i+{k}]", FILL_BLUE, BLUE)
    out += row(84, lambda k: f"x[i+{k}]", FILL_GREY, MUTED)
    for k in range(8):
        cx = X0 + (7 - k) * CW + CW / 2
        out.append(line(cx - 14, 120, cx - 8, 160, BLUE, 1.4))
        out.append(line(cx + 14, 120, cx + 8, 160, MUTED, 1.4))
        out.append(circle(cx, 176, 16, FILL_ORANGE, ORANGE, 1.8))
        out.append(text(cx, 182, "×", 18, ORANGE, "bold"))
        out.append(arrow(cx, 192, cx, 222, ORANGE, 1.6))
        out.append(mono(cx + 38, 292, f"s{k}", 13, GREEN, anchor="middle"))
        out.append(line(cx + 30, 298, cx + 14, 318, GREEN, 1.4))
        out.append(line(cx, 262, cx - 6, 314, ORANGE, 1.4))
        out.append(circle(cx, 330, 16, FILL_GREEN, GREEN, 1.8))
        out.append(text(cx, 336, "+", 18, GREEN, "bold"))
        out.append(arrow(cx, 346, cx, 386, GREEN, 1.6))
    out += row(226, lambda k: f"积{k}", FILL_ORANGE, ORANGE)
    out.append(text(20, 190, "vpmulld", 17, ORANGE, "bold", anchor="start"))
    out.append(text(20, 212, "8 个乘法器", 14, MUTED, anchor="start"))
    out.append(text(20, 344, "vpaddd", 17, GREEN, "bold", anchor="start"))
    out.append(text(20, 366, "8 个加法器", 14, MUTED, anchor="start"))
    out.append(mono(20, 252, "%ymm0", 17, INK, "bold"))
    out.append(text(20, 296, "%ymm1 原累加和", 14, GREEN, anchor="start"))
    out += row(390, lambda k: f"s{k}", FILL_GREEN, GREEN, 15)
    out.append(mono(20, 414, "%ymm1", 17, GREEN, "bold"))
    out.append(text(20, 434, "新累加和", 14, MUTED, anchor="start"))
    out.append(text(560, 456, "一条指令驱动 8 个通道同时运算", 16, INK, "bold"))
    return out


if __name__ == "__main__":
    save("lanes-mul-add", W, H, build(), left=-10)    # centres the ink on the canvas
