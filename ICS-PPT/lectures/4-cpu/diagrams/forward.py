#!/usr/bin/env python3
"""imull computes %ecx in execute; the next addl uses it one cycle later.

Two rows of the shared time-space grid (timegrid.py), one column apart. The
product exists at the end of imull's execute (cycle 3); an orange elbow carries
it straight into addl's execute (cycle 4) instead of waiting for imull's
write-back in cycle 5, so the addl row has no stall cell. The slide's own
highlight line states the conclusion; the figure only annotates the grid.
Nothing is drawn above or below the rows, so the grid sits UP px higher than
in the other animations and the canvas ends under the addl row: the slide has
two paragraphs and a two-line highlight, and a taller figure spills out of its
box onto the highlight.

An animation, one frame per cycle from cycle 2 on: frame k shows the cells of
cycles 1..k+1. The elbow appears with cycle 4, the write-back note with
cycle 5, "addl 没有停" with cycle 6; the last frame is the whole figure.
Run it to refresh ../assets/forward-1.svg .. forward-5.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import GREEN, INK, ORANGE, elbow, save, text
from timegrid import CH, CW, LX, NUM_Y, TOP, VIEW_LEFT, cx, cycle_numbers, row

W, H = 1120, 210
UP = 30                             # no caption above the grid
T0 = TOP - UP                       # top of the imull row (34)
GAP = 64                            # room between the rows for the elbow + label
R2 = T0 + CH + GAP                  # top of the addl row (150)
STAGES = ["取指", "译码", "执行", "访存", "写回"]


UPTO = [2, 3, 4, 5, 6]               # cycles shown in each frame


def build(upto):
    out = cycle_numbers(6, y=NUM_Y - UP)
    out += row("imull", T0, 0, STAGES, upto)
    out += row("addl", R2, 1, STAGES, upto)
    if upto < 4:
        return out
    mid = T0 + CH + GAP / 2         # the horizontal run of the elbow (118)
    out.append(elbow([(cx(2), T0 + CH - 1), (cx(2), mid), (cx(3), mid), (cx(3), R2 + 1)],
                     ORANGE, 2.4))
    out.append(text(LX + 4 * CW + 8, mid + 5,
                    "转发：周期 3 算出的乘积，周期 4 直接送进 addl 的执行", 16, ORANGE, "bold",
                    anchor="start"))
    if upto >= 5:
        out.append(text(LX + 5 * CW + 12, T0 + CH / 2 + 6, "正式写回 %ecx 要到周期 5", 15, INK,
                        anchor="start"))
    if upto >= 6:
        out.append(text(LX + 6 * CW + 12, R2 + CH / 2 + 6, "addl 没有停", 15, GREEN, "bold",
                        anchor="start"))
    return out


if __name__ == "__main__":
    for k, upto in enumerate(UPTO, 1):
        save(f"forward-{k}", W, H, build(upto), left=VIEW_LEFT)
