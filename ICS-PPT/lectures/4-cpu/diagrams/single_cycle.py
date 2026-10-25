#!/usr/bin/env python3
"""Why the single-cycle clock is as long as the slowest instruction.

Two consecutive loop-body instructions on one time axis, one clock cycle each.
Every stage is a box whose width is the time it takes: fetch, execute and
memory 2 units, decode and write-back 1 (the register file is quicker than the
other three steps). movslq has no memory step, writes back early and idles
for the rest of cycle 1; movl cannot fetch before the clock edge, uses all
five stages and fills cycle 2. Below, the same movl in a cycle only as long as
movslq's work: the red edge runs up to where movslq's work ends and falls
inside movl's memory step.
The widths illustrate, they are not measured. Stage colours and row labels
follow timegrid.py.
Run it to refresh ../assets/single-cycle.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, LINE, MUTED, ORANGE, RED, WHITE, brace, line, mono, rect, save, text
from timegrid import CH, TONE

W, H = 1120, 288
LX = 200                     # time axis starts here; row labels live in [24, LX)
U = 56                       # one time unit
T = 8 * U                    # one clock cycle: movl's five stages
NUM_Y, TOP = 20, 34
PITCH = CH + 6
COST = {"取指": 2, "译码": 1, "执行": 2, "访存": 2, "写回": 1}
ROWS = [                     # (label, stages); row k runs in cycle k + 1
    ("movslq", ["取指", "译码", "执行", "写回"]),
    ("movl", ["取指", "译码", "执行", "访存", "写回"]),
]
SHORT = 6 * U                # movslq's work: a cycle that leaves movl unfinished
SHORT_TOP = 226


def stage_boxes(x, y, stages, after=None):
    """Stage boxes from x; work at or after `after` is not reached (dashed)."""
    out = []
    for name in stages:
        w = COST[name] * U
        if after is not None and x < after < x + w:    # cut by the clock edge
            fill, stroke, tone = TONE[name]
            out.append(rect(after, y + 3, x + w - 3 - after, CH - 6, WHITE, MUTED, rx=3,
                            width=1.4, dash="5 4"))
            out.append(rect(x + 3, y + 3, after - x - 3, CH - 6, fill, stroke, rx=3,
                            width=1.4))
            out.append(text((x + 3 + after) / 2, y + CH / 2 + 6, name, 17, tone, "bold"))
        elif after is not None and x >= after:
            out.append(rect(x + 3, y + 3, w - 6, CH - 6, WHITE, MUTED, rx=3, width=1.4,
                            dash="5 4"))
            out.append(text(x + w / 2, y + CH / 2 + 6, name, 17, MUTED, "bold"))
        else:
            fill, stroke, tone = TONE[name]
            out.append(rect(x + 3, y + 3, w - 6, CH - 6, fill, stroke, rx=3, width=1.4))
            out.append(text(x + w / 2, y + CH / 2 + 6, name, 17, tone, "bold"))
        x += w
    return out, x


def too_short():
    """movl in a cycle as long as movslq's work: the edge cuts its memory step."""
    y = SHORT_TOP
    out = [text(LX, y - 14, "时钟周期若只等于 movslq 的用时：", 16, INK, anchor="start"),
           mono(24, y + CH / 2 + 6, "movl", 16, INK),
           rect(LX, y, SHORT, CH, WHITE, LINE, rx=3, width=1.4, dash="4 3")]
    boxes, end = stage_boxes(LX, y, ROWS[1][1], after=LX + SHORT)
    out += boxes
    out.append(line(LX + SHORT, TOP - 6, LX + SHORT, y + CH + 8, RED, 2, "6 4"))  # up to movslq's end
    out.append(text(end + 16, y + CH / 2 + 6, "周期结束时，访存还没有做完", 16, RED, "bold",
                    anchor="start"))
    return out


def build():
    out = [text(LX - 12, NUM_Y, "时钟周期", 15, MUTED, anchor="end")]
    for c in range(2):
        out.append(text(LX + c * T + T / 2, NUM_Y, str(c + 1), 15, MUTED))
    bottom = TOP + (len(ROWS) - 1) * PITCH + CH
    for c in range(3):                               # clock edges
        out.append(line(LX + c * T, TOP - 6, LX + c * T, bottom + 4, LINE, 1.4, "6 4"))
    for r, (label, stages) in enumerate(ROWS):
        y, x0 = TOP + r * PITCH, LX + r * T
        out.append(mono(24, y + CH / 2 + 6, label, 16, INK))
        out.append(rect(x0, y, T, CH, WHITE, LINE, rx=3, width=1.4, dash="4 3"))
        boxes, x = stage_boxes(x0, y, stages)
        out += boxes
        if x < x0 + T:
            out.append(text((x + x0 + T) / 2, y + CH / 2 + 6, "空等", 16, INK))
    out += brace(LX + T, LX + 2 * T, bottom + 12, ORANGE,
                 "一个时钟周期 = movl 走完五个阶段的时间", 16, depth=8)
    return out + too_short()


if __name__ == "__main__":
    save("single-cycle", W, H, build())
