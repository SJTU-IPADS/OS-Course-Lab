#!/usr/bin/env python3
"""The figure part-5.md asks for on its page 1: ticks that thin out.

Left, a toy format with a 3-bit fraction over [0.5, 4): each binade holds
eight values, so the eye can count them and see the spacing double at every
power of two.  Right, FP32 just above 2^24, where the spacing has reached 2
and the odd integers fall between two ticks.  Run it to refresh
../assets/ulp-spacing.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import BLUE, INK, LINE, MUTED, brace, line, svg, text

W, H = 1000, 150
RED = "#c0392b"
AXIS_Y = 74

# left panel: linear axis over [0.5, 4]
L0, L1 = 40, 560
LO, HI = 0.5, 4.0
FRACTION_BITS = 3

# right panel: FP32 integers 2^24 .. 2^24 + 6
R0, R1 = 640, 970
N = 6


def lx(v):
    return L0 + (v - LO) / (HI - LO) * (L1 - L0)


def rx(k):
    return R0 + 20 + k / N * (R1 - R0 - 40)


def left():
    out = [text((L0 + L1) / 2, 22, "尾数 3 位的简化格式：每个区间等分为 8 份", 14, INK, "bold"),
           line(L0 - 12, AXIS_Y, L1 + 14, AXIS_Y, LINE, 1.6)]
    for e, name, step in ((-1, "[0.5, 1)", "1/16"), (0, "[1, 2)", "1/8"), (1, "[2, 4)", "1/4")):
        low = 2.0 ** e
        d = low / (1 << FRACTION_BITS)
        for i in range(1 << FRACTION_BITS):
            v = low + i * d
            tall = 11 if i == 0 else 7
            out.append(line(lx(v), AXIS_Y - tall, lx(v), AXIS_Y + tall, BLUE, 1.4 if i == 0 else 1.1))
        out += brace(lx(low), lx(2 * low), AXIS_Y + 18, MUTED, name, 12.5)
        out.append(text((lx(low) + lx(2 * low)) / 2, AXIS_Y + 62, f"间距 {step}", 12.5, BLUE))
    out.append(line(lx(4), AXIS_Y - 11, lx(4), AXIS_Y + 11, BLUE, 1.4))
    for v, s in ((0.5, "0.5"), (1, "1"), (2, "2"), (4, "4")):
        out.append(text(lx(v), AXIS_Y - 17, s, 12.5, MUTED))
    return out


def right():
    out = [text((R0 + R1) / 2, 22, "FP32 在 [2²⁴, 2²⁵) 内：间距为 2", 14, INK, "bold"),
           line(R0, AXIS_Y, R1, AXIS_Y, LINE, 1.6)]
    for k in range(N + 1):
        x = rx(k)
        if k % 2 == 0:
            out.append(line(x, AXIS_Y - 11, x, AXIS_Y + 11, BLUE, 1.4))
            out.append(text(x, AXIS_Y - 17, "2²⁴" if k == 0 else f"+{k}", 12.5, MUTED))
        else:
            out.append(f'<circle cx="{x:.1f}" cy="{AXIS_Y}" r="4.5" fill="#fff" '
                       f'stroke="{RED}" stroke-width="1.6"/>')
            out.append(text(x, AXIS_Y - 17, f"+{k}", 12.5, RED))
    out += brace(rx(0), rx(2), AXIS_Y + 18, MUTED, "间距 2", 12.5)
    out.append(text((R0 + R1) / 2, AXIS_Y + 62, "空心圆：奇数整数落在缝隙中，无法表示", 12.5, RED))
    return out


def build():
    sep = (L1 + R0) / 2 + 6
    return svg(W, H, left() + [line(sep, 12, sep, H - 12, LINE, 1, dash="4 4")] + right())


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "ulp-spacing.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
