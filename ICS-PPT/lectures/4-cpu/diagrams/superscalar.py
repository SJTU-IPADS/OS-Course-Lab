#!/usr/bin/env python3
"""Two instructions start every cycle: the loop body on a two-wide pipeline.

The same five rows and the same nine-column grid as pipeline.svg
(timegrid.py), drawn ideally as that figure is: every operand ready at
decode. Two frames. The first is the one-instruction-per-cycle schedule of
pipeline.svg again, with the cycle-3 column framed: one instruction in
execute, one execution unit; five instructions take nine cycles, one
write-back per cycle from cycle 5. The second starts two instructions per
cycle: rows 1-2 in cycle 1, rows 3-4 in cycle 2, row 5 in cycle 3; in cycle 3
two instructions are in execute at once, so the execute stage holds two
execution units; five instructions take seven cycles, two write-backs per
cycle from cycle 5. The rows of the first pair do not move between the
frames, the others shift left. Columns 8-9 stay on the second frame so the
two schedules are compared on one grid.
Run it to refresh ../assets/superscalar-1.svg and superscalar-2.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import GREEN, INK, ORANGE, brace, save, text
from timegrid import CH, CW, LX, TOP, VIEW_LEFT, col_frame, cycle_numbers, row

W, H = 1120, 400
ROWS = ["movslq", "movl", "imull", "addl %ecx,%r9d", "addl $1,%eax"]
STAGES = ["取指", "译码", "执行", "访存", "写回"]
PITCH = CH + 6                      # as pipeline.svg
BOTTOM = TOP + 4 * PITCH + CH
FRAMES = {                          # width -> (top line, bracket label)
    1: ("周期 3：一条指令在执行，一个执行部件",
        "从周期 5 起，每个周期写回 1 条：5 条指令用 9 个周期"),
    2: ("周期 3：两条指令同时在执行，两个执行部件",
        "从周期 5 起，每个周期写回 2 条：5 条指令用 7 个周期"),
}


def build(width):
    top, label = FRAMES[width]
    last = 4 // width + 4                  # column of the last write-back
    out = [text(LX + 2.5 * CW, 22, top, 16, ORANGE, "bold")]
    out += cycle_numbers(9, emph=3)
    for r, name in enumerate(ROWS):
        out += row(name, TOP + r * PITCH, r // width, STAGES)
    out.append(col_frame(2, TOP - 5, BOTTOM + 5, ORANGE))
    out.append(text(LX + 5 * CW + 14, TOP + CH / 2 + 6,
                    "一条指令从取指到写回仍要 5 个周期", 16, INK, anchor="start"))
    out += brace(LX + 4 * CW + 3, LX + (last + 1) * CW - 3, BOTTOM + 12, GREEN, label, 16,
                 depth=8)
    return out


if __name__ == "__main__":
    for width in (1, 2):
        save(f"superscalar-{width}", W, H, build(width), left=VIEW_LEFT)
