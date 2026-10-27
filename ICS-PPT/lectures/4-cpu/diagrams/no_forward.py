#!/usr/bin/env python3
"""Stall until the write-back: addl decodes only after imull has written %ecx.

Two rows of the shared time-space grid (timegrid.py). imull runs its five
stages in cycles 1-5 and writes %ecx at the clock edge that ends cycle 5. addl
reads %ecx in decode, so it stalls in cycles 3-5 (red "停" cells) and decodes
in cycle 6; execute, memory and write-back follow in cycles 7-9. A dark elbow
runs from the write-back cell to the decode cell, coloured like the stage it
leaves.

Under the grid, the five stages in the cycle the frame has reached, drawn with
stall_bubble.py's snapshot (same boxes, register bars and marks): which
instruction each stage holds, which pipeline registers stalled at the clock
edge that began the cycle, and where the bubble went in. addl sits in decode
from cycle 3 to 6; the edges that end cycles 3, 4 and 5 stall PC and the
fetch/decode register and load a bubble into decode/execute, so the cycle-6
snapshot holds the three bubbles in execute, memory and write-back. The
cycle numbers emphasise the snapshot's cycle, red while addl reads the old
%ecx. Nothing is drawn above the grid, so it sits UP px higher than in the
other animations.

An animation: frames 1-5 show cycles 1..2 up to 1..6, one cycle per frame,
with the snapshot of the last cycle shown; the last frame adds cycles 7-9,
the bracket under the three stall cells and the snapshot of cycle 7.
Run it to refresh ../assets/no-forward-1.svg .. no-forward-6.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, RED, brace, elbow, save, text
from stall_bubble import ADDL, HOLD, NEXT, cycle_snapshot
from timegrid import CH, CW, LX, NUM_Y, TOP, VIEW_LEFT, cx, cycle_numbers, row

W, H = 1120, 346
UP = 30                             # no caption above the grid
T0 = TOP - UP                       # top of the imull row (34)
GAP = 56                            # room between the rows for the elbow and its label
R2 = T0 + CH + GAP                  # top of the second row (142)
MID = T0 + CH + GAP / 2             # the elbow's horizontal run (114)
SNAP = 262                          # box top of the snapshot under the grid
FIRST = ["取指", "译码", "执行", "访存", "写回"]
SECOND = ["取指", "停", "停", "停", "译码", "执行", "访存", "写回"]
UPTO = [2, 3, 4, 5, 6, 9]           # cycles shown in each frame
CMPL = "cmpl %edx,%eax"
OLD = {1: ("读到 %ecx 的旧值 4", RED)}
STATE = {                           # cycle -> (instruction per stage, bar marks, notes)
    2: ([ADDL, "imull", "…", "…", "…"], {}, {}),
    3: ([NEXT, ADDL, "imull", "…", "…"], {}, OLD),
    4: ([NEXT, ADDL, None, "imull", "…"], HOLD, OLD),
    5: ([NEXT, ADDL, None, None, "imull"], HOLD, OLD),
    6: ([NEXT, ADDL, None, None, None], HOLD, {1: ("读到 %ecx 的新值 12", INK)}),
    7: ([CMPL, NEXT, ADDL, None, None], {}, {2: ("执行 10 + 12 = 22", INK)}),
}


def build(upto):
    now = min(upto, 7)
    out = cycle_numbers(9, emph=now, color=RED if 3 <= now <= 5 else INK, y=NUM_Y - UP)
    out += row("imull", T0, 0, FIRST, upto)
    out += row("addl", R2, 1, SECOND, upto)
    if upto >= 5:
        out.append(text(LX + 5 * CW + 12, T0 + CH / 2 + 6, "周期 5 结束时乘积写入 %ecx", 15,
                        INK, anchor="start"))
    if upto >= 6:
        out.append(elbow([(cx(4), T0 + CH - 1), (cx(4), MID), (cx(5), MID), (cx(5), R2 + 1)],
                         INK, 2.4))
        out.append(text(LX + 6 * CW + 8, MID + 5, "周期 6 译码，读到写回后的 %ecx", 16, INK,
                        "bold", anchor="start"))
    if upto >= 9:
        out += brace(LX + 2 * CW + 4, LX + 5 * CW - 4, R2 + CH + 6, RED,
                     "stall 3 个周期，等待写回", 16)
    return out + cycle_snapshot(SNAP, now, *STATE[now])


if __name__ == "__main__":
    for k, upto in enumerate(UPTO, 1):
        save(f"no-forward-{k}", W, H, build(upto), left=VIEW_LEFT)
