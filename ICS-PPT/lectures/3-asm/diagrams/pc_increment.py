#!/usr/bin/env python3
"""The loop body laid out by address, each box as wide as its encoding.

Offsets and lengths come from `objdump -d dot.o` (-Og build). The hops above
the boxes are the sequential %rip updates, each one the length of the
instruction it leaves; all of them go to higher addresses.
Run it to refresh ../assets/pc-increment.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, INK, LINE, MUTED, ORANGE, WHITE,
                    arrow, mono, path, rect, save, text)

W, H = 1120, 230
X0, PX = 110, 40                              # left edge, pixels per byte

INSNS = [(0x20, 3, "movslq"), (0x23, 4, "movl"), (0x27, 5, "imull"),
         (0x2c, 3, "addl"), (0x2f, 3, "addl"), (0x32, 2, "cmpl"), (0x34, 2, "jl")]


def xof(off):
    return X0 + (off - 0x20) * PX


def build():
    out = []
    for k, (off, n, name) in enumerate(INSNS):
        x = xof(off)
        out.append(rect(x, 100, n * PX, 54, FILL_BLUE if k % 2 == 0 else WHITE, BLUE,
                        rx=3, width=1.6))
        out.append(mono(x + n * PX / 2, 125, name, 16, INK, "bold", anchor="middle"))
        out.append(text(x + n * PX / 2, 146, f"{n} 字节", 13, MUTED))
        out.append(mono(x, 178, f"0x{off:x}", 14, MUTED, anchor="middle"))
        if k < len(INSNS) - 1:
            nx = xof(INSNS[k + 1][0])
            out.append(path(f"M {x + 6} 96 C {x + 10} 50, {nx - 10} 50, {nx - 4} 94",
                            ORANGE, 2))
            out.append(text((x + nx) / 2, 52, f"+{n}", 15, ORANGE, "bold"))
    end = xof(0x36)
    out.append(mono(end, 178, "0x36", 14, MUTED, anchor="middle"))
    out.append(arrow(X0, 200, end + 40, 200, LINE, 1.6))
    out.append(text(X0, 220, "低地址", 14, MUTED, anchor="start"))
    out.append(text(end + 40, 220, "高地址", 14, MUTED, anchor="end"))
    out.append(text(20, 28, "%rip ← %rip + 指令长度：", 17, INK, "bold", anchor="start"))
    out.append(text(250, 28, "每次都前进到更高的地址", 17, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("pc-increment", W, H, build())
