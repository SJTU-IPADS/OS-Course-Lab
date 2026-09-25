#!/usr/bin/env python3
"""The left half of the figure part-5.md asks for on its page 7: the drift.

The clock counts every 0.1 s and loses 9.54e-8 s per count, so the error is a
straight line in the running time: 3.6 million counts in 100 hours, 0.3433 s.
The right half (the tracking window) is patriot_gate.py.  Run it to refresh
../assets/patriot-drift.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import INK, LINE, MUTED, line, svg, text

W, H = 540, 204
RED = "#c0392b"

PER_TICK = 9.54e-8          # s lost per 0.1 s count
X0, X1, Y0, Y1 = 78, 440, 34, 156
T_MAX, E_MAX = 100.0, 0.4


def px(hours):
    return X0 + hours / T_MAX * (X1 - X0)


def py(err):
    return Y1 - err / E_MAX * (Y1 - Y0)


def build():
    out = [line(X0, Y1, X1 + 10, Y1, MUTED, 1.3), line(X0, Y1, X0, Y0 - 8, MUTED, 1.3)]
    for h in range(0, 101, 20):
        out.append(line(px(h), Y1, px(h), Y1 + 5, MUTED, 1.2))
        out.append(text(px(h), Y1 + 19, f"{h}", 12, MUTED))
    for e in (0.1, 0.2, 0.3, 0.4):
        out.append(line(X0 - 5, py(e), X0, py(e), MUTED, 1.2))
        out.append(line(X0, py(e), X1, py(e), LINE, 0.8, dash="3 4"))
        out.append(text(X0 - 9, py(e) + 4, f"{e:.1f}", 12, MUTED, anchor="end"))
    out.append(text(X0 - 9, py(0) + 4, "0", 12, MUTED, anchor="end"))
    out.append(text((X0 + X1) / 2, Y1 + 40, "连续运行时间（小时）", 13, MUTED))
    out.append(text(X0 - 50, Y0 - 18, "时钟误差（秒）", 13, MUTED, anchor="start"))

    end = 100 * 3600 * 10 * PER_TICK
    out.append(line(px(0), py(0), px(100), py(end), RED, 2.6))
    out.append(f'<circle cx="{px(100):.1f}" cy="{py(end):.1f}" r="5" fill="{RED}"/>')
    out.append(text(px(100) + 10, py(end) - 12, "0.3433 秒", 14, RED, "bold", "start"))
    out.append(text(px(100) + 10, py(end) + 6, "360 万次计数", 12, RED, anchor="start"))
    out.append(text(px(6), py(0.24), "每 0.1 秒计数一次", 12.5, INK, anchor="start"))
    out.append(text(px(6), py(0.24) + 18, "每次少 9.54 × 10⁻⁸ 秒", 12.5, INK, anchor="start"))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "patriot-drift.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
