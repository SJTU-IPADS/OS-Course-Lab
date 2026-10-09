#!/usr/bin/env python3
"""A nop sled in front of the attack code.

The row is the input, low addresses on the left: 256 bytes of nop, the
attack code, and the guessed address written over the return address. The
stack is at a different place in each run, so the guessed address lands at a
different offset each time. Wherever it lands inside the sled, the CPU
executes nop after nop and reaches the attack code.

Drawn to scale at 3 px per byte. Colours as in exploit.svg: what the input
wrote is red.
Run it to refresh ../assets/nop-sled.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREY, FILL_RED, FONT, INK, LINE, MONO, MUTED, RED, arrow,
                    brace, label_line, mono, path, rect, save, text)

W, H = 1120, 206
X0, B = 92, 3                       # left edge, pixels per byte
Y, CH = 82, 48
SLED, CODE, ADDR = 256, 48, 8       # bytes


def bx(off):
    return X0 + off * B


def build():
    out = []
    end = SLED + CODE + ADDR
    # the sled, with a few of its one-byte instructions drawn at the left end
    out.append(rect(bx(0), Y, SLED * B, CH, FILL_GREY, RED, rx=2, width=1.6))
    for k in range(1, 5):
        out.append(f'<line x1="{bx(0) + 44 * k:.1f}" y1="{Y:.1f}" x2="{bx(0) + 44 * k:.1f}" '
                   f'y2="{Y + CH:.1f}" stroke="{RED}" stroke-width="1"/>')
    for k in range(4):
        out.append(mono(bx(0) + 44 * k + 22, Y + 30, "nop", 15, INK, anchor="middle"))
    out.append(mono(bx(0) + 44 * 4 + 22, Y + 30, "…", 15, INK, anchor="middle"))
    out += label_line(bx(124), Y + 30, [("nop sled", MONO, INK, "bold"),
                                        ("：256 条 ", FONT, INK, "bold"),
                                        ("nop", MONO, INK, "bold")], 15)
    out.append(rect(bx(SLED), Y, CODE * B, CH, FILL_RED, RED, rx=2, width=1.6))
    out.append(text(bx(SLED) + CODE * B / 2, Y + 30, "攻击代码", 15, RED, "bold"))
    out.append(rect(bx(SLED + CODE), Y, ADDR * B, CH, FILL_RED, RED, rx=2, width=1.6))
    out.append(text(bx(end) + 10, Y + 36, "猜测的地址", 15, RED, "bold", anchor="start"))
    # the guessed address lands somewhere in the sled, a different place each run
    ax = bx(SLED + CODE) + ADDR * B / 2
    for k, off in enumerate((60, 138, 214)):
        x = bx(off)
        out.append(path(f"M {ax:.1f} {Y - 3} C {ax:.1f} {Y - 58 + 10 * k} "
                        f"{x:.1f} {Y - 58 + 10 * k} {x:.1f} {Y - 4}", RED, 1.8,
                        dash="6 4"))
    out.append(text(bx(138), Y - 62, "栈的位置每次运行都不同：猜测的地址每次落在不同的位置", 15,
                    RED, "bold"))
    # execution runs through the sled into the code
    y = Y + CH + 18
    out += brace(bx(0) + 2, bx(SLED) - 2, Y + CH + 4, MUTED, "", 14, depth=6)
    out.append(arrow(bx(60), y + 12, bx(SLED) + 40, y + 12, INK, 2.2))
    out.append(text(bx(60), y + 38, "落在这 256 字节中的任何一处，都逐条执行 nop，到达攻击代码",
                    15, INK, "bold", anchor="start"))
    out.append(text(bx(0) - 8, Y + CH / 2 + 5, "低地址", 13, MUTED, anchor="end"))
    out.append(text(bx(end) + 10, Y + 8, "高地址", 13, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("nop-sled", W, H, build(), left=19)    # centres the ink on the canvas
