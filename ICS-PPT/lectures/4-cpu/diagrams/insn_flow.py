#!/usr/bin/env python3
"""The three steps of the instruction flow and the loop through them.

Drawn for the page "指令流" (ICS1 lecture 14, page 19, "SEQ 硬件结构"): fetch by
PC, the four stages that follow, the clock edge writing the next address into
PC, and the arrow back to fetch for the next instruction. Each box carries the
number of its step in the slide's list. Fetch and the four stages take their
colours from timegrid.TONE; the clock edge is ink, as in clock.py.
Run it to refresh ../assets/insn-flow.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, LINE, MUTED, WHITE, arrow, circle, elbow, rect, save, text
from timegrid import TONE

W, H = 1120, 252
TOP, BH = 24, 128             # boxes
MID = TOP + BH / 2
BOXES = [(40, 260), (380, 420), (880, 200)]     # (x, width) of steps 1, 2, 3
CHIP, CGAP = 84, 28           # a stage inside step 2, and the gap between two
LOOP = 214                    # the line back to fetch


def step(k, x):
    return [circle(x + 4, TOP + 4, 15, WHITE, INK, 1.6),
            text(x + 4, TOP + 10, str(k), 16, INK, "bold")]


def build():
    out = []
    (x1, w1), (x2, w2), (x3, w3) = BOXES
    fill, stroke, _ = TONE["取指"]
    out.append(rect(x1, TOP, w1, BH, fill, stroke, rx=8, width=1.8))
    out.append(text(x1 + w1 / 2, TOP + 50, "取指", 20, stroke, "bold"))
    out.append(text(x1 + w1 / 2, TOP + 80, "按 PC 从指令内存", 16, INK))
    out.append(text(x1 + w1 / 2, TOP + 104, "读出指令", 16, INK))

    out.append(rect(x2, TOP, w2, BH, WHITE, LINE, rx=8, width=1.8))
    cx = x2 + (w2 - 4 * CHIP - 3 * CGAP) / 2
    for k, stage in enumerate(["译码", "执行", "访存", "写回"]):
        fill, stroke, ink = TONE[stage]
        x = cx + k * (CHIP + CGAP)
        out.append(rect(x, TOP + 28, CHIP, 46, fill, stroke, rx=6, width=1.6))
        out.append(text(x + CHIP / 2, TOP + 57, stage, 17,
                        WHITE if stage == "写回" else INK, "bold"))
        if k:
            out.append(arrow(x - CGAP + 3, TOP + 51, x - 3, TOP + 51, INK, 1.8))
    out.append(text(x2 + w2 / 2, TOP + 104, "依次经过四个阶段", 16, INK))

    out.append(rect(x3, TOP, w3, BH, WHITE, INK, rx=8, width=2.6))
    out.append(text(x3 + w3 / 2, TOP + 50, "时钟沿", 20, INK, "bold"))
    out.append(text(x3 + w3 / 2, TOP + 80, "把下一条指令的", 16, INK))
    out.append(text(x3 + w3 / 2, TOP + 104, "地址写入 PC", 16, INK))

    out.append(arrow(x1 + w1 + 4, MID, x2 - 6, MID, INK, 2.2))
    out.append(arrow(x2 + w2 + 4, MID, x3 - 6, MID, INK, 2.2))
    out.append(elbow([(x3 + w3 / 2, TOP + BH + 2), (x3 + w3 / 2, LOOP), (x1 + w1 / 2, LOOP),
                      (x1 + w1 / 2, TOP + BH + 6)], INK, 2.2))
    out.append(text((x1 + x3 + w1 / 2 + w3 / 2) / 2, LOOP - 10, "下一条指令", 16, MUTED))
    for k, (x, _) in enumerate(BOXES, 1):
        out += step(k, x)
    return out


if __name__ == "__main__":
    save("insn-flow", W, H, build())
