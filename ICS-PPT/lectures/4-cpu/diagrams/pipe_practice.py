#!/usr/bin/env python3
"""Six logic blocks A-F of 80, 30, 60, 50, 70 and 10 ps, then one 20 ps register.

ICS1 lecture 17, page 11, "Practice Problem" (CS:APP 4.4), element for
element: an arrow into every box, the time of every box above it, the
register at the end with its clock input going down to the label. The
original is a bitmap, so the shapes take the measures of pipe_3way.py: a
block is as wide as its time at that page's 74 pt per 100 ps, all boxes are
as tall as there, the register and the arrows between boxes are the same,
and points become pixels by comp_example.py's S. The clock input is drawn as
in comp_example.py. Colours as in components.py: combinational logic blue,
the clocked register orange. The text is translated. A letter is written in
its block; F is too narrow for one, so its letter is centred under it.
The canvas is the ink plus PAD on either side, so the page centres the drawing.
Run it to refresh ../assets/pipe-practice.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from comp_example import pt, register
from pipe_3way import BH, LW, RW, TOP
from svgkit import BLUE, FILL_BLUE, INK, arrow, line, rect, save, text

BLOCKS = (("A", 80), ("B", 30), ("C", 60), ("D", 50), ("E", 70), ("F", 10))
REG_PS = 20
PER_PS = LW / 100            # points per ps: pipe_3way.py's 100 ps block is LW wide
GAP = 38                     # the arrow before every box, points, as in pipe_3way.py
TIME_Y = 24                  # baseline of the times, points
CLOCK_LEN = 21               # the clock input below the register, points,
CLOCK_LABEL = 50             # and the baseline of 时钟 below it, as in comp_example.py
LETTER = 18                  # size of the block letters
FITS = 1.5 * LETTER          # a block narrower than this, in px, has its letter below
INK_RIGHT = 22               # half the width of "20 ps" as rendered, px
PAD = 8                      # blank margin left and right of the ink, px
H = TOP + BH + CLOCK_LABEL + 11


def block(x, name, ps):
    """One logic block at x (points), its time above it and its letter."""
    w = ps * PER_PS
    mid, wide = pt(TOP + BH / 2), pt(w)
    out = [rect(pt(x), pt(TOP), wide, pt(BH), FILL_BLUE, BLUE, rx=min(6, wide / 4), width=1.6),
           text(pt(x + w / 2), pt(TIME_Y), f"{ps} ps", 18, INK)]
    if wide >= FITS:
        out.append(text(pt(x + w / 2), mid + LETTER * 0.36, name, LETTER, INK, "bold"))
    else:
        out.append(text(pt(x + w / 2), pt(TOP + BH) + LETTER + 4, name, LETTER, INK, "bold"))
    return out, x + w


def build():
    out, x, y = [], 0, pt(TOP + BH / 2)
    for name, ps in BLOCKS:
        out.append(arrow(pt(x) + (2 if x else 0), y, pt(x + GAP) - 2, y, INK, 2.2))
        shapes, x = block(x + GAP, name, ps)
        out += shapes
    out.append(arrow(pt(x) + 2, y, pt(x + GAP) - 2, y, INK, 2.2))
    rx = x + GAP
    out += register(rx, TOP, RW, BH)
    cx = pt(rx + RW / 2)
    out.append(text(cx, pt(TIME_Y), f"{REG_PS} ps", 18, INK))
    out.append(line(cx, pt(TOP + BH), cx, pt(TOP + BH + CLOCK_LEN), INK, 1.6))
    out.append(text(cx, pt(TOP + BH + CLOCK_LABEL), "时钟", 18, INK))
    return out, cx + INK_RIGHT


if __name__ == "__main__":
    body, right = build()
    save("pipe-practice", round(right + 2 * PAD), round(pt(H)), body, left=-PAD)
