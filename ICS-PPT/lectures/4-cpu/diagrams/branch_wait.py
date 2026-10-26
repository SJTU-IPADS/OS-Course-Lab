#!/usr/bin/env python3
"""After jl, the fetch stage does not know which instruction comes next.

The control hazard page, before any guessing. A time-space grid (timegrid.py)
with jl and one row for the instruction after it; under the grid, the five
stages in the cycle shown, drawn with stall_bubble.py's cycle_snapshot as on
the data hazard pages. jl is fetched in cycle 1 and decides whether to jump at
the end of its execute (cycle 3, the dashed orange line). In cycles 2 and 3 the
fetch stage would fetch the next instruction, but whether that is movl
%r9d,%eax (no jump) or .L3's movslq %eax,%r8 (jump) is not known yet: dashed
"?" cells on the grid, a "?" fetch stage in the snapshot.

  frame 1  cycle 1: jl is fetched.
  frame 2  cycle 2: jl decodes; the fetch stage does not know what to fetch.
  frame 3  cycle 3: jl executes; still unknown, and whatever the fetch stage
           produced in cycle 2 sits in decode as "?".
  frame 4  the simplest fix, waiting: the next instruction stalls in cycles 2
           and 3 (red "停" cells) and is fetched in cycle 4. The snapshot is
           cycle 3 again, now with PC stalled and a bubble in decode where
           frame 3 had the "?".

Nothing is drawn above the grid, so it sits UP px higher, as in no_forward.py.
Run it to refresh ../assets/branch-wait-1.svg .. branch-wait-4.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import FONT, INK, MONO, ORANGE, RED, label_line, line, save, text
from stall_bubble import ADDL, NEXT, cycle_snapshot
from timegrid import CH, CW, LX, NUM_Y, TOP, VIEW_LEFT, cycle_numbers, row

W, H = 1120, 274
UP = 30                             # no caption above the grid
T0 = TOP - UP                       # top of the jl row (34)
GAP = 12                            # as branch.py
R2 = T0 + CH + GAP                  # top of the next instruction's row (98)
XD = LX + 3 * CW                    # end of cycle 3: jl has just executed
SNAP = 190                          # box top of the snapshot under the grid
JL, CMPL = "jl .L3", "cmpl %edx,%eax"
STAGES = ["取指", "译码", "执行", "访存", "写回"]
DECIDE = ("周期末才知道跳不跳", ORANGE)
FRAMES = [                          # (cycles on the grid, next row, snapshot)
    (1, [], (1, [JL, CMPL, NEXT, ADDL, "imull"], {}, {})),
    (2, ["?"], (2, ["?", JL, CMPL, NEXT, ADDL], {}, {0: ("取哪一条？", RED)})),
    (3, ["?", "?"], (3, ["?", "?", JL, CMPL, NEXT], {}, {2: DECIDE})),
    (8, ["停", "停"] + STAGES,
     (3, ["?", None, JL, CMPL, NEXT], {0: ("stall", INK), 1: ("bubble", INK)},
      {0: ("等 jl 判定", INK), 2: DECIDE})),
]


def build(upto, nxt, snap):
    cycle = snap[0]
    out = cycle_numbers(8, emph=cycle, color=RED if cycle > 1 else INK, y=NUM_Y - UP)
    out += row("jl .L3", T0, 0, STAGES, upto)
    out += row("jl 的下一条", R2, 1, nxt, upto)
    if upto in (2, 3):
        out += label_line(LX + upto * CW + 12, R2 + CH / 2 + 6, [
            ("取 ", FONT, RED, "bold"), ("movl %r9d,%eax", MONO, INK, "normal"),
            ("（不跳）还是 ", FONT, RED, "bold"), ("movslq %eax,%r8", MONO, INK, "normal"),
            ("（跳）？", FONT, RED, "bold")], 16)
    if upto >= 3:
        out.append(line(XD, T0 - 8, XD, R2 + CH + 8, ORANGE, 2, dash="6 4"))
        out.append(text(LX + 5 * CW + 12, T0 + CH / 2 + 6, "周期 3 结束：jl 执行完，才知道跳不跳",
                        16, ORANGE, "bold", anchor="start"))
    return out + cycle_snapshot(SNAP, *snap)


if __name__ == "__main__":
    for k, (upto, nxt, snap) in enumerate(FRAMES, 1):
        save(f"branch-wait-{k}", W, H, build(upto, nxt, snap), left=VIEW_LEFT)
