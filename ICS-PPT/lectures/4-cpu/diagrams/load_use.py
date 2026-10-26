#!/usr/bin/env python3
"""Load-use with forwarding and no stall: the value arrives after it is needed.

Two rows on the shared time-space grid (timegrid.py), spaced like stall.py so
the cells stay put when the next page (stall.svg) replaces this figure: movl
in cycles 1-5, imull right behind it in cycles 2-6 with no stall cell. movl
computes the address of w[i] in execute (cycle 3) and has w[i] only at the end
of its memory stage (cycle 4); imull would execute in that same cycle 4. The
red column frames cycle 4, and a red dashed arrow runs from the end of movl's
memory cell back to the start of imull's execute cell: the forward would go
backwards in time, so it is crossed out.

Run it to refresh ../assets/load-use.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import GREEN, ORANGE, RED, arrow, line, save, text
from timegrid import CH, CW, LX, TOP, VIEW_LEFT, col_frame, cx, cycle_numbers, row
from stall import GRID_H as H, MID, R2, W

STAGES = ["取指", "译码", "执行", "访存", "写回"]


def build():
    out = [text(cx(3), 22, "周期 4：w[i] 还在读", 16, RED, "bold")]
    out += cycle_numbers(7, emph=4, color=RED)
    out += row("movl", TOP, 0, STAGES)
    out += row("imull", R2, 1, STAGES)
    out.append(col_frame(3, TOP - 5, R2 + CH + 5, RED))
    out.append(text(cx(2), MID + 5, "算出地址", 15, ORANGE, "bold"))
    out.append(text(LX + 5 * CW + 12, TOP + CH / 2 + 6, "周期 4 结束时读出 w[i]", 15, GREEN,
                    "bold", anchor="start"))
    x0, x1 = LX + 4 * CW - 12, LX + 3 * CW + 14
    out.append(arrow(x0, TOP + CH + 2, x1, R2 - 2, RED, 2.2, dash="6 4"))
    mx, my, d = (x0 + x1) / 2, (TOP + CH + R2) / 2, 9
    out.append(line(mx - d, my - d, mx + d, my + d, RED, 3))
    out.append(line(mx - d, my + d, mx + d, my - d, RED, 3))
    out.append(text(LX + 4 * CW + 8, MID + 5, "转发来不及", 16, RED, "bold", anchor="start"))
    out.append(text(cx(3), R2 + CH + 26, "imull 在周期 4 开始执行，需要 w[i]", 15, RED))
    return out


if __name__ == "__main__":
    save("load-use", W, H, build(), left=VIEW_LEFT)
