#!/usr/bin/env python3
"""%ymm0 as eight 32-bit lanes, and what vzeroupper does to it.

Lane 0 is at the low end (right). The low 128 bits are %xmm0. Below, the same
256 bits split as float32, int64/double and int8. On the right, vzeroupper
clears the high 128 bits of every %ymm register and keeps the low half.
Run it to refresh ../assets/ymm-lanes.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, GREEN, INK, LINE, MONO,
                    MUTED, WHITE, arrow, mono, rect, save, text)

W, H = 1120, 330
X0, CW = 170, 70                    # lanes span X0 .. X0 + 8 * CW


def lanes():
    right = X0 + 8 * CW
    out = [mono(20, 104, "%ymm0", 20, INK, "bold"),
           text(X0 + 4 * CW, 26, "256 ÷ 32 = 8 个 int32 通道", 17, INK, "bold")]
    for k in range(8):
        x = X0 + (7 - k) * CW
        low = k < 4
        out.append(rect(x, 70, CW, 56, FILL_GREEN if low else FILL_BLUE,
                        GREEN if low else BLUE, rx=2, width=1.6))
        out.append(text(x + CW / 2, 96, f"Lane {k}", 15, INK, "bold"))
        out.append(mono(x + CW / 2, 116, "int32", 13, MUTED, anchor="middle"))
    for b, x, a in ((255, X0, "start"), (128, X0 + 4 * CW - 4, "end"),
                    (127, X0 + 4 * CW + 4, "start"), (0, right, "end")):
        out.append(mono(x, 58, str(b), 13, MUTED, anchor=a))
    out.append(rect(X0 + 4 * CW - 4, 64, 4 * CW + 8, 68, "none", GREEN, rx=4, width=2,
                    dash="6 4"))
    out.append(text(X0 + 6 * CW, 154, "低 128 位 = %xmm0", 15, GREEN, "bold"))
    # other ways to split the same 256 bits
    out.append(text(20, 196, "其他划分", 15, MUTED, anchor="start"))
    rows = [("8 × float32", 8), ("4 × int64/double", 4), ("32 × int8", 32)]
    for j, (name, n) in enumerate(rows):
        y = 210 + j * 36
        w = 8 * CW / n
        for i in range(n):
            out.append(rect(X0 + i * w, y, w, 26, WHITE, LINE, rx=1, width=1.2))
        out.append(mono(X0 - 10, y + 18, name, 13, INK, anchor="end"))
    return out


def half(y, high, fill):
    x, w = 800, 150
    return [rect(x, y, w, 48, fill, BLUE if fill != FILL_GREY else MUTED, rx=2, width=1.6),
            text(x + w / 2, y + 30, high, 15, INK, "bold"),
            rect(x + w, y, w, 48, FILL_GREEN, GREEN, rx=2, width=1.6),
            mono(x + 1.5 * w, y + 30, "%xmm0", 15, INK, "bold", anchor="middle")]


def vzeroupper():
    out = [mono(950, 26, "vzeroupper", 19, INK, "bold", anchor="middle"),
           text(790, 80, "之前", 15, MUTED, anchor="end")]
    out += half(50, "高 128 位", FILL_BLUE)
    out.append(arrow(875, 104, 875, 154, INK, 2))
    out.append(text(886, 134, "高半清零，低半保留", 14, INK, anchor="start"))
    out.append(text(790, 190, "之后", 15, MUTED, anchor="end"))
    out += half(160, "0", FILL_GREY)
    out.append(text(950, 244, "作用于 %ymm0 ~ %ymm15", 15, INK))
    out.append(text(950, 270, "编译器在返回调用者前插入", 14, MUTED))
    return out


def build():
    return lanes() + vzeroupper()


if __name__ == "__main__":
    save("ymm-lanes", W, H, build())
