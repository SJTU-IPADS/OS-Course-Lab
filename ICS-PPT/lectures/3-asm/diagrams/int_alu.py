#!/usr/bin/env python3
"""Two 32-bit operands into the ALU; the low 32 bits back to the register.

The status outputs of the same operation go to RFLAGS, which the next part
reads for loop control.
Run it to refresh ../assets/int-alu.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, GREEN, INK, MUTED,
                    ORANGE, WHITE, arrow, box, elbow, rect, save, text)

W, H = 1120, 112


def build():
    out = []
    out += box(20, 8, 220, 42, "%eax（32 位）", FILL_BLUE, BLUE, 17)
    out += box(20, 62, 220, 42, "(%rdi) 或 %ecx（32 位）", FILL_BLUE, BLUE, 16)
    out.append(arrow(240, 29, 316, 29, BLUE, 2.2))
    out.append(arrow(240, 83, 316, 83, BLUE, 2.2))
    out.append(rect(320, 4, 290, 104, FILL_ORANGE, ORANGE, rx=10, width=1.8))
    out.append(text(354, 62, "ALU", 18, INK, "bold"))
    out += box(390, 12, 206, 40, "乘法器（imull）", WHITE, ORANGE, 16)
    out += box(390, 60, 206, 40, "加法器（addl）", WHITE, ORANGE, 16)
    out.append(arrow(610, 30, 676, 30, ORANGE, 2.4))
    out += box(680, 8, 170, 44, "取低 32 位", WHITE, ORANGE, 17)
    out.append(arrow(850, 30, 916, 30, GREEN, 2.4))
    out += box(920, 8, 180, 44, "写回 %eax", FILL_GREY, GREEN, 17)
    out.append(elbow([(610, 82), (676, 82)], MUTED, 2.2))
    out.append(text(643, 72, "状态输出", 14, MUTED))
    out += box(680, 60, 170, 44, "RFLAGS", FILL_GREY, MUTED, 17, font="monospace")
    return out


if __name__ == "__main__":
    save("int-alu", W, H, build())
