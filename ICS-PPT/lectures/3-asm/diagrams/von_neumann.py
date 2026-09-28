#!/usr/bin/env python3
"""The CPU and DRAM as two chips joined by a bus.

The CPU holds the control unit, the ALU and the registers; the DRAM holds a
grid of one-transistor-one-capacitor cells and no arithmetic gates. The dashed
line is the chip boundary the data has to cross.
Run it to refresh ../assets/von-neumann.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED,
                    ORANGE, WHITE, arrow, box, line, rect, save, text)

W, H = 1120, 260


def cpu():
    out = [rect(20, 20, 420, 200, FILL_BLUE, BLUE, rx=10, width=1.8),
           text(230, 50, "CPU 芯片", 18, INK, "bold")]
    out += box(40, 70, 170, 56, "控制单元", WHITE, MUTED, 17)
    out += box(40, 142, 170, 60, "寄存器", WHITE, BLUE, 17)
    out += box(240, 70, 180, 132, "ALU", FILL_ORANGE, ORANGE, 19,
               sub="乘法器 / 加法器")
    return out


def cell(x, y):
    """One DRAM cell: an access transistor and a storage capacitor."""
    return [line(x, y, x + 14, y, INK, 1.4),                 # transistor channel
            line(x + 7, y - 7, x + 7, y, INK, 1.4),          # gate from the word line
            line(x + 14, y, x + 14, y + 8, INK, 1.4),
            line(x + 8, y + 8, x + 20, y + 8, INK, 1.6),     # capacitor plates
            line(x + 8, y + 12, x + 20, y + 12, INK, 1.6)]


def dram():
    out = [rect(680, 20, 420, 200, FILL_GREY, MUTED, rx=10, width=1.8),
           text(890, 50, "DRAM 芯片", 18, INK, "bold")]
    for r in range(4):
        for c in range(9):
            out += cell(712 + c * 40, 78 + r * 26)
    out.append(text(890, 204, "1T1C 单元阵列：只存储电荷，没有运算门电路", 15, MUTED))
    return out


def build():
    out = cpu() + dram()
    for y in (104, 136):
        out.append(line(440, y, 680, y, BLUE, 5))
    out.append(arrow(520, 120, 600, 120, BLUE, 2, both=True))
    out.append(text(560, 92, "总线", 16, BLUE, "bold"))
    out.append(line(560, 150, 560, 250, INK, 1.6, "6 5"))
    out.append(text(560, 176, "物理边界", 15, INK, "bold"))
    out.append(text(500, 240, "计算部件", 15, MUTED, anchor="end"))
    out.append(text(620, 240, "存储介质", 15, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("von-neumann", W, H, build())
