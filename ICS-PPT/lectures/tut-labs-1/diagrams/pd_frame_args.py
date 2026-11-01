#!/usr/bin/env python3
"""The five arguments of matvec_q4_0 drawn onto the product y = W x.

W is rows x cols of Q4_0 weights, stored row after row from w; x is cols
floats above it, as wide as a row; y is rows floats beside it, as tall as
the matrix. Row r and y[r] are the pair the function computes: y[r] is the
dot product of row r with x. The pointers w, x, y and the start of row r
are arrows into the picture.
Run it to refresh ../assets/pd-frame-args.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, GREEN, INK, LINE,
                    MUTED, WHITE, arrow, brace, line, mono, rect, save, text,
                    vbrace)

W, H = 1120, 315
MX, MY, MW, MH = 300, 96, 460, 170     # the matrix
XY, XH = 40, 26                        # the x strip above it
YX, YW = 856, 26                       # the y strip beside it
RY, RH = 166, 14                       # row r


def build():
    out = []
    # x: cols floats, as wide as a row
    out.append(rect(MX, XY, MW, XH, FILL_GREY, MUTED, rx=2, width=1.4))
    for i in range(6):
        out.append(line(MX + (i + 1) * 12, XY, MX + (i + 1) * 12, XY + XH, MUTED, 0.8))
    out.append(text(MX + 100, XY + 18, "…", 16, MUTED))
    out.append(text(MX + MW / 2 + 40, XY + 18, "x：cols 个 float", 14, INK, "bold"))
    out.append(arrow(MX - 70, XY + XH / 2, MX - 4, XY + XH / 2, MUTED))
    out.append(mono(MX - 76, XY + XH / 2 + 5, "x", 15, INK, "bold", anchor="end"))
    # W: rows x cols, stored row after row
    out.append(rect(MX, MY, MW, MH, FILL_BLUE, BLUE, rx=2, width=1.4))
    for i in range(1, 5):
        out.append(line(MX, MY + i * RH, MX + MW, MY + i * RH, BLUE, 0.6))
    out.append(text(MX + MW / 2, MY + MH - 48, "…", 16, BLUE))
    out.append(rect(MX, RY, MW, RH, BLUE, BLUE, rx=0, width=1))
    out.append(text(MX + MW / 2, RY + 11, "第 r 行：cols 个权重，cols / 32 个块", 11.5, WHITE, "bold"))
    out.append(text(MX + MW / 2, MY + MH - 18, "W：rows 行，每行 cols / 32 · 18 字节", 14, INK, "bold"))
    out += brace(MX, MX + MW, MY + MH + 6, BLUE, "cols 列（32 的倍数）", 13)
    out += vbrace(MX + MW + 6, MY, MY + MH, BLUE, "rows 行", 13)
    # the pointers w and w + r * row_bytes
    out.append(arrow(MX - 70, MY + 2, MX - 4, MY + 2, BLUE))
    out.append(mono(MX - 76, MY + 7, "w", 15, INK, "bold", anchor="end"))
    out.append(arrow(MX - 150, RY + RH / 2, MX - 4, RY + RH / 2, BLUE))
    out.append(mono(MX - 156, RY + RH / 2 - 4, "w + r * (cols/32*18)", 13, INK, "bold",
                    anchor="end"))
    out.append(text(MX - 156, RY + RH / 2 + 14, "第 r 行的第一个字节", 11.5, MUTED, anchor="end"))
    # y: rows floats, as tall as W
    out.append(rect(YX, MY, YW, MH, FILL_GREEN, GREEN, rx=2, width=1.4))
    for i in range(1, 5):
        out.append(line(YX, MY + i * RH, YX + YW, MY + i * RH, GREEN, 0.6))
    out.append(rect(YX, RY, YW, RH, GREEN, GREEN, rx=0, width=1))
    out.append(arrow(YX + YW / 2, MY - 30, YX + YW / 2, MY - 4, GREEN))
    out.append(mono(YX + YW / 2, MY - 36, "y", 15, INK, "bold", anchor="middle"))
    out.append(text(YX + YW + 10, MY + MH - 4, "y：rows 个 float", 14, INK, "bold", anchor="start"))
    out.append(mono(YX + YW + 10, RY + 11, "y[r]", 13, GREEN, "bold"))
    # the rule
    out.append(arrow(MX + MW + 6, RY + RH / 2, YX - 4, RY + RH / 2, GREEN, 1.8))
    out.append(text(YX + YW + 10, RY + 40, "y[r] = Σ W[r][c] · x[c]", 15, INK, "bold", anchor="start"))
    out.append(text(YX + YW + 10, RY + 58, "c = 0..cols−1，以 float 计算", 12, MUTED, anchor="start"))
    out.append(text(YX + YW + 10, RY + 74, "相加的次序不限", 12, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("pd-frame-args", W, H, build(), left=-35)
