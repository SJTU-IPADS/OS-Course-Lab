#!/usr/bin/env python3
"""The address generation unit on the lea exercise, and where its sum goes.

Base, index shifted left by the SIB scale field, and displacement go into one
three-input adder. The sum is the effective address: a memory access sends it
to the memory bus, lea writes it into a register and never touches memory.
Values are those of leal 8(%rdi,%rcx,4), %eax with %rdi = 0x2000, %rcx = 3.
Run it to refresh ../assets/agu.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, MUTED, ORANGE, WHITE, arrow, box, elbow, mono, save, text)

W, H = 1120, 324
AX, Y0, Y1 = 500, 14, 290           # adder: left edge, top, bottom
MID = (Y0 + Y1) / 2


def edge(y):
    """Left edge of the adder at height y, notch included."""
    return AX + 22 * max(0.0, 1 - abs(y - MID) / 24)


def inputs():
    out = []
    out += box(30, 20, 200, 60, "Base：%rdi", FILL_BLUE, BLUE, 18, sub="0x2000")
    out += box(30, 122, 200, 60, "Index：%rcx", FILL_BLUE, BLUE, 18, sub="3")
    out += box(30, 236, 200, 60, "Disp：8", FILL_GREY, MUTED, 18, sub="指令中的立即数")
    return out


def shifter():
    out = box(290, 122, 150, 60, "左移 2 位", FILL_ORANGE, ORANGE, 18, sub="× 4 → 12")
    out += box(290, 206, 150, 38, "SIB.scale = 10", WHITE, ORANGE, 15, font="monospace")
    out.append(arrow(365, 204, 365, 186, ORANGE, 2))
    out.append(text(365, 292, "2 位字段选择移位量", 14, ORANGE))
    return out


def adder():
    d = (f"M {AX} {Y0} L {AX + 90} {Y0 + 80} L {AX + 90} {Y1 - 80} L {AX} {Y1} "
         f"L {AX} {MID + 24} L {AX + 22} {MID} L {AX} {MID - 24} Z")
    return [f'<path d="{d}" fill="{FILL_BLUE}" stroke="{BLUE}" stroke-width="2"/>',
            text(AX + 56, MID + 10, "+", 30, BLUE, "bold"),
            text(AX + 45, Y1 + 24, "三输入加法器", 15, INK, "bold")]


def outputs():
    out = box(640, MID - 35, 170, 70, "有效地址", WHITE, INK, 18, sub="0x2014")
    out.append(arrow(AX + 90, MID, 636, MID, INK, 2.6))
    out += box(900, 14, 200, 80, "内存总线", FILL_GREY, MUTED, 18, sub="读写地址 0x2014")
    out += box(900, 214, 200, 80, "寄存器堆", FILL_GREEN, GREEN, 18, sub="%eax = 0x2014")
    out.append(elbow([(814, MID - 10), (850, MID - 10), (850, 54), (896, 54)], MUTED, 2.4))
    out.append(elbow([(814, MID + 10), (850, MID + 10), (850, 254), (896, 254)], GREEN, 2.6))
    out.append(text(862, 116, "访问内存", 15, MUTED, "bold", anchor="start"))
    out.append(mono(862, 186, "lea", 17, GREEN, "bold"))
    out.append(text(862, 206, "不访问内存", 14, GREEN, anchor="start"))
    return out


def build():
    out = inputs() + shifter() + adder() + outputs()
    out.append(arrow(230, 50, edge(50) - 4, 50, BLUE, 2.2))
    out.append(arrow(230, 152, 286, 152, BLUE, 2.2))
    out.append(arrow(440, 152, edge(152) - 4, 152, ORANGE, 2.2))
    out.append(arrow(230, 266, edge(266) - 4, 266, MUTED, 2.2))
    out.append(text(365, 18, "组合逻辑：单周期内完成", 15, MUTED))
    return out


if __name__ == "__main__":
    save("agu", W, H, build())
