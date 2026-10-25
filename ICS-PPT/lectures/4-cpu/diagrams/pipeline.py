#!/usr/bin/env python3
"""The five loop-body instructions overlapping, one new instruction per cycle.

Rows are the gcc -Og loop body, in program order. Columns are cycles.
A cell holds the stage that instruction is in. At cycle 5 every stage holds an
instruction; from then on one instruction writes back per cycle (throughput),
while each instruction itself still takes five cycles (latency). The grid comes
from timegrid.py so this figure lines up with forward / stall / branch.
Run it to refresh ../assets/pipeline.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import GREEN, INK, ORANGE, brace, save, text
from timegrid import CH, CW, LX, TOP, VIEW_LEFT, col_frame, cycle_numbers, row

W, H = 1120, 400
ROWS = ["movslq", "movl", "imull", "addl %ecx,%r9d", "addl $1,%eax"]
STAGES = ["取指", "译码", "执行", "访存", "写回"]
PITCH = CH + 6                      # rows packed tight: the overlap is the point


def build():
    out = [text(LX + 4.5 * CW, 22, "周期 5：五级同时有指令", 16, ORANGE, "bold")]
    out += cycle_numbers(9, emph=5)
    for r, name in enumerate(ROWS):
        out += row(name, TOP + r * PITCH, r, STAGES)
    bottom = TOP + 4 * PITCH + CH
    out.append(col_frame(4, TOP - 5, bottom + 5, ORANGE))
    out.append(text(LX + 5 * CW + 14, TOP + CH / 2 + 6,
                    "一条指令从取指到写回仍要 5 个周期（延迟）", 16, INK, anchor="start"))
    out += brace(LX + 4 * CW + 3, LX + 9 * CW - 3, bottom + 12, GREEN,
                 "从周期 5 起，每个周期写回一条（吞吐）", 16, depth=8)
    return out


if __name__ == "__main__":
    save("pipeline", W, H, build(), left=VIEW_LEFT)
