#!/usr/bin/env python3
"""The same computation cut into three 100 ps blocks, a 20 ps register after each.

ICS1 lecture 17, page 5, "3-Way Pipelined Version", shape for shape, at the
coordinates the slide gives (points, group origin at the top left) and the
scale of comp_example.py, so the logic and register boxes of the two pages
match. Colours as in components.py: combinational logic blue, clocked
registers orange. The clock reaches the three registers through one line.
Nothing is added or dropped; the text is translated. The ink is centred on
the canvas as in comp_example.py.
Run it to refresh ../assets/pipe-3way.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from comp_example import S, centring, pt, register
from svgkit import BLUE, FILL_BLUE, INK, arrow, line, rect, save, text

W, H = 688, 198                            # the original group, in points
INK_X = (0, 850)                           # x extent of the ink in px, measured on the
                                           # rendered slide: the input arrow .. the end
                                           # of "吞吐率 = 8.33 GOPS"
LOGIC_X = (38, 207, 376)                   # the three blocks A, B, C
REG_X = (151, 320, 489)                    # the register after each
LW, RW, TOP, BH = 74, 18, 32, 101          # widths, top and height of the boxes
ARROW_Y = 80
CLOCK_Y = 150                              # the line joining the three clock inputs
STAGES = "ABC"


def build():
    out = []
    for k, (lx, rx) in enumerate(zip(LOGIC_X, REG_X)):
        out.append(rect(pt(lx), pt(TOP), pt(LW), pt(BH), FILL_BLUE, BLUE, rx=6, width=1.6))
        out.append(text(pt(lx + LW / 2), pt(TOP + BH / 2) - 4, "组合逻辑", 17, INK, "bold"))
        out.append(text(pt(lx + LW / 2), pt(TOP + BH / 2) + 20, STAGES[k], 17, INK, "bold"))
        out.append(text(pt(lx + LW / 2), pt(24), "100 ps", 18, INK))
        out += register(rx, TOP, RW, BH)
        out.append(text(pt(rx + RW / 2), pt(24), "20 ps", 18, INK))
        before = 0 if k == 0 else REG_X[k - 1] + RW
        out.append(arrow(pt(before) + (2 if k else 0), pt(ARROW_Y), pt(lx) - 2, pt(ARROW_Y),
                         INK, 2.2))
        out.append(arrow(pt(lx + LW) + 2, pt(ARROW_Y), pt(rx) - 2, pt(ARROW_Y), INK, 2.2))
        cx = pt(rx + RW / 2)
        drop = 168 if k == 2 else CLOCK_Y
        out.append(line(cx, pt(TOP + BH), cx, pt(drop), INK, 1.6))
    out.append(line(pt(REG_X[0] + RW / 2), pt(CLOCK_Y), pt(REG_X[2] + RW / 2), pt(CLOCK_Y),
                    INK, 1.6))
    out.append(text(pt(498), pt(186), "时钟", 18, INK))
    out.append(text(pt(512), pt(84), "延迟 = 360 ps", 18, INK, anchor="start"))
    out.append(text(pt(512), pt(110), "吞吐率 = 8.33 GOPS", 18, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("pipe-3way", pt(W), pt(H), build(), left=centring(INK_X, pt(W)))
