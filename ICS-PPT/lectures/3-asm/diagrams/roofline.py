#!/usr/bin/env python3
"""A roofline, log-log, with LLM decoding on the bandwidth slope.

The slope is 50 GB/s of DRAM bandwidth. An int8 7B model reads 7 GB of
weights per token for 7e9 multiply-adds: one multiply-add (two operations)
per byte, so it sits at 2 ops/byte and 100 GOPs/s, i.e. 7.1 tokens/s. The
compute roof is drawn schematically; no peak value is claimed.
Run it to refresh ../assets/roofline.svg.
"""

import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_RED, INK, LINE, MUTED, ORANGE, RED,
                    arrow, circle, line, mono, rect, save, text)

W, H = 1120, 470
PX0, PX1, PY0, PY1 = 130, 1000, 50, 390     # plot area
BW = 50                                      # GB/s
PEAK = 2000                                  # GOPs/s, schematic only


def gx(i):
    return PX0 + (math.log10(i) + 1) * (PX1 - PX0) / 4       # 0.1 .. 1000


def gy(p):
    return PY1 - math.log10(p) * (PY1 - PY0) / 4             # 1 .. 10000


def build():
    ridge = PEAK / BW
    out = [rect(PX0, PY0, gx(ridge) - PX0, PY1 - PY0, FILL_RED, "none", rx=0, width=0),
           rect(gx(ridge), PY0, PX1 - gx(ridge), PY1 - PY0, FILL_BLUE, "none", rx=0,
                width=0),
           text((PX0 + gx(ridge)) / 2, PY0 + 34, "访存受限区（Memory Bound）", 17, RED,
                "bold"),
           text((gx(ridge) + PX1) / 2, PY1 - 60, "计算受限区", 17, BLUE, "bold"),
           text((gx(ridge) + PX1) / 2, PY1 - 36, "（Compute Bound）", 15, BLUE)]
    # axes
    out.append(arrow(PX0, PY1, PX1 + 20, PY1, INK, 1.8))
    out.append(arrow(PX0, PY1, PX0, PY0 - 16, INK, 1.8))
    for i in (0.1, 1, 10, 100, 1000):
        out.append(line(gx(i), PY1, gx(i), PY1 + 6, INK, 1.4))
        out.append(mono(gx(i), PY1 + 24, f"{i:g}", 14, MUTED, anchor="middle"))
    out.append(text((PX0 + PX1) / 2, PY1 + 58, "计算强度（运算次数 / Byte，对数坐标）", 16, INK))
    out.append(text(PX0 + 14, PY0 - 8, "性能（GOPs/s，对数坐标）", 16, INK, anchor="start"))
    # the roofs
    x0, y0 = gx(0.1), gy(BW * 0.1)
    xr, yr = gx(ridge), gy(PEAK)
    out.append(line(x0, y0, xr, yr, RED, 3.4))
    out.append(line(xr, yr, PX1, yr, BLUE, 3.4))
    out.append(text(xr + 20, yr - 12, "计算峰值", 15, BLUE, "bold", anchor="start"))
    ang = math.degrees(math.atan2(yr - y0, xr - x0))
    mx, my = gx(0.4), gy(BW * 0.4) - 14
    out.append(f'<text x="{mx:.1f}" y="{my:.1f}" font-family="PingFang SC, Noto Sans CJK '
               f'SC, sans-serif" font-size="15" font-weight="bold" fill="{RED}" '
               f'text-anchor="middle" transform="rotate({ang:.1f} {mx:.1f} {my:.1f})">'
               f'斜率 = 内存带宽 50 GB/s</text>')
    # LLM decoding
    px, py = gx(2), gy(BW * 2)
    out.append(line(px, py, px, PY1, MUTED, 1.2, "4 4"))
    out.append(line(PX0, py, px, py, MUTED, 1.2, "4 4"))
    out.append(mono(PX0 - 8, py + 5, "100", 14, MUTED, anchor="end"))
    out.append(circle(px, py, 8, ORANGE, ORANGE))
    out.append(text(px + 18, py + 30, "大模型自回归解码", 16, ORANGE, "bold", anchor="start"))
    out.append(text(px + 18, py + 54, "每字节权重 1 次乘加（2 次运算）", 14, INK,
                    anchor="start"))
    out.append(text(px + 18, py + 76, "上限 50 GB/s ÷ 7 GB ≈ 7.1 Token/s", 14, INK,
                    anchor="start"))
    return out


if __name__ == "__main__":
    save("roofline", W, H, build())
