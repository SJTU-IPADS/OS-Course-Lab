#!/usr/bin/env python3
"""The five stages as five groups of hardware, and how long each one works.

Same time axis as single_cycle.py (movslq in cycle 1, movl in cycle 2, the
same stage widths), turned around: rows are the hardware a stage uses, a
block marks the time that hardware works for the named instruction, blank
row space is time it idles. An orange frame marks the slice where movl reads
memory and every other row is blank.
Run it to refresh ../assets/stage-hw.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from single_cycle import COST, LX, ROWS, T, U
from svgkit import INK, LINE, MUTED, ORANGE, WHITE, line, mono, rect, save, text
from timegrid import TONE

W, H = 1120, 304
NUM_Y, TOP = 20, 34
RH, PITCH = 40, 46            # rows are lower than single_cycle's: five of them
HW = [                        # (stage, the hardware it uses)
    ("取指", "指令内存"),
    ("译码", "寄存器堆（读）"),
    ("执行", "ALU"),
    ("访存", "数据内存"),
    ("写回", "寄存器堆（写）"),
]


def build():
    out = [text(LX - 12, NUM_Y, "时钟周期", 15, MUTED, anchor="end")]
    for c in range(2):
        out.append(text(LX + c * T + T / 2, NUM_Y, str(c + 1), 15, MUTED))
    bottom = TOP + (len(HW) - 1) * PITCH + RH
    for c in range(3):                               # clock edges
        out.append(line(LX + c * T, TOP - 6, LX + c * T, bottom + 4, LINE, 1.4, "6 4"))
    row_y = {}
    for r, (stage, hw) in enumerate(HW):
        y = TOP + r * PITCH
        row_y[stage] = y
        tone = TONE[stage][1]
        out.append(text(24, y + RH / 2 + 6, stage, 16, tone, "bold", anchor="start"))
        out.append(text(70, y + RH / 2 + 6, hw, 15, INK, anchor="start"))
        out.append(rect(LX, y, 2 * T, RH, WHITE, LINE, rx=3, width=1.2, dash="4 3"))
    for c, (insn, stages) in enumerate(ROWS):        # instruction c runs in cycle c + 1
        x = LX + c * T
        for stage in stages:
            w = COST[stage] * U
            fill, stroke, tone = TONE[stage]
            y = row_y[stage]
            out.append(rect(x + 2, y + 3, w - 4, RH - 6, fill, stroke, rx=3, width=1.4))
            out.append(mono(x + w / 2, y + RH / 2 + 5, insn, 13, tone, "bold", anchor="middle"))
            x += w
    x0 = LX + T + 5 * U                              # movl's memory step
    out.append(rect(x0 - 2, TOP - 4, 2 * U + 4, bottom - TOP + 8, "none", ORANGE, rx=6,
                    width=2, dash="6 4"))
    out.append(text(x0 + U, bottom + 26, "movl 访存期间", 16, ORANGE, "bold"))
    return out


if __name__ == "__main__":
    save("stage-hw", W, H, build())
