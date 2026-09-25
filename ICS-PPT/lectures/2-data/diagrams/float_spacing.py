#!/usr/bin/env python3
"""Where the representable values sit on the real line.

Drawn for a toy format with a 3-bit fraction, so one binade holds eight
values and the eye can count them. Every crossing of a power of two doubles
the spacing, which is the same statement as "the relative precision is
constant". Run it to refresh ../assets/float-spacing.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import BLUE, INK, LINE, MONO, MUTED, ORANGE, brace, line, svg, text

W, H = 900, 262
AX0, AX1, AXIS_Y = 84, 862, 132
SPAN = 8.0                      # the axis runs 0 .. 8
FRACTION_BITS = 3
BINADES = range(-4, 3)          # 2^-4 .. 2^2, the last one ending at 8


def x_of(v):
    return AX0 + v / SPAN * (AX1 - AX0)


def build():
    out = [text(W / 2, 30, "可表示的值在数轴上的分布（尾数 3 位的简化格式）",
                15.5, INK, "bold"),
           line(AX0 - 18, AXIS_Y, AX1 + 20, AXIS_Y, LINE, 1.6)]

    step_of = {}
    for e in BINADES:
        low, step = 2.0 ** e, 2.0 ** e / (1 << FRACTION_BITS)
        step_of[e] = step
        v = low
        while v < low * 2 - step / 2 and v <= SPAN:
            out.append(line(x_of(v), AXIS_Y - 8, x_of(v), AXIS_Y + 8, BLUE, 1.2))
            v += step
    out.append(line(x_of(SPAN), AXIS_Y - 8, x_of(SPAN), AXIS_Y + 8, BLUE, 1.2))

    # The powers of two are where the spacing changes, so they carry the labels.
    for v in (0, 1, 2, 4, 8):
        out.append(line(x_of(v), AXIS_Y - 15, x_of(v), AXIS_Y + 15, INK, 1.8))
        out.append(text(x_of(v), AXIS_Y + 34, str(v), 13, INK, "bold", font=MONO))

    for low, e in ((1, 0), (2, 1), (4, 2)):
        out += brace(x_of(low) + 3, x_of(low * 2) - 3, AXIS_Y - 26, ORANGE,
                     f"步长 {step_of[e]:g}", 12.5, 7, below=False)

    out.append(text(x_of(0.5), AXIS_Y + 62,
                    "越靠近零，可表示的值越密", 12.5, MUTED))
    out.append(text(x_of(5.0), AXIS_Y + 62,
                    "每跨过一个 2 的幂，步长加倍，相对精度不变", 12.5, MUTED))
    out.append(text(W / 2, H - 18,
                    "阶码选定一个区间，尾数把这个区间等分；区间越大，同样的等分越粗",
                    13.5, INK))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "float-spacing.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
