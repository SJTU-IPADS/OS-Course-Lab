#!/usr/bin/env python3
"""Branch divergence in one warp, drawn a step at a time.

Left: an if/else whose lines carry the address of the instruction (PC 100 to
105). Right: the warp at each PC it executes, one column per step on a time
axis; each row is one thread, filled when the thread executes the instruction
and dashed when it waits. Threads 4 to 29 are left out and stand behind the dots.
The even threads run line 102 while the odd ones wait, then the odd threads run
line 104 while the even ones wait; at 105 all of them run again.
Frame k shows the first k steps: ../assets/divergence-1.svg .. divergence-5.svg.
Run it to refresh them.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE,
                    WHITE, arrow, listing, mono, rect, save, text)

W, H = 1080, 330
CODE = [
    "100:  x = threadIdx.x;",
    "101:  if (x % 2 == 0) {",
    "102:      A();",
    "103:  } else {",
    "104:      B();",
    "105:  }",
]
# The threads drawn, one row each. None is the row of dots for threads 4 to 29.
ROWS = [0, 1, 2, 3, None, 30, 31]
# The steps of the warp: (pc, line index in CODE, which threads execute).
EVEN, ODD, ALL = (lambda t: t % 2 == 0), (lambda t: t % 2 == 1), (lambda t: True)
STEPS = [(100, 0, ALL), (101, 1, ALL), (102, 2, EVEN), (104, 4, ODD), (105, 5, ALL)]

CX, CY, LH = 34, 52, 34          # the listing
TX0, COLW, PITCH = 480, 74, 108  # the time axis: first column, column width, pitch
TOP, ROWH = 52, 27               # the top of the warp columns, height of a thread row
N = len(ROWS)


def frame(k):
    out = []
    # --- the code, with the line of the current step washed ---------------
    out.append(rect(CX - 16, CY - 14, 370, len(CODE) * LH + 26, WHITE, LINE, rx=6))
    out += listing(CX, CY, CODE, size=19, lh=LH, marks={STEPS[k - 1][1]: FILL_ORANGE},
                   width=346)

    # --- the warp, one column per step ----------------------------------------
    out.append(text(TX0 - 20, TOP - 18, "一个 Warp 的 32 个线程", 15, INK, "bold", "start"))
    for r, t in enumerate(ROWS):
        label = "…" if t is None else f"T{t}"
        out.append(mono(TX0 - 12, TOP + r * ROWH + 17, label, 13, MUTED, anchor="end"))
    for i, (pc, _, active) in enumerate(STEPS[:k]):
        x = TX0 + i * PITCH
        current = i == k - 1
        out.append(rect(x - 6, TOP - 6, COLW + 12, N * ROWH + 10, WHITE,
                        ORANGE if current else LINE, rx=5, width=1.8 if current else 1.2))
        for r, t in enumerate(ROWS):
            y = TOP + r * ROWH
            if t is None:
                out.append(text(x + COLW / 2, y + 16, "⋮", 16, MUTED, "bold"))
            elif active(t):
                out.append(rect(x, y + 3, COLW, ROWH - 8, FILL_ORANGE if current else FILL_BLUE,
                                ORANGE if current else BLUE, rx=2, width=1.2))
            else:
                out.append(rect(x, y + 3, COLW, ROWH - 8, FILL_GREY, MUTED, rx=2, width=1,
                                dash="3 3"))
        out.append(mono(x + COLW / 2, TOP + N * ROWH + 24, f"PC {pc}", 15,
                        ORANGE if current else INK, "bold", "middle"))

    axis = TOP + N * ROWH + 40
    out.append(arrow(TX0 - 10, axis, TX0 + 5 * PITCH, axis, MUTED, 1.6))
    out.append(text(TX0 + 5 * PITCH + 4, axis + 22, "时间", 14, MUTED, anchor="end"))

    # --- the legend --------------------------------------------------------------
    ly = H - 22
    out.append(rect(CX - 16, ly - 12, 30, 14, FILL_BLUE, BLUE, rx=2, width=1.2))
    out.append(text(CX + 22, ly, "执行这条指令", 14, MUTED, anchor="start"))
    out.append(rect(CX + 130, ly - 12, 30, 14, FILL_GREY, MUTED, rx=2, width=1, dash="3 3"))
    out.append(text(CX + 168, ly, "等待：不执行这条指令", 14, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    for k in range(1, len(STEPS) + 1):
        save(f"divergence-{k}", W, H, frame(k))
