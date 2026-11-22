#!/usr/bin/env python3
"""The machine of quiz question 5 (1): memory, one link, one CPU.

The memory holds the two vectors and sends them to the CPU over one link of
96 GB/s. The CPU runs at 4 GHz and starts at most 5 instructions per cycle.
Uses the deck's svgkit, so the style is the deck's.
Run it to refresh ../assets/q-cpu-memory.svg.
"""

import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from svgkit import BLUE, FILL_BLUE, FILL_GREY, INK, ORANGE, arrow, box, save, text

W, H = 760, 92
MEM_X, MEM_W = 20, 150
CPU_X, CPU_W = 380, 360
BOX_Y, BOX_H = 8, 76
MID = BOX_Y + BOX_H / 2


def build():
    out = box(MEM_X, BOX_Y, MEM_W, BOX_H, "内存", FILL_GREY, INK, size=18,
              sub="存放向量 w 与 x", sub_size=14)
    out.append(arrow(MEM_X + MEM_W + 2, MID, CPU_X - 2, MID, ORANGE, 3))
    out.append(text((MEM_X + MEM_W + CPU_X) / 2, MID - 14, "带宽 96 GB/s", 17, ORANGE,
                    "bold"))
    out += box(CPU_X, BOX_Y, CPU_W, BOX_H, "CPU", FILL_BLUE, BLUE, size=18,
               sub="主频 4 GHz，每个周期最多 5 条指令开始执行", sub_size=14)
    return out


if __name__ == "__main__":
    save("q-cpu-memory", W, H, build())
