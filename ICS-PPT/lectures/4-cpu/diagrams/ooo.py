#!/usr/bin/env python3
"""Out-of-order execution: a later independent instruction runs while imull waits.

Five rows of the shared time-space grid (timegrid.py), the loop body from the
movl that reads w[i] on: movl, imull, addl %ecx,%r9d, addl $1,%eax, cmpl. The
grid starts at cycle 3 (movl's execute), as branch-miss starts at cycle 3, and
its ten columns reach cycle 12, the last write-back of ooo_inorder.py, which
draws the in-order run on this same canvas. The memory read of w[i] is
assumed to take three cycles (a wide green cell over cycles 4-6): with the
one-cycle read of the stall figure, the next independent instruction is not
decoded yet when the stall happens, and there is nothing to move forward.

Rules drawn: a decoded instruction waits inside the processor and executes
when its operands are ready and the one execution unit is free, the oldest
first (imull in cycle 7, after the green elbow from the end of the read; addl
%ecx,%r9d in cycle 8, after the orange elbow from imull's execute); the
execution unit is free in cycle 6 and addl $1,%eax, decoded in cycle 5 with
%eax ready, executes there, ahead of the two older instructions; cmpl needs
the new %eax, ready from cycle 7, but cycles 7 and 8 go to the older imull and
addl, so it executes in cycle 9. Write-back stays in program order: an
instruction writes back no earlier than the one before it, in the same cycle
at the earliest, so addl $1,%eax waits (the dashed "等待写回" cell over cycles
8-9) and writes back in cycle 10 together with addl %ecx,%r9d, and cmpl
writes back in cycle 11, one cycle before the in-order run. The new %eax
reaches cmpl's execute in cycle 9 by a second orange elbow, from the
"等待写回" cell in cycle 8: the result of addl $1,%eax stays inside the
processor until it is written back. The red "停" cells are decoded
instructions waiting to execute.

An animation, four frames: cycles 3-5 (w[i] still being read), cycle 6 (the
orange column: addl $1,%eax executes first), cycle 7 (w[i] arrives, imull
executes), cycles 8-12. The slide's conclusion is its highlight line; the
figure only annotates the grid. The rows are closer below addl %ecx,%r9d,
where no elbow runs, and the other three gaps, which hold elbows here or in
ooo_inorder.py, are 14 px: the five rows fit the slide with the code above
them, and the highlight under the out-of-order grid keeps clear of it.
Run it to refresh ../assets/ooo-1.svg .. ooo-4.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import GREEN, INK, MUTED, ORANGE, RED, WHITE, elbow, rect, save, text
from timegrid import CH, CW, LX, NUM_Y, TONE, TOP, col_frame, cx, row

W, H = 1160, 380
LEFT = 12                           # centres the ink (x 24..1160) on the canvas
FIRST = 3                           # the cycle in column 0
COLS = 10                           # cycles 3..12
GAPS = [14, 14, 6, 14]              # room under each row; all but the third hold elbows
R = [TOP]                           # tops of the five rows
for g in GAPS:
    R.append(R[-1] + CH + g)
ROWS = ["movl", "imull", "addl %ecx,%r9d", "addl $1,%eax", "cmpl %edx,%eax"]
FRAMES = [6, 7, 8, 13]              # cycles shown in each frame: up to (exclusive)


def col(cycle):
    return cycle - FIRST


def numbers(n=COLS, emph=None, color=ORANGE):
    """'周期' and the numbers FIRST..FIRST+n-1, as timegrid.cycle_numbers."""
    out = [text(LX - 12, NUM_Y, "周期", 15, MUTED, anchor="end")]
    for c in range(n):
        x = LX + c * CW + CW / 2
        k = FIRST + c
        if emph == k:
            out.append(text(x, NUM_Y + 1, str(k), 18, color, "bold"))
        else:
            out.append(text(x, NUM_Y, str(k), 15, MUTED))
    return out


def wide(first_cycle, span, y, name, upto, fill, stroke, tone, dash=None):
    """One cell over `span` cycles, clipped to the cycles before `upto`."""
    shown = min(span, upto - first_cycle)
    if shown <= 0:
        return []
    x = LX + col(first_cycle) * CW
    w = shown * CW
    return [rect(x + 3, y + 3, w - 6, CH - 6, fill, stroke, rx=3, width=1.4, dash=dash),
            text(x + w / 2, y + CH / 2 + 6, name, 17, tone, "bold")]


def stages(label, y, first_cycle, names, upto):
    return row(label, y, col(first_cycle), names, upto=col(upto))


def handoff(k, cycle, color):
    """The elbow from row k's cell in `cycle` into row k+1's cell one cycle later."""
    mid = R[k] + CH + GAPS[k] / 2
    return elbow([(cx(col(cycle)), R[k] + CH - 1), (cx(col(cycle)), mid),
                  (cx(col(cycle + 1)), mid), (cx(col(cycle + 1)), R[k + 1] + 1)], color, 2.4)


def build(upto):
    out = []
    if upto <= 6:
        out.append(text(cx(col(4)) + CW / 2, 22, "周期 4、5：w[i] 还在读，imull 等它", 16, RED,
                        "bold"))
        out += numbers(emph=None)
    else:
        out.append(text(cx(col(6)), 22, "周期 6：imull 仍在等 w[i]，执行部件空闲，addl $1,%eax 先执行",
                        16, ORANGE, "bold"))
        out += numbers(emph=6)
    # movl: execute in cycle 3, a three-cycle read, write-back in cycle 7
    out += stages(ROWS[0], R[0], 3, ["执行"], upto)
    out += wide(4, 3, R[0], "访存", upto, *TONE["访存"])
    out += stages(ROWS[0], R[0], 7, ["写回"], upto)[1:]
    # imull: decoded in cycle 3, waits for w[i] through cycle 6
    out += stages(ROWS[1], R[1], 3, ["译码", "停", "停", "停", "执行", "访存", "写回"], upto)
    # addl %ecx,%r9d: waits for the product through cycle 7
    out += stages(ROWS[2], R[2], 3, ["取指", "译码", "停", "停", "停", "执行", "访存", "写回"], upto)
    # addl $1,%eax: executes in cycle 6, writes back with addl %ecx,%r9d in cycle 10
    out += stages(ROWS[3], R[3], 4, ["取指", "译码", "执行", "访存"], upto)
    out += wide(8, 2, R[3], "等待写回", upto, WHITE, INK, INK, dash="5 4")
    out += stages(ROWS[3], R[3], 10, ["写回"], upto)[1:]
    # cmpl: %eax ready from cycle 7, the execution unit from cycle 9
    out += stages(ROWS[4], R[4], 5, ["取指", "译码", "停", "停", "执行", "访存", "写回"], upto)
    if upto > 6:
        out.append(col_frame(col(6), TOP - 5, R[4] + CH + 5, ORANGE))
    if upto > 7:                     # w[i] read: into imull's execute
        out.append(handoff(0, 6, GREEN))
    if upto > 8:                     # the product: into addl's execute
        out.append(handoff(1, 7, ORANGE))
    if upto > 9:                     # the new %eax, held until write-back: into cmpl's execute
        out.append(handoff(3, 8, ORANGE))
    return out


if __name__ == "__main__":
    for k, upto in enumerate(FRAMES, 1):
        save(f"ooo-{k}", W, H, build(upto), left=LEFT)
