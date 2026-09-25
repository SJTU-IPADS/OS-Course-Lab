#!/usr/bin/env python3
"""The figure part-5.md asks for on its page 3: where the rounding error goes.

Round the eight midpoints 1.5, 2.5, ..., 8.5 one after another.  Each bar is
one rounding's error, the line is the running sum.  Rounding half up adds
+0.5 every time and the sum climbs to 4; rounding half to even alternates
+0.5 and -0.5 and the sum stays between 0 and 0.5.  Both panels share one y
scale.  Run it to refresh ../assets/round-bias.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import INK, LINE, MUTED, line, rect, svg, text

W, H = 900, 212
RED, FILL_RED = "#c0392b", "#f6cfca"
GREEN, FILL_GREEN = "#196b24", "#cfe8d2"

MIDS = [k + 0.5 for k in range(1, 9)]
Y_LO, Y_HI = -1.0, 4.5
TOP, BOT = 44, 170
SLOT, BAR = 38, 16


def half_up(m):
    return m + 0.5


def half_even(m):
    lo = m - 0.5
    return lo if lo % 2 == 0 else lo + 1


def py(v):
    return BOT - (v - Y_LO) / (Y_HI - Y_LO) * (BOT - TOP)


def panel(x0, title, rule, color, fill, tail):
    out = [text(x0 + len(MIDS) * SLOT / 2, 20, title, 14, color, "bold")]
    right = x0 + len(MIDS) * SLOT
    out.append(line(x0, py(0), right + 8, py(0), INK, 1.3))
    out.append(line(x0, py(Y_LO), x0, py(Y_HI), LINE, 1.2))
    for v in range(0, 5):
        out.append(line(x0 - 4, py(v), x0, py(v), MUTED, 1.1))
        out.append(text(x0 - 8, py(v) + 4, f"{v}", 11.5, MUTED, anchor="end"))
    total, pts = 0.0, []
    for i, m in enumerate(MIDS):
        err = rule(m) - m
        total += err
        cx = x0 + (i + 0.5) * SLOT
        top, bot = sorted((py(0), py(err)))
        out.append(rect(cx - BAR / 2, top, BAR, bot - top, fill, color, rx=1.5, width=1))
        out.append(text(cx, py(Y_LO) + 16, f"{m}", 11.5, MUTED))
        pts.append((cx, py(total)))
    d = " ".join(f"{x:.1f},{y:.1f}" for x, y in pts)
    out.append(f'<polyline points="{d}" fill="none" stroke="{color}" stroke-width="2.2"/>')
    for x, y in pts:
        out.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="3.2" fill="{color}"/>')
    lx, ly = pts[-1]
    if total > 1:
        out.append(text(lx + 12, ly + 4, tail, 12.5, color, "bold", "start"))
    else:
        out.append(text((x0 + right) / 2, py(1.6), tail, 12.5, color, "bold"))
    return out


def build():
    out = panel(60, "传统四舍五入：中点一律向上", half_up, RED, FILL_RED, "累积 +4.0")
    out += panel(520, "向最近偶数舍入：中点一半向上、一半向下", half_even, GREEN, FILL_GREEN,
                 "累积误差在 0 与 0.5 之间振荡")
    out.append(text(W / 2, H - 8, "横轴：依次舍入的 8 个中点　柱：单次舍入的误差（±0.5）　折线：累积误差",
                    12, MUTED))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "round-bias.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
