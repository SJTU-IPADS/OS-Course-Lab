#!/usr/bin/env python3
"""Lecture 2's six weights quantized by the Q4_0 rule of include/nano_quant.h.

The weights are -0.40, 0.00, 0.31, 0.58, 0.87 and 1.40, the ones of lecture
2's exercise quant_example.py, drawn on the same real line. Q4_0 takes the
weight of largest magnitude with its sign, amax = 1.40, and d = amax / -8 =
-0.175, so code 0 sits at amax and code 8 at 0; the sixteen grid points are
d * (q - 8). Each weight's code is min(15, (int)(x * id + 8.5)) with
id = 1 / d, and an arrow runs from the weight to the grid point it restores
to. Grid points outside the data range are grey. The real line, the arrows and
the brackets are drawn by the helpers below, which nq_q4_1.py reuses for the
Q4_1 grid of the same weights. Run it to refresh ../assets/nq_q4_grid.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, INK, LINE, MONO, MUTED, ORANGE, line, rect, save, text

W, H = 1120, 214

FILL_DATA = "#fdf1e8"
GREY = "#b4c0c8"

WEIGHTS = (-0.40, 0.00, 0.31, 0.58, 0.87, 1.40)
LO, HI = -0.4, 1.4

AX0, AX1, VLO, VHI = 100, 1100, -1.7, 1.5
MARKS = [i / 5 for i in range(-8, 8)]           # the real line is marked every 0.2
LABEL_Y, REAL_Y, INT_Y = 16, 56, 156
BAND_TOP = 24


def px(v):
    return AX0 + (v - VLO) / (VHI - VLO) * (AX1 - AX0)


def minus(v, fmt):
    return format(v, fmt).replace("-", "−")


def arrow(x0, y0, x1, y1, color):
    """A dashed shaft with a filled head at (x1, y1)."""
    dx, dy = x1 - x0, y1 - y0
    n = (dx * dx + dy * dy) ** 0.5
    ux, uy = dx / n, dy / n
    hx, hy = x1 - ux * 9, y1 - uy * 9
    pts = (f"{x1:.1f},{y1:.1f} {hx - uy * 4.5:.1f},{hy + ux * 4.5:.1f} "
           f"{hx + uy * 4.5:.1f},{hy - ux * 4.5:.1f}")
    return [line(x0, y0, hx, hy, color, 1.6, "4 3"),
            f'<polygon points="{pts}" fill="{color}"/>']


def bracket(x0, x1, y, color):
    """A bracket just above the integer line, from x0 to x1."""
    y0, y1 = y - 16, y - 23
    return (f'<path d="M {x0:.1f} {y0} L {x0:.1f} {y1} L {x1:.1f} {y1} L {x1:.1f} {y0}" '
            f'fill="none" stroke="{color}" stroke-width="1.5"/>')


def real_line(label, dy=0):
    """The data range, the real line marked every 0.2, and the weights."""
    x0, x1 = px(LO), px(HI)
    ry = REAL_Y + dy
    out = [rect(x0, BAND_TOP + dy, x1 - x0, H - BAND_TOP, FILL_DATA, "none", rx=0, width=0),
           text((x0 + x1) / 2, LABEL_Y + dy, label, 15.5, ORANGE, "bold")]
    out.append(line(AX0, ry, AX1, ry, LINE, 1.5))
    out.append(text(AX0 - 12, ry + 5, "实数 w", 15, INK, "bold", anchor="end"))
    for v in MARKS:
        out.append(line(px(v), ry - 5, px(v), ry + 5, MUTED, 1.2))
        out.append(text(px(v), ry - 11, minus(v, ".1f"), 12.5,
                        INK if v == 0 else MUTED, "bold" if v == 0 else "normal", font=MONO))
    for w in WEIGHTS:
        out.append(f'<circle cx="{px(w):.1f}" cy="{ry}" r="6.5" fill="{ORANGE}"/>')
        out.append(text(px(w), ry + 27, minus(w, ".2f"), 16, ORANGE, "bold", font=MONO))
    return out


def figure(label, grid, codes, step_at, step_label, special, special_note=None, dy=0):
    """The real line over the integer line.

    grid: [(q, value)] in code order; codes: {weight: q}; step_at: the index
    in grid whose gap to the next point carries the step bracket; special:
    the code drawn in orange, with special_note beside it.
    """
    inside = [q for q, v in grid if LO - 1e-9 <= v <= HI + 1e-9]
    outside = [q for q, v in grid if q not in inside]
    out = real_line(label, dy)
    iy = INT_Y + dy
    out.append(line(AX0, iy, AX1, iy, LINE, 1.5))
    out.append(text(AX0 - 12, iy + 5, "整数 q", 15, INK, "bold", anchor="end"))
    for q, v in grid:
        c = ORANGE if q == special else (BLUE if q in inside else GREY)
        out.append(line(px(v), iy - 11, px(v), iy + 11, c, 2.8 if q == special else 2.2))
        out.append(text(px(v), iy + 26, str(q), 16, c, "bold" if q == special else "normal",
                        font=MONO))
    if special_note:
        sx = px(dict(grid)[special])
        anchor = "end" if sx > (AX0 + AX1) / 2 else "start"
        out.append(text(sx + (-8 if anchor == "end" else 8), iy + 46, special_note, 15,
                        ORANGE, "bold", anchor=anchor))
    a, b = px(grid[step_at][1]), px(grid[step_at + 1][1])
    out.append(bracket(min(a, b), max(a, b), iy, BLUE))
    out.append(text((a + b) / 2, iy - 30, step_label, 16, BLUE, "bold"))
    if outside:
        vs = [v for q, v in grid if q in outside]
        a, b = px(min(vs)), px(max(vs))
        out.append(bracket(a, b, iy, MUTED))
        out.append(text((a + b) / 2, iy - 30, f"数据范围外 {len(outside)} 个格点", 15, MUTED,
                        "bold"))
    for w in WEIGHTS:
        out += arrow(px(w), REAL_Y + dy + 34, px(dict(grid)[codes[w]]), iy - 13, ORANGE)
    return out


def q4_0():
    amax = max(WEIGHTS, key=abs)                   # 1.40, positive
    d = amax / -8.0
    idv = 1.0 / d
    grid = [(q, d * (q - 8)) for q in range(16)]
    codes = {w: min(15, int(w * idv + 8.5)) for w in WEIGHTS}
    return figure(f"Q4_0：amax = {amax:.2f}，d = amax / −8 = {minus(d, '.3f')}，"
                  f"数据范围 [−0.4, 1.4] 内 11 个格点",
                  grid, codes, 1, "|d| = 0.175", 0, "q = 0 对应 amax")


if __name__ == "__main__":
    save("nq_q4_grid", W, H, q4_0())
