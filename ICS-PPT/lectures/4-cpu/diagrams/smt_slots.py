#!/usr/bin/env python3
"""The instructions in the two execution units in each cycle: one thread, then two.

The two execution units of superscalar.py, ten cycles as columns, one row per
unit, as ooo_compare.py draws the execute stage. A schematic: the cells hold
the thread an instruction belongs to (A blue, B orange, the colours of 请求 1
and 请求 2 in part 4), not an instruction, and the counts are chosen to show
the two kinds of empty cell. An empty unit is a dashed white cell, as a bubble.

  thread A alone   2 1 1 0 0 0 2 1 1 1   instructions per cycle, 9 of 20 cells
  thread B added   0 1 1 2 1 2 0 0 1 1   9 more, 18 of 20 cells

Thread A finds one ready instruction in cycles 2-3 and 8-10 (each needs the
result of the one before) and none in cycles 4-6 (it waits for a memory read,
three cycles as in part 3). Thread B uses only the cells A leaves empty, so
A's cells are the same in both grids. B has nothing to start in cycle 1 (A
holds both units), finds one ready instruction in cycle 5 and none in cycles
7-8 (its own memory read).

smt-idle.svg is the thread A grid alone; smt-fill.svg puts it above the grid
of both threads, the A grid at the same place as in smt-idle.svg.
Run it to refresh ../assets/smt-idle.svg and smt-fill.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, MUTED, ORANGE, WHITE, brace, rect,
                    save, text)

W = 1120
LX = 176                            # grid left edge; row labels in [24, LX)
CW, CH = 86, 44
COLS = 10
LEFT = -26                          # centres the ink (x 24..1036) on the canvas
A = [2, 1, 1, 0, 0, 0, 2, 1, 1, 1]
B = [0, 1, 1, 2, 1, 2, 0, 0, 1, 1]
TONE = {"A": (FILL_BLUE, BLUE), "B": (FILL_ORANGE, ORANGE)}


def cx(col):
    return LX + col * CW + CW / 2


def numbers(y):
    out = [text(LX - 12, y, "周期", 15, MUTED, anchor="end")]
    for c in range(COLS):
        out.append(text(cx(c), y, str(c + 1), 15, MUTED))
    return out


def cell(col, y, who):
    x = LX + col * CW
    if who is None:
        return [rect(x + 3, y + 3, CW - 6, CH - 6, WHITE, MUTED, rx=3, width=1.4, dash="5 4"),
                text(x + CW / 2, y + CH / 2 + 6, "空闲", 16, MUTED, "bold")]
    fill, stroke = TONE[who]
    return [rect(x + 3, y + 3, CW - 6, CH - 6, fill, stroke, rx=3, width=1.4),
            text(x + CW / 2, y + CH / 2 + 6, who, 18, stroke, "bold")]


def grid(top, caption, threads):
    """Caption, cycle numbers and the two unit rows; `threads` is [(name, counts)]."""
    out = [text(LX + COLS * CW / 2, top + 18, caption, 16, INK, "bold")]
    out += numbers(top + 44)
    rows = top + 56
    for unit in range(2):
        y = rows + unit * (CH + 6)
        out.append(text(24, y + CH / 2 + 6, f"执行部件 {unit + 1}", 16, INK, "bold",
                        anchor="start"))
        for c in range(COLS):
            who = [name for name, counts in threads for _ in range(counts[c])]
            out += cell(c, y, who[unit] if unit < len(who) else None)
    return out, rows + 2 * CH + 6


def idle():
    out, bottom = grid(0, "线程 A 在两个执行部件中的指令（示意）：20 格用了 9 格", [("A", A)])
    y = bottom + 8
    out += brace(LX + 1 * CW + 3, LX + 3 * CW - 3, y, MUTED, "依赖：只有 1 条就绪", 15)
    out += brace(LX + 3 * CW + 3, LX + 6 * CW - 3, y, MUTED, "等读内存：没有就绪的指令", 15)
    out += brace(LX + 7 * CW + 3, LX + 10 * CW - 3, y, MUTED, "依赖：只有 1 条就绪", 15)
    return out


def fill():
    out, bottom = grid(0, "只有线程 A（示意）：20 格用了 9 格", [("A", A)])
    more, _ = grid(bottom + 22, "线程 A 与线程 B：20 格用了 18 格", [("A", A), ("B", B)])
    return out + more


if __name__ == "__main__":
    save("smt-idle", W, 214, idle(), left=LEFT)
    save("smt-fill", W, 334, fill(), left=LEFT)
