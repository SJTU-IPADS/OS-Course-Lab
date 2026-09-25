#!/usr/bin/env python3
"""The right half of the figure part-5.md asks for on its page 7: the miss.

The radar places its range gate where the clock says the missile should be;
with the clock 0.3433 s behind, that spot trails a 3750 mph (about 1676 m/s)
missile by about 575 m, and the missile flies outside the gate.  Not to scale.
The left half (the drift itself) is patriot_drift.py.  Run it to refresh
../assets/patriot-gate.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import BLUE, INK, LINE, MUTED, line, rect, svg, text

W, H = 560, 214
RED = "#c0392b"

START, STOP = (40, 22), (430, 176)      # the trajectory, top left to ground
GATE_T, ACTUAL_T = 0.36, 0.70           # where along it the two points sit
RADAR = (500, 184)


def along(t):
    return START[0] + t * (STOP[0] - START[0]), START[1] + t * (STOP[1] - START[1])


def build():
    out = [line(10, 190, W - 10, 190, MUTED, 1.4)]
    # trajectory
    out.append(line(*START, *STOP, INK, 1.6, dash="7 5"))
    out.append(text(START[0] + 6, START[1] - 6, "飞毛腿导弹的轨迹", 12.5, MUTED, anchor="start"))

    gx, gy = along(GATE_T)
    ax, ay = along(ACTUAL_T)
    # the range gate the radar computed
    gw, gh = 96, 64
    out.append(rect(gx - gw / 2, gy - gh / 2, gw, gh, "#eaf3f7", BLUE, rx=4, width=2,
                    dash="6 4"))
    out.append(f'<circle cx="{gx:.1f}" cy="{gy:.1f}" r="4.5" fill="{BLUE}"/>')
    out.append(text(gx - gw / 2 - 8, gy - 8, "雷达计算出的", 12.5, BLUE, "bold", "end"))
    out.append(text(gx - gw / 2 - 8, gy + 10, "跟踪窗口", 12.5, BLUE, "bold", "end"))
    # the missile itself
    out.append(f'<circle cx="{ax:.1f}" cy="{ay:.1f}" r="7" fill="{RED}"/>')
    out.append(text(ax - 2, ay + 30, "导弹的实际位置", 12.5, RED, "bold"))
    # offset between them, drawn above the trajectory
    dx, dy = ax - gx, ay - gy
    n = (dx * dx + dy * dy) ** 0.5
    ox, oy = dy / n * 46, -dx / n * 46
    x1, y1, x2, y2 = gx + ox, gy + oy, ax + ox, ay + oy
    # dimension lines: from each point up to the arrow
    for (px, py) in ((gx, gy), (ax, ay)):
        out.append(line(px + ox * 0.2, py + oy * 0.2, px + ox * 1.15, py + oy * 1.15,
                        RED, 1, dash="3 3"))
    out.append(line(x1, y1, x2 - dx / n * 8, y2 - dy / n * 8, RED, 2.2))
    ux, uy = dx / n, dy / n
    head = [(x2, y2), (x2 - ux * 9 - uy * 5, y2 - uy * 9 + ux * 5),
            (x2 - ux * 9 + uy * 5, y2 - uy * 9 - ux * 5)]
    out.append('<polygon points="' + " ".join(f"{p:.1f},{q:.1f}" for p, q in head)
               + f'" fill="{RED}"/>')
    mx, my = (x1 + x2) / 2 + ox * 0.25, (y1 + y2) / 2 + oy * 0.25
    out.append(text(mx + 4, my - 10, "ΔS ≈ 1676 m/s × 0.3433 s", 12.5, RED, anchor="start"))
    out.append(text(mx + 4, my + 8, "≈ 575 米", 14, RED, "bold", "start"))

    # radar on the ground, looking at the gate
    rx, ry = RADAR
    out.append(line(rx, ry, gx + gw / 2, gy - gh / 2, LINE, 1, dash="3 4"))
    out.append(line(rx, ry, gx + gw / 2, gy + gh / 2, LINE, 1, dash="3 4"))
    out.append(f'<path d="M {rx - 14:.1f} {ry:.1f} A 14 14 0 0 1 {rx + 14:.1f} {ry:.1f} Z" '
               f'fill="{INK}"/>')
    out.append(text(rx, ry + 22, "雷达", 12.5, INK, "bold"))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "patriot-gate.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
