#!/usr/bin/env python3
"""The figure part-5.md asks for on its page 6: int, float, double and the casts.

Three types, six conversions, coloured by what they can lose: green keeps the
value exactly, gold may round, red (floating point to int) truncates toward
zero and is undefined out of range; the box in the middle is what x86-64's
cvttsd2si returns then.  Run it to refresh ../assets/cast-paths.svg.
"""

import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import FILL_GREY, INK, MONO, MUTED, rect, svg, text

W, H = 900, 262
GREEN, GOLD, RED = "#196b24", "#b8860b", "#c0392b"
FILL_RED = "#fbe3e0"

BW, BH = 150, 56
INT = (40, 103)
FLOAT = (560, 16)
DOUBLE = (560, 190)


def node(pos, name, sub):
    x, y = pos
    return [rect(x, y, BW, BH, FILL_GREY, INK, rx=8, width=1.6),
            text(x + BW / 2, y + 24, name, 17, INK, "bold", font=MONO),
            text(x + BW / 2, y + 44, sub, 12.5, MUTED)]


def arrow(x1, y1, x2, y2, color, off=0.0):
    """A straight arrow, shifted `off` px to its left, ending in a head."""
    dx, dy = x2 - x1, y2 - y1
    n = math.hypot(dx, dy)
    ux, uy = dx / n, dy / n
    nx, ny = -uy * off, ux * off
    a = (x1 + nx, y1 + ny)
    b = (x2 + nx, y2 + ny)
    s = 9.0
    shaft_end = (b[0] - ux * s, b[1] - uy * s)
    head = [b, (b[0] - ux * s - uy * s * 0.55, b[1] - uy * s + ux * s * 0.55),
            (b[0] - ux * s + uy * s * 0.55, b[1] - uy * s - ux * s * 0.55)]
    pts = " ".join(f"{p:.1f},{q:.1f}" for p, q in head)
    return [f'<line x1="{a[0]:.1f}" y1="{a[1]:.1f}" x2="{shaft_end[0]:.1f}" '
            f'y2="{shaft_end[1]:.1f}" stroke="{color}" stroke-width="2.4"/>',
            f'<polygon points="{pts}" fill="{color}"/>']


def build():
    out = []
    out += node(INT, "int", "32 位，含 31 位数值")
    out += node(FLOAT, "float", "24 位有效精度")
    out += node(DOUBLE, "double", "53 位有效精度")

    ix, iy = INT[0] + BW, INT[1] + BH / 2
    fx, fy = FLOAT[0], FLOAT[1] + BH / 2
    dx, dy = DOUBLE[0], DOUBLE[1] + BH / 2
    # int <-> float: gold outward (above), red inward
    out += arrow(ix, iy - 10, fx, fy + 4, GOLD, off=-7)
    out += arrow(fx, fy + 4, ix, iy - 10, RED, off=-7)
    # int <-> double: green outward (below), red inward
    out += arrow(ix, iy + 10, dx, dy - 4, GREEN, off=7)
    out += arrow(dx, dy - 4, ix, iy + 10, RED, off=7)
    # float <-> double
    cx = FLOAT[0] + BW / 2
    out += arrow(cx - 12, FLOAT[1] + BH, cx - 12, DOUBLE[1], GREEN)
    out += arrow(cx + 12, DOUBLE[1], cx + 12, FLOAT[1] + BH, GOLD)

    out.append(text(360, 40, "int → float：超过 2²⁴ 时舍入", 13, GOLD, "bold"))
    out.append(text(360, 236, "int → double：精确", 13, GREEN, "bold"))
    out.append(text(cx + 26, 108, "float → double：精确", 13, GREEN, "bold", "start"))
    out.append(text(cx + 26, 136, "double → float：", 13, GOLD, "bold", "start"))
    out.append(text(cx + 26, 154, "舍入或溢出", 13, GOLD, "bold", "start"))

    # the red pair converges on int; the hardware note sits between them
    bx, by, bw, bh = 356, 104, 214, 60
    out.append(rect(bx, by, bw, bh, FILL_RED, RED, rx=6, width=1.6))
    out.append(text(bx + bw / 2, by + 20, "浮点 → int：向零截断", 13, RED, "bold"))
    out.append(text(bx + bw / 2, by + 38, "越界：x86-64 的 cvttsd2si", 12, RED))
    out.append(text(bx + bw / 2, by + 53, "输出 0x80000000", 12, RED, font=MONO))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "cast-paths.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
