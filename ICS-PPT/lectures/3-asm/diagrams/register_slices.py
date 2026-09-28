#!/usr/bin/env python3
"""%rax and its low slices %eax, %ax, %al, drawn to scale on one bit axis.

Bit 63 is on the left and bit 0 on the right, so every slice is flush right.
Run it to refresh ../assets/register-slices.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, FILL_GREY, GREEN,
                    INK, MUTED, ORANGE, line, mono, rect, save, text)

W, H = 1000, 200
X0, X1 = 120, 980
PX = (X1 - X0) / 64


def xbit(b):
    """Left edge of bit b (bit 63 leftmost)."""
    return X0 + (63 - b) * PX


ROWS = [("%rax", 64, "64 位", FILL_GREY, INK),
        ("%eax", 32, "低 32 位", FILL_BLUE, BLUE),
        ("%ax", 16, "低 16 位", FILL_ORANGE, ORANGE),
        ("%al", 8, "低 8 位", FILL_GREEN, GREEN)]


def build():
    out = []
    for b in (63, 31, 15, 7):
        out.append(mono(xbit(b) + 2, 22, str(b), 15, MUTED))
        out.append(line(xbit(b), 28, xbit(b), 190, "#c9d4db", 1, "3 4"))
    out.append(mono(X1 - 2, 22, "0", 15, MUTED, anchor="end"))
    for k, (name, bits, what, fill, color) in enumerate(ROWS):
        y = 32 + k * 40
        x = xbit(bits - 1)
        out.append(rect(x, y, X1 - x, 32, fill, color, rx=3, width=1.6))
        out.append(mono(X0 - 16, y + 22, name, 18, color, "bold", anchor="end"))
        out.append(text(x + (X1 - x) / 2, y + 22, what, 15, INK))
    return out


if __name__ == "__main__":
    save("register-slices", W, H, build())
