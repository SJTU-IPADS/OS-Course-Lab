#!/usr/bin/env python3
"""Where the data of dot_product lives: sum and i in registers, w and x in main memory.

The processor (dashed box) holds two register cells, sum and i, and touches no
memory for them. Main memory (green) holds the arrays w and x as rows of ints;
the elements w[i] and x[i] of the current iteration are orange, and an orange
arrow per array carries 4 bytes into the processor. Run it to refresh
../assets/dot-memory.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, MONO, MUTED, ORANGE,
                    arrow, rect, save, text)

W, H = 440, 232
CPU = (4, 8, 140, 216)                  # x, y, w, h of the processor
MEM = (204, 8, 232, 216)                # x, y, w, h of main memory
REG_X, REG_W, REG_H = 19, 110, 42       # register cells
ARR_X, CELL_W, CELL_H = 248, 44, 42     # first array cell
ROW_Y = (62, 142)                       # rows of w and x
NAMES = ["[0]", "[1]", "[i]", "[n-1]"]


def figure():
    out = []
    x, y, w, h = CPU
    out.append(rect(x, y, w, h, "none", MUTED, rx=8, width=1.6, dash="6 4"))
    out.append(text(x + 12, y + 24, "处理器", 17, INK, "bold", "start"))
    out.append(text(x + w / 2, y + 52, "寄存器", 15, MUTED))
    for k, name in enumerate(("sum", "i")):
        ry = 82 + k * 56
        out.append(rect(REG_X, ry, REG_W, REG_H, FILL_BLUE, BLUE, rx=3, width=1.6))
        out.append(text(REG_X + REG_W / 2, ry + 27, name, 18, INK, "bold", font=MONO))
    out.append(text(x + w / 2, 206, "无地址的局部变量", 15, MUTED))
    x, y, w, h = MEM
    out.append(rect(x, y, w, h, FILL_GREEN, GREEN, rx=8, width=1.6))
    out.append(text(x + 12, y + 24, "内存", 17, GREEN, "bold", "start"))
    for row, arr in zip(ROW_Y, ("w", "x")):
        out.append(text(ARR_X - 10, row + 27, arr, 18, INK, "bold", "end", font=MONO))
        for k, name in enumerate(NAMES):
            hot = name == "[i]"
            cx = ARR_X + k * CELL_W
            out.append(rect(cx, row, CELL_W, CELL_H, FILL_ORANGE if hot else "#ffffff",
                            ORANGE if hot else GREEN, rx=2, width=2 if hot else 1.4))
            out.append(text(cx + CELL_W / 2, row + 27, name, 13, INK,
                            "bold" if hot else "normal", font=MONO))
    for row in ROW_Y:
        yy = row + CELL_H / 2
        out.append(arrow(MEM[0], yy, CPU[0] + CPU[2], yy, ORANGE, 3))
        out.append(text((MEM[0] + CPU[0] + CPU[2]) / 2, yy - 12, "4 字节", 13, ORANGE, "bold"))
    return out


if __name__ == "__main__":
    save("dot-memory", W, H, figure())
