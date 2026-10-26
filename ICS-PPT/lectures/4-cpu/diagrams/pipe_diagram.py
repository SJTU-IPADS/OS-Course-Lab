#!/usr/bin/env python3
"""Three operations against time, unpipelined and in the three-stage pipeline.

ICS1 lecture 17, page 6, "Pipeline Diagrams", shape for shape, at the
coordinates the slide gives (points, each group's origin at its top left)
and the scale of comp_example.py. Two figures, because the page's text runs
between them: pipe-diagram-seq.svg (OP1..OP3 one after the other, a bar
each, 300 ps of logic and the 20 ps register of comp_example.py) and
pipe-diagram-3way.svg (each operation as the cells A, B, C, the rows shifted
one 120 ps cell; a dashed frame marks the column at which three operations
are in process). The bars are laid back to back; the original's third bar
starts one point late.
Below the rows, where the original has its time axis, is the clock:
clock_row() writes 「时钟」 and draws a square wave with a rising edge (the
registers load on it) on every bar or cell boundary, from the first bar or
cell to the last, high for the first half of each period as in clock.py,
whose stroke and height of the high level it takes, and a dashed line on
every edge from the wave up to just above the first row, on top of the
cells, drawn like the cycle line of stage_hw.py (MUTED, 1.6 px, dashed 6 4).
pipe_nonuniform.py draws its clock with the same function.
Both canvases are W wide, the end of the upper figure's last bar, and each
figure has its own window, centring(...) as in comp_example.py, that centres
its ink on the slide: INK_X for the upper figure, INK_X_3WAY for the narrower
lower one, which therefore starts to the right of the upper one. Cells take the combinational-logic blue of pipe_3way.py, since
A, B and C are its three blocks; the cell shape is svgkit.cells. The frame is
drawn like timegrid.col_frame in ooo.py and superscalar.py: orange, 2 px
wide, dashed 6 4, rx 6, 2 px beside the cells and 5 px above and below the
rows. The text is translated.
Run it to refresh ../assets/pipe-diagram-seq.svg and pipe-diagram-3way.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import clock
from comp_example import centring, pt
from svgkit import (BLUE, FILL_BLUE, FONT, INK, MUTED, ORANGE, cells, line, path, rect, save,
                    text)

RH = 24                                    # one row
ROW_X = 66                                 # where OP1 starts, both figures
BAR = 138                                  # one unpipelined operation, 320 ps
CELL = 48                                  # one cycle of the pipeline, 120 ps
W = ROW_X + 3 * BAR                        # the end of OP3's bar, in points
LABEL_X = 61                               # right edge of the OP labels, 5 points
                                           # left of the first cycle line
OPS = ("OP1", "OP2", "OP3")
CLOCK_X = 60                               # 「时钟」 ends 6 px before this
CLOCK_GAP = 12                             # px from the last row down to the high level
LINE_ABOVE = 4                             # px the cycle lines reach above the first row
CLOCK_ROW = 32.5                           # points from the last row to the canvas bottom:
                                           # the gap, the wave, 2 px below it
INK_X = (42, pt(W))                        # x extent of the upper figure's ink in px:
                                           # 「时钟」 .. the end of OP3's bar
INK_X_3WAY = (42, pt(ROW_X + 5 * CELL))    # the lower figure's: 「时钟」 .. OP3's C


def label(name, y):
    return text(pt(LABEL_X), pt(y + RH / 2) + 6, name, 17, INK, anchor="end")


def clock_row(edges, rows_top, rows_end):
    """「时钟」, one clock period per pair of neighbouring edges, and the cycle lines.

    edges are in points, rows_top and rows_end are the top of the first row
    and the bottom of the last. The wave rises on every edge, is high for the
    first half of the period and low for the second, in clock.py's ink, width
    and height; its high level is CLOCK_GAP px below the last row. A dashed
    line on every edge runs from the wave up to LINE_ABOVE px above the first
    row, drawn like the cycle line of stage_hw.py; call this after the cells,
    so the lines lie on top of them.
    """
    hi = pt(rows_end) + CLOCK_GAP
    lo = hi + clock.LO - clock.HI
    xs = [pt(e) for e in edges]
    out = [line(x, pt(rows_top) - LINE_ABOVE, x, hi, MUTED, 1.6, "6 4") for x in xs]
    d = f"M {xs[0]:.1f} {lo:.1f}"
    for a, b in zip(xs, xs[1:]):
        m = (a + b) / 2
        d += f" L {a:.1f} {hi:.1f} L {m:.1f} {hi:.1f} L {m:.1f} {lo:.1f} L {b:.1f} {lo:.1f}"
    d += f" L {xs[-1]:.1f} {hi:.1f}"
    return out + [path(d, INK, 2, head=False),
                  text(pt(CLOCK_X) - 6, (hi + lo) / 2 + 17 * 0.36, "时钟", 17, INK,
                       anchor="end")]


def seq():
    """OP1..OP3 one after the other: a 320 ps bar each, no stages."""
    pad = 4                                # room for the cycle lines above the first row
    out = []
    for k, name in enumerate(OPS):
        y = pad + k * RH
        out.append(label(name, y))
        bar, _ = cells(pt(ROW_X + k * BAR), pt(y), [""], pt(BAR), pt(RH), FILL_BLUE, BLUE)
        out += bar
    out += clock_row([ROW_X + k * BAR for k in range(4)], pad, pad + 3 * RH)
    return out, pad + 3 * RH + CLOCK_ROW


def pipelined():
    """Each operation as A, B, C; row k starts one cell later than row k - 1."""
    pad = 5                                # room for the frame and the cycle lines above
                                           # the first row
    out = []
    for k, name in enumerate(OPS):
        y = pad + k * RH
        out.append(label(name, y))
        row, _ = cells(pt(ROW_X + k * CELL), pt(y), list("ABC"), pt(CELL), pt(RH), FILL_BLUE,
                       BLUE, size=17, font=FONT)
        out += row
    out += clock_row([ROW_X + k * CELL for k in range(6)], pad, pad + 3 * RH)
    x = pt(ROW_X + 2 * CELL)               # the column of OP1 in C, OP2 in B, OP3 in A
    out.append(rect(x - 2, pt(pad) - 5, pt(CELL) + 4, pt(3 * RH) + 10, "none", ORANGE,
                    rx=6, width=2, dash="6 4"))
    return out, pad + 3 * RH + CLOCK_ROW


if __name__ == "__main__":
    for name, (body, h), ink in (("pipe-diagram-seq", seq(), INK_X),
                                 ("pipe-diagram-3way", pipelined(), INK_X_3WAY)):
        save(name, pt(W), pt(h), body, left=centring(ink, pt(W)))
