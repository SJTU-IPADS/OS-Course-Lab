#!/usr/bin/env python3
"""The pipeline left as it is: addl reads %ecx before imull has written it.

Two rows of the shared time-space grid (timegrid.py), one column apart and no
stall cell: imull in cycles 1-5, addl in cycles 2-6. Between the rows is the
value of %ecx in each cycle, so the reader sees which value addl gets. Before
imull, %ecx holds x[i] = 4; imull computes 3 x 4 = 12 in cycle 3 and writes it
at the clock edge that ends cycle 5. addl decodes in cycle 3 and reads the 4
(red arrow from the %ecx cell of cycle 3); its execute adds 10 + 4 = 14, and
the row ends with the wrong sum against the right one. The slide states the
sum being computed in a formula above the figure, so nothing is drawn above
the grid and the grid sits UP px higher than on the animated pages.

Run it to refresh ../assets/hazard.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREY, FILL_RED, INK, MONO, MUTED, ORANGE, RED, WHITE, arrow, elbow,
                    rect, save, text)
from timegrid import CH, CW, LX, NUM_Y, TOP, VIEW_LEFT, cx, cycle_numbers, row

W, H = 1120, 300
UP = 30                             # no caption above the grid
T0 = TOP - UP                       # top of the imull row (34)
STAGES = ["取指", "译码", "执行", "访存", "写回"]
GAP = 46                            # between a row and the %ecx strip
S = T0 + CH + GAP                   # top of the %ecx strip (132)
SH = 34                             # height of the strip
R2 = S + SH + GAP                   # top of the addl row (212)
MID = T0 + CH + GAP / 2             # the write-back elbow's horizontal run (109)
ECX = ["4", "4", "4", "4", "4", "12", "12"]   # %ecx in cycles 1..7


def strip():
    out = [text(24, S + SH / 2 + 6, "%ecx 的值", 16, INK, anchor="start")]
    for c, v in enumerate(ECX):
        x = LX + c * CW + 3
        if c == 2:                  # the cell addl reads
            out.append(rect(x, S, CW - 6, SH, FILL_RED, RED, rx=3, width=1.6))
            out.append(text(x + (CW - 6) / 2, S + SH / 2 + 6, v, 17, RED, "bold", font=MONO))
        else:
            fill = FILL_GREY if v == "12" else WHITE
            out.append(rect(x, S, CW - 6, SH, fill, MUTED, rx=3, width=1.2))
            out.append(text(x + (CW - 6) / 2, S + SH / 2 + 6, v, 16, INK, font=MONO))
    return out


def build():
    out = cycle_numbers(7, emph=3, color=RED, y=NUM_Y - UP)
    out += row("imull", T0, 0, STAGES)
    out.append(text(cx(2), MID + 6, "3 × 4 = 12", 15, ORANGE, "bold"))
    out.append(text(LX + 5 * CW + 12, T0 + CH / 2 + 6, "周期 5 结束时 12 写入 %ecx", 15, INK,
                    anchor="start"))
    out.append(elbow([(cx(4), T0 + CH - 1), (cx(4), MID), (cx(5), MID), (cx(5), S - 1)],
                     INK, 2.4))
    out += strip()
    out.append(arrow(cx(2), S + SH + 1, cx(2), R2 + 1, RED, 2.4))
    out.append(text(cx(2) + 12, S + SH + GAP / 2 + 6, "周期 3 译码，读到旧值 4", 16, RED, "bold",
                    anchor="start"))
    out += row("addl", R2, 1, STAGES)
    out.append(text(cx(3), R2 + CH + 24, "10 + 4 = 14", 15, RED, "bold"))
    out.append(text(LX + 6 * CW + 12, R2 + CH / 2 + 6, "%r9d = 14，应为 10 + 12 = 22", 16, RED,
                    "bold", anchor="start"))
    return out


if __name__ == "__main__":
    save("hazard", W, H, build(), left=VIEW_LEFT)
