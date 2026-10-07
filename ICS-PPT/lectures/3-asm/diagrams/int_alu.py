#!/usr/bin/env python3
"""imull in the ALU: D and S enter the multiplier, the 64-bit product leaves.

Where the two halves of the product go depends on the format: the two- and
three-operand forms write the low 32 bits back to D and drop the high half,
the one-operand form writes the high half to %edx and the low half to %eax.
Run it to refresh ../assets/int-alu.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, GREEN, INK, MUTED,
                    ORANGE, WHITE, arrow, box, rect, save, text)

W, H = 1120, 136


def build():
    out = []
    # operands
    out += box(20, 8, 240, 44, "D（寄存器，如 %eax）", FILL_BLUE, BLUE, 16)
    out += box(20, 84, 240, 44, "S（寄存器、内存或立即数）", FILL_BLUE, BLUE, 16)
    # ALU with the multiplier; the operand arrows are drawn over the ALU fill
    out.append(rect(310, 4, 250, 128, FILL_ORANGE, ORANGE, rx=10, width=1.8))
    out.append(text(340, 74, "ALU", 18, INK, "bold"))
    out.append(arrow(260, 30, 366, 30, BLUE, 2.2))
    out.append(arrow(260, 106, 366, 106, BLUE, 2.2))
    out += box(370, 14, 170, 108, "乘法器", WHITE, ORANGE, 17, sub="imull")
    out.append(arrow(540, 68, 592, 68, ORANGE, 2.4))
    # the 64-bit product, two halves
    out.append(rect(596, 4, 168, 128, FILL_GREY, MUTED, rx=8, width=1.4, dash="5,4"))
    out.append(text(680, 22, "64 位乘积", 14, INK, "bold"))
    out += box(608, 30, 144, 40, "高 32 位", WHITE, ORANGE, 15)
    out += box(608, 86, 144, 40, "低 32 位", WHITE, ORANGE, 15)
    # where each half goes, by format
    out.append(arrow(752, 50, 826, 50, GREEN, 2.4))
    out.append(arrow(752, 106, 826, 106, GREEN, 2.4))
    out += box(830, 24, 270, 52, "单操作数格式：写入 %edx", FILL_GREY, GREEN, 15,
               sub="二、三操作数格式：丢弃")
    out += box(830, 80, 270, 52, "二、三操作数格式：写回 D", FILL_GREY, GREEN, 15,
               sub="单操作数格式：写入 %eax")
    return out


if __name__ == "__main__":
    save("int-alu", W, H, build())
