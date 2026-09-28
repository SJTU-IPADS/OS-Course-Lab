#!/usr/bin/env python3
"""The i9-11900H roofline, log-log, with the scalar int4 matvec on it.

Memory roof: dual-channel DDR4-3200, 51.2 GB/s. Compute roof: 8 cores x 64
int8 multiply-adds per cycle (vpdpbusd) x 4.0 GHz = 2.05e12 per second; the
ridge is at 40 multiply-adds per byte. An int4 weight is half a byte and is
used once, so the matvec sits at 2 multiply-adds per byte, under the memory
roof at 1.02e11 per second. SCALAR is what ./matvec_scalar measured.
Run it to refresh ../assets/roofline.svg.
"""

import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_RED, INK, MUTED, ORANGE, RED, WHITE,
                    arrow, circle, line, rect, save, text)

W, H = 1120, 470
PX0, PX1, PY0, PY1 = 130, 1000, 50, 390     # plot area
BW = 51.2e9                                  # bytes per second
PEAK = 8 * 64 * 4.0e9                        # multiply-adds per second
AI = 2                                       # multiply-adds per byte, int4
SCALAR = 1.07e9                              # ./matvec_scalar, multiply-adds per second
SUP = str.maketrans("0123456789", "⁰¹²³⁴⁵⁶⁷⁸⁹")


def gx(i):
    return PX0 + (math.log10(i) + 1) * (PX1 - PX0) / 4       # 0.1 .. 1000


def gy(p):
    return PY1 - (math.log10(p) - 8) * (PY1 - PY0) / 5       # 1e8 .. 1e13


def build():
    ridge = PEAK / BW
    out = [rect(PX0, PY0, gx(ridge) - PX0, PY1 - PY0, FILL_RED, "none", rx=0, width=0),
           rect(gx(ridge), PY0, PX1 - gx(ridge), PY1 - PY0, FILL_BLUE, "none", rx=0,
                width=0),
           text((PX0 + gx(ridge)) / 2, PY0 + 34, "访存受限区", 17, RED, "bold"),
           text((gx(ridge) + PX1) / 2, PY1 - 36, "计算受限区", 17, BLUE, "bold")]
    # axes
    out.append(arrow(PX0, PY1, PX1 + 20, PY1, INK, 1.8))
    out.append(arrow(PX0, PY1, PX0, PY0 - 16, INK, 1.8))
    for i in (0.1, 1, 10, 100, 1000):
        out.append(line(gx(i), PY1, gx(i), PY1 + 6, INK, 1.4))
        out.append(text(gx(i), PY1 + 24, f"{i:g}", 14, MUTED))
    for e in range(8, 14):
        out.append(line(PX0 - 6, gy(10 ** e), PX0, gy(10 ** e), INK, 1.4))
        out.append(text(PX0 - 10, gy(10 ** e) + 5, "10" + str(e).translate(SUP), 15, MUTED,
                        anchor="end"))
    out.append(text((PX0 + PX1) / 2, PY1 + 58, "计算强度（每字节权重的乘加次数，对数坐标）", 16,
                    INK))
    out.append(text(PX0 + 14, PY0 - 8, "乘加速率（次 / 秒，对数坐标）", 16, INK,
                    anchor="start"))
    # the roofs
    x0, y0 = gx(0.1), gy(BW * 0.1)
    xr, yr = gx(ridge), gy(PEAK)
    out.append(line(x0, y0, xr, yr, RED, 3.4))
    out.append(line(xr, yr, PX1, yr, BLUE, 3.4))
    out.append(text(PX1, yr + 30, "峰值算力 2.0 × 10¹² 次 / 秒", 16, BLUE, "bold",
                    anchor="end"))
    out.append(text(PX1, yr + 54, "8 核 × 64 次 / 周期 × 4.0 GHz", 14, MUTED, anchor="end"))
    ang = math.degrees(math.atan2(yr - y0, xr - x0))
    mx, my = gx(0.35), gy(BW * 0.35) + 26
    out.append(f'<text x="{mx:.1f}" y="{my:.1f}" font-family="PingFang SC, Noto Sans CJK '
               f'SC, sans-serif" font-size="15" font-weight="bold" fill="{RED}" '
               f'text-anchor="middle" transform="rotate({ang:.1f} {mx:.1f} {my:.1f})">'
               f'斜率 = 内存带宽 51.2 GB/s</text>')
    # the int4 matvec: its roof and the measured scalar program
    px, pr, ps = gx(AI), gy(BW * AI), gy(SCALAR)
    out.append(line(px, pr, px, PY1, MUTED, 1.2, "4 4"))
    out.append(text(px, PY1 + 24, "2", 14, INK, "bold"))
    out.append(circle(px, pr, 8, WHITE, RED, 3))
    out.append(text(px - 18, pr - 14, "带宽上限 1.0 × 10¹¹", 15, RED, "bold", anchor="end"))
    out.append(circle(px, ps, 8, ORANGE, ORANGE))
    out.append(text(px + 18, ps + 5, f"标量程序实测 {SCALAR / 1e9:.1f} × 10⁹", 15, ORANGE,
                    "bold", anchor="start"))
    out.append(arrow(px + 30, pr + 12, px + 30, ps - 14, INK, 1.8, both=True))
    out.append(text(px + 44, (pr + ps) / 2 + 5, f"相差 {BW * AI / SCALAR:.0f} 倍", 16, INK,
                    "bold", anchor="start"))
    return out


if __name__ == "__main__":
    save("roofline", W, H, build())
