#!/usr/bin/env python3
"""One multiplication per instruction against eight: the idea of SIMD.

Left, the scalar imull of the lecture's loop: one w, one x, one product in
the 32-bit %ecx. Right, vpmulld: eight w, eight x, eight products in the
256-bit %ymm0, the same drawing eight times over. The rows are those of
lanes_mul_add.py, which continues the picture with the accumulate.
Run it to refresh ../assets/simd-idea.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED,
                    ORANGE, arrow, brace, circle, line, mono, rect, save, text)

W, H = 1120, 262
Y_W, Y_X, Y_MUL, Y_P, BH = 46, 90, 152, 182, 34


def lane(x, cw, w_label, x_label, p_label):
    """One column: a w cell over an x cell, a multiplier, the product cell."""
    bw, cx = cw - 12, x + cw / 2
    return [rect(x + 6, Y_W, bw, BH, FILL_BLUE, BLUE, rx=3, width=1.4),
            mono(cx, Y_W + 22, w_label, 14, INK, anchor="middle"),
            rect(x + 6, Y_X, bw, BH, FILL_GREY, MUTED, rx=3, width=1.4),
            mono(cx, Y_X + 22, x_label, 14, INK, anchor="middle"),
            line(cx - 14, Y_X + BH, cx - 8, Y_MUL - 14, BLUE, 1.4),
            line(cx + 14, Y_X + BH, cx + 8, Y_MUL - 14, MUTED, 1.4),
            circle(cx, Y_MUL, 15, FILL_ORANGE, ORANGE, 1.8),
            text(cx, Y_MUL + 6, "×", 18, ORANGE, "bold"),
            arrow(cx, Y_MUL + 15, cx, Y_P - 4, ORANGE, 1.6),
            rect(x + 6, Y_P, bw, BH, FILL_ORANGE, ORANGE, rx=3, width=1.4),
            mono(cx, Y_P + 22, p_label, 14, INK, anchor="middle")]


def build():
    out = []
    xs, cws = 110, 120
    out.append(text(40, 24, "标量 imull：1 条指令，1 次乘法", 17, INK, "bold", anchor="start"))
    out += lane(xs, cws, "w[i]", "x[i]", "积")
    out += brace(xs + 6, xs + cws - 6, Y_P + BH + 6, ORANGE, "%ecx，32 位")

    out.append(line(340, 12, 340, H - 12, LINE, 1, "4 4"))

    xv, cwv = 380, 90
    out.append(text(xv, 24, "向量 vpmulld：1 条指令，8 次乘法", 17, INK, "bold", anchor="start"))
    for k in range(8):
        out += lane(xv + (7 - k) * cwv, cwv, f"w[i+{k}]", f"x[i+{k}]", f"积{k}")
    out += brace(xv + 6, xv + 8 * cwv - 6, Y_P + BH + 6, ORANGE, "%ymm0，256 位：8 个 32 位的积")
    return out


if __name__ == "__main__":
    save("simd-idea", W, H, build())
