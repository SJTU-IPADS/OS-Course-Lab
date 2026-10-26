#!/usr/bin/env python3
"""The five stages as five groups of hardware, and how long each one works.

One movl instruction in one clock cycle, with the stage widths from
single_cycle.py: rows are the hardware a stage uses, a block marks the time
that hardware works for movl, blank row space is time it idles.
Run it to refresh ../assets/stage-hw.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from single_cycle import COST, LX, ROWS, T, U
from svgkit import INK, LINE, MUTED, WHITE, line, mono, rect, save, text
from timegrid import TONE

W, H = 1120, 244
LEFT = (24 + LX + T - W) / 2  # centre the one-cycle drawing in the slide canvas
TOP = 10
RH, PITCH = 40, 46            # rows are lower than single_cycle's: five of them
HW = [                        # (stage, the hardware it uses)
    ("取指", "指令内存"),
    ("译码", "寄存器堆（读）"),
    ("执行", "ALU"),
    ("访存", "数据内存"),
    ("写回", "寄存器堆（写）"),
]


def build():
    out = []
    bottom = TOP + (len(HW) - 1) * PITCH + RH
    row_y = {}
    for r, (stage, hw) in enumerate(HW):
        y = TOP + r * PITCH
        row_y[stage] = y
        tone = TONE[stage][1]
        out.append(text(24, y + RH / 2 + 6, stage, 16, tone, "bold", anchor="start"))
        out.append(text(70, y + RH / 2 + 6, hw, 15, INK, anchor="start"))
        out.append(rect(LX, y, T, RH, WHITE, LINE, rx=3, width=1.2, dash="4 3"))
    for edge in (LX, LX + T):                        # the cycle's clock edges
        out.append(line(edge, TOP - 6, edge, bottom + 4, MUTED, 1.6, "6 4"))
    insn, stages = next(row for row in ROWS if row[0] == "movl")
    x = LX
    for stage in stages:
        w = COST[stage] * U
        fill, stroke, tone = TONE[stage]
        y = row_y[stage]
        out.append(rect(x + 2, y + 3, w - 4, RH - 6, fill, stroke, rx=3, width=1.4))
        out.append(mono(x + w / 2, y + RH / 2 + 5, insn, 13, tone, "bold", anchor="middle"))
        x += w
    return out


if __name__ == "__main__":
    save("stage-hw", W, H, build(), left=LEFT)
