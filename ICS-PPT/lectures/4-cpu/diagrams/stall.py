#!/usr/bin/env python3
"""Load-use: imull needs w[i] while movl is still reading it from memory.

Two rows on the shared time-space grid (timegrid.py), the same skeleton as
forward.svg. movl reads w[i] into %r10d in its memory stage (cycle 4); the
register imull that follows (the multiply half of imull (%rdi,%r8,4),%ecx) would
execute in that same cycle, but the value is nowhere yet, so imull stalls in
decode for one cycle while execute gets a bubble (the red "停" cell) and
executes in cycle 5. The green elbow runs from
movl's memory cell down into imull's execute cell, one column to the right,
in the same shape as the orange forwarding elbow of forward.svg: side by side,
the two figures differ only in which stage the value comes out of.
The slide's conclusion lives in its text, not in the figure.

Under the grid, the five stages in the cycle the frame has reached, drawn with
stall_bubble.py's cycle_snapshot as on the stall-wait page (no_forward.py):
the edge that ends cycle 3 stalls PC and the fetch/decode register and loads a
bubble into decode/execute, so in cycle 4 imull is still in decode and execute
is empty; the bubble then moves on through memory and write-back. The cycle
numbers emphasise the snapshot's cycle, red in the "停" column.

An animation, one frame per cycle from cycle 2 on: frame k shows the cells of
cycles 1..k+1 and the snapshot of cycle k+1. Cycle 4 brings the red column and
the "停" cell; cycle 5 brings the elbow; the last frame is the whole figure.
Run it to refresh ../assets/stall-1.svg .. stall-6.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import GREEN, INK, RED, elbow, save, text
from stall_bubble import ADDL, HOLD, NEXT, cycle_snapshot
from timegrid import CH, CW, LX, TOP, VIEW_LEFT, col_frame, cx, cycle_numbers, row

W = 1120
GRID_H = 280                        # the grid alone: load_use.py's canvas
H = 374
GAP = 64                            # room between the rows for the elbow and its label
R2 = TOP + CH + GAP                 # top of the second row (= 180)
MID = TOP + CH + GAP / 2            # the elbow's horizontal run (= 148)
SNAP = 290                          # box top of the snapshot under the grid

UPTO = [2, 3, 4, 5, 6, 7]            # cycles shown in each frame
CMPL, JL = "cmpl %edx,%eax", "jl .L3"
STATE = {                           # cycle -> (instruction per stage, bar marks, notes)
    2: (["imull", "movl", "…", "…", "…"], {}, {}),
    3: ([ADDL, "imull", "movl", "…", "…"], {},
        {1: ("要用 w[i]", RED), 2: ("算出 w[i] 的地址", INK)}),
    4: ([ADDL, "imull", None, "movl", "…"], HOLD,
        {1: ("仍在等 w[i]", RED), 3: ("周期末读出 w[i]", GREEN)}),
    5: ([NEXT, ADDL, "imull", None, "movl"], {}, {2: ("用转发来的 w[i]", GREEN)}),
    6: ([CMPL, NEXT, ADDL, "imull", None], {}, {}),
    7: ([JL, CMPL, NEXT, ADDL, "imull"], {}, {}),
}


def build(upto):
    out = []
    if upto >= 4:
        out.append(text(cx(3), 22, "周期 4：w[i] 还在读", 16, RED, "bold"))
    out += cycle_numbers(7, emph=upto, color=RED if upto == 4 else INK)
    out += row("movl", TOP, 0, ["取指", "译码", "执行", "访存", "写回"], upto)
    out += row("imull", R2, 1, ["取指", "译码", "停", "执行", "访存", "写回"], upto)
    if upto >= 4:
        out.append(col_frame(3, TOP - 5, R2 + CH + 5, RED))
    if upto >= 5:
        out.append(elbow([(cx(3), TOP + CH - 1), (cx(3), MID), (cx(4), MID), (cx(4), R2 + 1)],
                         GREEN, 2.4))
        out.append(text(LX + 5 * CW + 8, MID + 5, "访存结束，读出的 w[i] 才能送进执行", 16,
                        GREEN, "bold", anchor="start"))
    return out + cycle_snapshot(SNAP, upto, *STATE[upto])


if __name__ == "__main__":
    for k, upto in enumerate(UPTO, 1):
        save(f"stall-{k}", W, H, build(upto), left=VIEW_LEFT)
