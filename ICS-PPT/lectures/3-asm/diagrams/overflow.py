#!/usr/bin/env python3
"""Where the writes land when x overflows, in main's frame of overflow.c.

Offsets are overflow.s's (gcc -Og -fcf-protection=none -fno-stack-protector):
after `pushq %rbx` and `subq $32, %rsp`, x[0..3] at 0..15, w[0..3] at 16..31,
the saved %rbx at 32..39, the return address at 40..47. fill writes upward
from offset 0, four bytes per value, so the count on the command line decides
how far the writes reach: 4 values stay in x, 8 reach w, 12 reach the return
address.
Run it to refresh ../assets/overflow.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, MONO,
                    MUTED, ORANGE, RED, arrow, mono, rect, save, text)

W, H = 1120, 330
X0, B = 100, 17                     # left edge, pixels per byte
Y, CH = 84, 60                      # frame row: top, height

# the frame, low addresses first: offset, size, name, fill, stroke
PARTS = [(0, 16, "x[0..3]", FILL_ORANGE, ORANGE),
         (16, 16, "w[0..3]", FILL_ORANGE, ORANGE),
         (32, 8, "保存的 %rbx", FILL_GREEN, GREEN),
         (40, 8, "返回地址", FILL_BLUE, BLUE)]

# how far each run writes: count, end offset, result, tone
RUNS = [(4, 16, "结果 70，正确", GREEN),
        (8, 32, "覆盖 w，结果 26", ORANGE),
        (12, 48, "覆盖返回地址，SIGSEGV", RED)]


def bx(off):
    return X0 + off * B


def frame():
    out = []
    for off, n, name, fill, stroke in PARTS:
        w = n * B
        out.append(rect(bx(off), Y, w, CH, fill, stroke, rx=2, width=1.6))
        font = MONO if name.isascii() else None
        if font:
            out.append(mono(bx(off) + w / 2, Y + 37, name, 16, INK, "bold",
                            anchor="middle"))
        else:
            out.append(text(bx(off) + w / 2, Y + 37, name, 15, INK, "bold"))
    for off in (0, 16, 32, 40, 48):
        s = "(%rsp)" if off == 0 else f"{off}(%rsp)"
        out.append(mono(bx(off), Y + CH + 22, s, 13, MUTED, anchor="middle"))
    out.append(text(bx(0), Y - 14, "低地址", 14, MUTED, anchor="start"))
    out.append(text(bx(48), Y - 14, "高地址", 14, MUTED, anchor="end"))
    return out


def runs(y0):
    """One row per run: an arrow from offset 0 to where its writes stop."""
    out = []
    for k, (count, end, note, tone) in enumerate(RUNS):
        y = y0 + k * 44
        out.append(text(bx(0) - 12, y + 5, f"{count} 个实参", 15, tone, "bold",
                        anchor="end"))
        out.append(arrow(bx(0), y, bx(end), y, tone, 2.4))
        out.append(text(bx(end) + 12, y + 5, note, 15, tone, "bold", anchor="start"))
    return out


def build():
    return frame() + runs(Y + CH + 58)


if __name__ == "__main__":
    save("overflow", W, H, build())
