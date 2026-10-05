#!/usr/bin/env python3
"""The i9-11900H roofline, log-log, with the model of the lecture on it.

Both roofs are published values: peak compute 320 GFLOPS (Intel, APP Metrics
for Intel Microprocessors) and memory bandwidth 51.2 GB/s (Intel product
specifications, dual-channel DDR4-3200). They meet at 320 / 51.2 = 6.25
FLOP/Byte.

The model is the language model of Qwen3-VL-2B-Instruct in Q4_0: a token
takes 1.72e9 multiply-adds, counted as 2 operations each, and reads 0.968 GB
of weights, so it sits at 3.44 / 0.968 = 3.55 FLOP/Byte, under the memory
roof at 182 GFLOPS, which is 51.2 / 0.968 = 53 tokens a second.

frame() is shared with roofline_measured.py, which adds the measured points.
Run it to refresh ../assets/roofline.svg.
"""

import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_RED, FONT, INK, MUTED, RED, WHITE, arrow,
                    circle, line, rect, save, text)

W, H = 1120, 284
PX0, PX1, PY0, PY1 = 150, 1000, 34, 224     # plot area
I_LO, OCTAVES = 1, 4                         # x axis: 1 .. 16 FLOP/Byte
P_LO, DECADES = 1, 3                         # y axis: 1 .. 1000 GFLOPS
BW = 51.2                                    # GB/s
PEAK = 320.0                                 # GFLOPS
FLOP = 3.44                                  # 1e9 FLOP per token
GB = 0.968                                   # GB of weights read per token
AI = FLOP / GB                               # FLOP/Byte


def gx(i):
    return PX0 + math.log2(i / I_LO) * (PX1 - PX0) / OCTAVES


def gy(p):
    return PY1 - math.log10(p / P_LO) * (PY1 - PY0) / DECADES


def gflops(tokens_per_s):
    return tokens_per_s * FLOP


def dashed(x, y0, y1, gaps):
    """A dashed vertical line from y0 down to y1 that skips the (top, bottom) gaps."""
    out = []
    for top, bottom in sorted(gaps):
        out.append(line(x, y0, x, top, MUTED, 1.2, "4 4"))
        y0 = bottom
    out.append(line(x, y0, x, y1, MUTED, 1.2, "4 4"))
    return out


def frame(ridge_gaps=()):
    """Axes, the two roofs, and the model's bound under the memory roof."""
    ridge = PEAK / BW
    xr, yr = gx(ridge), gy(PEAK)
    mid = (PY0 + PY1) / 2 + 46
    out = [rect(PX0, PY0, xr - PX0, PY1 - PY0, FILL_RED, "none", rx=0, width=0),
           rect(xr, PY0, PX1 - xr, PY1 - PY0, FILL_BLUE, "none", rx=0, width=0),
           text(PX0 + 90, mid, "访存受限区", 17, RED, "bold"),
           text((xr + PX1) / 2, mid, "算力受限区", 17, BLUE, "bold")]
    # axes
    out.append(arrow(PX0, PY1, PX1 + 20, PY1, INK, 1.8))
    out.append(arrow(PX0, PY1, PX0, PY0 - 16, INK, 1.8))
    for i in (1, 2, 8, 16):
        out.append(line(gx(i), PY1, gx(i), PY1 + 6, INK, 1.4))
        out.append(text(gx(i), PY1 + 24, f"{i:g}", 14, MUTED))
    for p in (1, 10, 100, 1000):
        out.append(line(PX0 - 6, gy(p), PX0, gy(p), INK, 1.4))
        out.append(text(PX0 - 10, gy(p) + 5, f"{p:g}", 14, MUTED, anchor="end"))
    out.append(text((PX0 + PX1) / 2, PY1 + 52, "算术强度（FLOP/Byte，对数坐标）", 16, INK))
    out.append(text(PX0 + 14, PY0 - 12, "运算速率（GFLOPS，对数坐标）", 16, INK,
                    anchor="start"))
    # the roofs and the point where they meet
    x0, y0 = gx(I_LO), gy(BW * I_LO)
    out += dashed(xr, yr, PY1 + 6, ridge_gaps)
    out.append(text(xr, PY1 + 24, f"{ridge:g}", 14, INK, "bold"))
    out.append(line(x0, y0, xr, yr, RED, 3.4))
    out.append(line(xr, yr, PX1, yr, BLUE, 3.4))
    out.append(text(xr, yr - 14, f"平衡点 {ridge:g} FLOP/Byte", 15, INK, "bold"))
    out.append(text(PX1 - 8, yr + 26, f"峰值算力 P = {PEAK:g} GFLOPS", 16, BLUE, "bold",
                    anchor="end"))
    ang = math.degrees(math.atan2(yr - y0, xr - x0))
    mx, my = gx(1.41), gy(BW * 1.41) - 9
    out.append(f'<text x="{mx:.1f}" y="{my:.1f}" font-family="{FONT}" font-size="15" '
               f'font-weight="bold" fill="{RED}" text-anchor="middle" '
               f'transform="rotate({ang:.1f} {mx:.1f} {my:.1f})">'
               f'斜率 = 内存带宽 B = {BW:g} GB/s</text>')
    # the model: where it sits and the roof above it
    px, pr = gx(AI), gy(BW * AI)
    out.append(line(px, pr, px, PY1 + 6, MUTED, 1.2, "4 4"))
    out.append(text(px, PY1 + 24, f"{AI:.1f}", 14, INK, "bold"))
    out.append(circle(px, pr, 8, WHITE, RED, 3))
    out.append(text(px - 16, pr - 18, f"带宽上限 {BW / GB:.0f} Token/s（{BW * AI:.0f} GFLOPS）",
                    15, RED, "bold", anchor="end"))
    out.append(text(px - 10, PY1 - 10, "Qwen3-VL-2B（Q4_0）", 14, MUTED, anchor="end"))
    return out


if __name__ == "__main__":
    save("roofline", W, H, frame())
