#!/usr/bin/env python3
"""The memory wall, measured: multiply-adds per second against threads.

Three runs of examples/matvec_q4.c: the AVX2 build with W = 8 MiB (in cache),
the AVX2 build with W = 512 MiB (from DRAM) and the scalar build with
W = 512 MiB. The dashed line is 51.2 GB/s of int4 weights, two multiply-adds
per byte. The numbers are the GMAC/s column of the runs on the page.
Run it to refresh ../assets/wall.svg.
"""

import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, INK, LINE, MUTED, ORANGE, RED, arrow, circle, line,
                    save, text)

W, H = 1120, 450
PX0, PX1, PY0, PY1 = 110, 700, 50, 380      # plot area
THREADS = (1, 2, 4, 8)
LIMIT = 51.2 * 2                             # the bandwidth line, 1e9 per second

# (label, colour, GMAC/s at 1, 2, 4, 8 threads)
RUNS = [("AVX2，W = 8 MiB（缓存）", BLUE, (25.14, 49.89, 81.84, 119.67)),
        ("AVX2，W = 512 MiB（内存）", ORANGE, (16.40, 27.73, 52.21, 66.03)),
        ("标量，W = 512 MiB（内存）", MUTED, (1.04, 1.81, 4.28, 4.74))]
YMAX = 40 * math.ceil(max(max(v) for _, _, v in RUNS) / 40)


def gx(k):
    return PX0 + 60 + k * (PX1 - PX0 - 120) / (len(THREADS) - 1)


def gy(v):
    return PY1 - v * (PY1 - PY0) / YMAX


def build():
    out = [arrow(PX0, PY1, PX1 + 20, PY1, INK, 1.8),
           arrow(PX0, PY1, PX0, PY0 - 16, INK, 1.8),
           text(PX0 + 14, PY0 - 22, "乘加速率（10⁹ 次 / 秒）", 16, INK, anchor="start"),
           text((PX0 + PX1) / 2, PY1 + 58, "线程数", 16, INK)]
    for v in range(0, YMAX + 1, 40):
        if v:
            out.append(line(PX0, gy(v), PX1, gy(v), LINE, 0.8, "2 4"))
        out.append(text(PX0 - 10, gy(v) + 5, f"{v}", 14, MUTED, anchor="end"))
    for k, t in enumerate(THREADS):
        out.append(line(gx(k), PY1, gx(k), PY1 + 6, INK, 1.4))
        out.append(text(gx(k), PY1 + 26, f"{t}", 16, INK, "bold"))
    out.append(line(PX0, gy(LIMIT), PX1, gy(LIMIT), RED, 2.4, "8 6"))
    out.append(text(PX1 + 12, gy(LIMIT) - 4, "带宽上限 102", 15, RED, "bold", anchor="start"))
    out.append(text(PX1 + 12, gy(LIMIT) + 18, "51.2 GB/s × 2 次 / 字节", 13, RED,
                    anchor="start"))
    for label, colour, values in RUNS:
        pts = [(gx(k), gy(v)) for k, v in enumerate(values)]
        d = " ".join(f"{'M' if k == 0 else 'L'} {x:.1f} {y:.1f}" for k, (x, y) in
                     enumerate(pts))
        out.append(f'<path d="{d}" fill="none" stroke="{colour}" stroke-width="3"/>')
        for k, ((x, y), v) in enumerate(zip(pts, values)):
            # the label goes left of the point when another run's point sits just above
            crowded = any(0 < y - gy(o[k]) < 34 for _, _, o in RUNS)
            tag = f"{v:.0f}" if v >= 10 else f"{v:.1f}"
            out.append(circle(x, y, 6, colour, colour))
            out.append(text(x - 14, y + 5, tag, 14, colour, "bold", anchor="end") if crowded
                       else text(x, y - 14, tag, 14, colour, "bold"))
    # legend
    for k, (label, colour, _) in enumerate(RUNS):
        y = 260 + k * 40
        out.append(line(PX1 + 40, y - 5, PX1 + 80, y - 5, colour, 3))
        out.append(circle(PX1 + 60, y - 5, 6, colour, colour))
        out.append(text(PX1 + 92, y, label, 15, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("wall", W, H, build())
