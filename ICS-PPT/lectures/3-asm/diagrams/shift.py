#!/usr/bin/env python3
"""sarl $2 and shrl $2 on the same 32-bit pattern 0xfffffff0.

The source row is in the middle and carries both readings of the pattern:
-16 as a signed number (orange, the reading sarl uses) and 4294967280 as an
unsigned one (green, the reading shrl uses). Every bit moves two places right;
what enters on the left is copies of bit 31 for sarl, zeros for shrl, and the
two low bits fall off. Bits 25..6 are 1 in all three rows and are folded into
one ellipsis column so the cells can be large.
Run it to refresh ../assets/shift.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, LINE, MUTED,
                    ORANGE, WHITE, arrow, brace, mono, rect, save, text)

W, H = 1120, 440
X0, CW, CH, EW = 236, 48, 54, 56        # cells: left edge, width, height; ellipsis
RIGHT = 896                             # left edge of the value column
BITS = list(range(31, 25, -1)) + [None] + list(range(5, -1, -1))
SRC = 0xFFFFFFF0
SAR = (SRC >> 2) | 0xC0000000
SHR = SRC >> 2


def x_of(k):
    """Left edge of column k; column 6 is the ellipsis."""
    return X0 + k * CW if k <= 6 else X0 + 6 * CW + EW + (k - 7) * CW


def cx(k):
    return x_of(k) + (EW if k == 6 else CW) / 2


def row(y, value, fills):
    out = []
    for k, b in enumerate(BITS):
        if b is None:
            out.append(text(cx(k), y + CH / 2 + 8, "…", 24, MUTED))
            continue
        out.append(rect(x_of(k), y, CW, CH, fills.get(b, WHITE), LINE, rx=3, width=1.4))
        out.append(text(cx(k), y + CH / 2 + 8, str(value >> b & 1), 22, INK,
                        font="monospace"))
    return out


def label(y, name, sub, is_code):
    font = "monospace" if is_code else None
    kw = {"font": font} if font else {}
    return [text(20, y + 24, name, 22, INK, "bold", anchor="start", **kw),
            text(20, y + 48, sub, 17, MUTED, anchor="start")]


def build():
    ys, ym, yh = 40, 196, 352                       # sarl, source, shrl rows
    out = [text(cx(k), 26, str(b), 15, MUTED, font="monospace")
           for k, b in enumerate(BITS) if b is not None]
    out.append(text(cx(6), 26, "25..6", 13, MUTED, font="monospace"))

    out += label(ys, "sarl $2", "算术右移", True)
    out += label(ym, "源操作数", "0xFFFFFFF0", False)
    out += label(yh, "shrl $2", "逻辑右移", True)
    out += row(ys, SAR, {31: FILL_ORANGE, 30: FILL_ORANGE})
    out += row(ym, SRC, {b: FILL_BLUE for b in BITS if b is not None and b > 1})
    out += row(yh, SHR, {31: FILL_GREEN, 30: FILL_GREEN})

    # the low two bits of the source fall off
    out.append(rect(x_of(11) + 3, ym + 3, 2 * CW - 6, CH - 6, "none", MUTED, rx=3,
                    width=1.6, dash="5 4"))
    out += brace(x_of(11), x_of(13), ym + CH + 4, MUTED, "低 2 位移出", 16)

    # every bit moves two columns right
    for k in (3, 7, 9):
        out.append(arrow(cx(k), ym - 4, cx(k + 2), ys + CH + 6, LINE, 1.6, "5 4"))
        out.append(arrow(cx(k), ym + CH + 4, cx(k + 2), yh - 6, LINE, 1.6, "5 4"))
    out.append(text(cx(6), 150, "每一位右移 2 位", 17, MUTED))

    # sarl copies bit 31 into the two new bits; shrl lets zeros in
    for k in (0, 1):
        out.append(arrow(cx(0), ym - 4, cx(k), ys + CH + 6, ORANGE, 2.2))
    out.append(text(cx(1) + 22, 136, "复制符号位", 18, ORANGE, "bold", anchor="start"))
    out.append(arrow(X0 - 44, yh + CH / 2, X0 - 6, yh + CH / 2, GREEN, 2.4))
    out.append(text(X0 - 25, yh - 10, "补 0", 18, GREEN, "bold"))

    # the reading of each row
    out.append(text(RIGHT, ys + 24, "= -4", 22, ORANGE, "bold", anchor="start"))
    out.append(text(RIGHT, ys + 48, "有符号：-16 ÷ 4", 17, MUTED, anchor="start"))
    out.append(text(RIGHT, ym + 22, "有符号：-16", 19, ORANGE, "bold", anchor="start"))
    out.append(text(RIGHT, ym + 48, "无符号：4294967280", 19, GREEN, "bold",
                    anchor="start"))
    out.append(text(RIGHT, yh + 24, "= 1073741820", 22, GREEN, "bold", anchor="start"))
    out.append(text(RIGHT, yh + 48, "无符号：4294967280 ÷ 4", 17, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    assert SAR == 0xFFFFFFFC and SHR == 1073741820
    save("shift", W, H, build())
