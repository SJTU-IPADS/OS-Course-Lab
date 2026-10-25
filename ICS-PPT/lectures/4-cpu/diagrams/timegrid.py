"""One time-space grid spec shared by pipeline / forward / stall / branch.

Rows are instructions, columns are cycles; the four figures read as one set
because the cell size, the stage colours and the cycle numbers never move.
This module only defines helpers: importing or running it draws nothing.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_RED,
                    GREEN, INK, MUTED, ORANGE, RED, WHITE, mono, rect, text)

CW, CH = 96, 52          # one cycle, one row
LX = 200                 # grid left edge; row labels live in [24, LX)
NUM_Y = 48               # baseline of the cycle numbers
TOP = 64                 # top of the first row
VIEW_LEFT = -17          # save(..., left=VIEW_LEFT): the row labels hang off the
                         # left, so the window moves 17 px to centre pipeline's
                         # 9 columns on the slide; one value for all four grids
                         # keeps the cells from jumping between pages
TONE = {                 # stage -> (fill, stroke, label colour)
    "取指": (FILL_BLUE, BLUE, BLUE),
    "译码": (FILL_GREY, INK, INK),
    "执行": (FILL_ORANGE, ORANGE, ORANGE),
    "访存": (FILL_GREEN, GREEN, GREEN),
    "写回": (INK, INK, WHITE),
    "停":   (FILL_RED, RED, RED),
    "?":    (WHITE, MUTED, MUTED),
}


def cycle_numbers(n, emph=None, color=ORANGE):
    """'周期' and the numbers 1..n; `emph` is drawn larger in `color`."""
    out = [text(LX - 12, NUM_Y, "周期", 15, MUTED, anchor="end")]
    for c in range(n):
        x = LX + c * CW + CW / 2
        if emph == c + 1:
            out.append(text(x, NUM_Y + 1, str(c + 1), 18, color, "bold"))
        else:
            out.append(text(x, NUM_Y, str(c + 1), 15, MUTED))
    return out


def cell(col, y, name):
    fill, stroke, tone = TONE[name]
    x = LX + col * CW
    dash = "5 4" if name == "?" else None
    return [rect(x + 3, y + 3, CW - 6, CH - 6, fill, stroke, rx=3, width=1.4, dash=dash),
            text(x + CW / 2, y + CH / 2 + 6, name, 17, tone, "bold")]


def row(label, y, first_col, names, upto=None):
    """A row label at x=24 and one cell per name from `first_col` on.

    `upto` draws only the cells in columns before it: the cycles that have
    happened in one frame of an animation."""
    out = [mono(24, y + CH / 2 + 6, label, 16, INK)]
    for k, name in enumerate(names):
        if name and (upto is None or first_col + k < upto):
            out += cell(first_col + k, y, name)
    return out


def col_frame(col, y0, y1, color):
    """Dashed frame around one cycle column, from y0 to y1."""
    return rect(LX + col * CW + 1, y0, CW - 2, y1 - y0, "none", color, rx=6,
                width=2, dash="6 4")


def cx(col):
    """x of a column's centre."""
    return LX + col * CW + CW / 2
