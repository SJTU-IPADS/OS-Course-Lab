#!/usr/bin/env python3
"""One group of 8 codes meets 8 floats of x: 8 multiplications and 8
additions driven by two instructions, the figure of lecture 3's vector loop
with the operands of matvec_q4_0.

Top rows: the 8 floats (q - 8) in a %ymm register and the 8 floats of x
loaded from memory. The products land in a register and meet the block's
running sums bsum. Lane 0 is at the right, as in lecture 3.
Run it to refresh ../assets/pd-class-mac.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, MUTED, ORANGE, arrow, circle, line, mono, rect, save,
                    text)

W, H = 1120, 410
X0, CW = 190, 112
RH = 34                                # the height of a row of cells


def row(y, fmt, fill, stroke, size=14):
    out = []
    for k in range(8):
        x = X0 + (7 - k) * CW
        out.append(rect(x + 6, y, CW - 12, RH, fill, stroke, rx=3, width=1.4))
        out.append(mono(x + CW / 2, y + 22, fmt(k), size, INK, anchor="middle"))
    return out


def build():
    out = []
    out.append(mono(20, 36, "%ymm", 17, INK, "bold"))
    out.append(text(20, 54, "8 个 (q − 8)，float", 13, MUTED, anchor="start"))
    out.append(text(20, 84, "内存", 17, INK, "bold", anchor="start"))
    out.append(text(20, 102, "x[c+8g .. c+8g+7]", 13, MUTED, anchor="start"))
    out += row(20, lambda k: f"q{k}−8", FILL_BLUE, BLUE)
    out += row(66, lambda k: f"x[c+{k}]", FILL_GREY, MUTED)
    for k in range(8):
        cx = X0 + (7 - k) * CW + CW / 2
        out.append(line(cx - 14, 100, cx - 8, 134, BLUE, 1.4))
        out.append(line(cx + 14, 100, cx + 8, 134, MUTED, 1.4))
        out.append(circle(cx, 148, 16, FILL_ORANGE, ORANGE, 1.8))
        out.append(text(cx, 154, "×", 18, ORANGE, "bold"))
        out.append(arrow(cx, 164, cx, 190, ORANGE, 1.6))
        out.append(mono(cx + 38, 254, f"b{k}", 13, GREEN, anchor="middle"))
        out.append(line(cx + 30, 258, cx + 14, 276, GREEN, 1.4))
        out.append(line(cx, 228, cx - 6, 272, ORANGE, 1.4))
        out.append(circle(cx, 288, 16, FILL_GREEN, GREEN, 1.8))
        out.append(text(cx, 294, "+", 18, GREEN, "bold"))
        out.append(arrow(cx, 304, cx, 330, GREEN, 1.6))
    out += row(194, lambda k: f"积{k}", FILL_ORANGE, ORANGE)
    out.append(text(20, 152, "_mm256_mul_ps", 15, ORANGE, "bold", anchor="start"))
    out.append(text(20, 172, "8 个乘法器", 14, MUTED, anchor="start"))
    out.append(text(20, 292, "_mm256_add_ps", 15, GREEN, "bold", anchor="start"))
    out.append(text(20, 312, "8 个加法器", 14, MUTED, anchor="start"))
    out.append(mono(20, 218, "%ymm", 17, INK, "bold"))
    out.append(text(20, 258, "bsum 原部分和", 14, GREEN, anchor="start"))
    out += row(334, lambda k: f"b{k}", FILL_GREEN, GREEN, 15)
    out.append(mono(20, 356, "bsum", 17, GREEN, "bold"))
    out.append(text(20, 376, "新部分和", 14, MUTED, anchor="start"))
    out.append(text(560, 398, "块的 4 组各做一次（g = 0..3）：bsum 的每一路累加 4 个积，32 个权重全部计入", 15,
                    INK, "bold"))
    return out


if __name__ == "__main__":
    save("pd-class-mac", W, H, build(), left=-10)
