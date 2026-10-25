#!/usr/bin/env python3
"""Load-use: imull needs w[i] while movl is still reading it from memory.

Two rows on the shared time-space grid (timegrid.py), the same skeleton as
forward.svg with the rows closer (the page has a paragraph above the figure). movl reads w[i] into %r10d in its memory stage (cycle 4); the
register imull that follows (the multiply half of imull (%rdi,%r8,4),%ecx) would
execute in that same cycle, but the value is nowhere yet, so imull waits one
cycle (the red "停" cell) and executes in cycle 5. The green elbow runs from
movl's memory cell down into imull's execute cell, one column to the right,
in the same shape as the orange forwarding elbow of forward.svg: side by side,
the two figures differ only in which stage the value comes out of.
The slide's conclusion lives in its text, not in the figure.

An animation, one frame per cycle from cycle 2 on: frame k shows the cells of
cycles 1..k+1. Cycle 4 brings the red column, the "停" cell and its notes;
cycle 5 brings the elbow; the last frame is the whole figure.
Run it to refresh ../assets/stall-1.svg .. stall-6.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import GREEN, RED, elbow, save, text
from timegrid import CH, CW, LX, TOP, VIEW_LEFT, col_frame, cx, cycle_numbers, row

W, H = 1120, 280
GAP = 64                            # room between the rows for the elbow and its label
R2 = TOP + CH + GAP                 # top of the second row (= 180)
MID = TOP + CH + GAP / 2            # the elbow's horizontal run (= 148)


UPTO = [2, 3, 4, 5, 6, 7]            # cycles shown in each frame


def build(upto):
    out = []
    if upto >= 4:
        out.append(text(cx(3), 22, "周期 4：w[i] 还在读", 16, RED, "bold"))
    out += cycle_numbers(7, emph=4 if upto >= 4 else None, color=RED)
    out += row("movl", TOP, 0, ["取指", "译码", "执行", "访存", "写回"], upto)
    out += row("imull", R2, 1, ["取指", "译码", "停", "执行", "访存", "写回"], upto)
    if upto < 4:
        return out
    out.append(col_frame(3, TOP - 5, R2 + CH + 5, RED))
    out.append(text(cx(3), R2 + CH + 26, "本该在这个周期执行，w[i] 还没读出来", 15, RED))
    if upto < 5:
        return out
    out.append(elbow([(cx(3), TOP + CH - 1), (cx(3), MID), (cx(4), MID), (cx(4), R2 + 1)],
                     GREEN, 2.4))
    out.append(text(LX + 5 * CW + 8, MID + 5, "访存结束，读出的 w[i] 才能送进执行", 16, GREEN,
                    "bold", anchor="start"))
    return out


if __name__ == "__main__":
    for k, upto in enumerate(UPTO, 1):
        save(f"stall-{k}", W, H, build(upto), left=VIEW_LEFT)
