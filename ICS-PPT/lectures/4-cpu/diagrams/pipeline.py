#!/usr/bin/env python3
"""The five loop-body instructions overlapping, one new instruction per cycle.

Rows are the gcc -Og loop body, in program order. Columns are cycles.
A cell holds the stage that instruction is in. At cycle 5 every stage is busy,
and a new instruction still enters each cycle. Run it to refresh
../assets/pipeline.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, MUTED, ORANGE, mono, rect, save, text)

W, H = 1120, 300
ROWS = ["movslq", "movl", "imull", "addl sum", "addl $1"]
STAGE = {
    "取": (FILL_BLUE, BLUE),
    "译": (FILL_GREY, INK),
    "执": (FILL_ORANGE, ORANGE),
    "访": (FILL_GREEN, GREEN),
    "写": (FILL_BLUE, BLUE),
}
CW, CH = 78, 36
LX, TOP = 168, 56


def build():
    cycles = 9
    out = [text(LX + cycles * CW / 2, 28, "周期", 15, MUTED)]
    for c in range(cycles):
        out.append(text(LX + c * CW + CW / 2, TOP - 8, str(c + 1), 14, MUTED))
    for r, name in enumerate(ROWS):
        y = TOP + r * CH
        out.append(mono(24, y + 24, name, 15, INK))
        for c in range(cycles):
            k = c - r
            if not 0 <= k < 5:
                continue
            fill, stroke = STAGE["取译执访写"[k]]
            x = LX + c * CW
            out.append(rect(x + 3, y + 3, CW - 6, CH - 6, fill, stroke, rx=3, width=1.4))
            out.append(text(x + CW / 2, y + 25, "取译执访写"[k], 16, stroke, "bold"))
    y = TOP + 5 * CH + 28
    out.append(text(W / 2, y, "周期 5：五级同时在工作，新的指令仍从取指进入", 16, INK))
    return out


if __name__ == "__main__":
    save("pipeline", W, H, build())
