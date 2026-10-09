#!/usr/bin/env python3
"""The whole gcc -S listing of dot_product, with its parts named on the right.

The listing is `gcc -Og -fcf-protection=none -S dot.c` with the assembler
directives removed.
Run it to refresh ../assets/dot-function.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, RED, WHITE, listing, rect, save,
                    text, vbrace)

W, H = 1120, 520
LH = 32
Y0 = 16

DOT_S = ["dot_product:",
         "\tmovl\t$0, %eax",
         "\tmovl\t$0, %r9d",
         "\tjmp\t.L2",
         ".L3:",
         "\tmovslq\t%eax, %r8",
         "\tmovl\t(%rsi,%r8,4), %ecx",
         "\timull\t(%rdi,%r8,4), %ecx",
         "\taddl\t%ecx, %r9d",
         "\taddl\t$1, %eax",
         ".L2:",
         "\tcmpl\t%edx, %eax",
         "\tjl\t.L3",
         "\tmovl\t%r9d, %eax",
         "\tret"]

GROUPS = [(1, 3, ["入口：i = 0，sum = 0，先跳到判断",
                  "参数 %rdi、%rsi、%edx 由调用方放好"], MUTED, FILL_GREY),
          (4, 12, ["循环计算：第二部分的循环代码",
                   "只用 caller-saved 寄存器"], BLUE, FILL_BLUE),
          (13, 13, ["返回值装填：sum → %eax"], ORANGE, FILL_ORANGE),
          (14, 14, ["返回：ret"], GREEN, FILL_GREEN)]


def build():
    marks = {r: fill for a, b, _, _, fill in GROUPS for r in range(a, b + 1)
             if not DOT_S[r].endswith(":")}
    out = [rect(40, Y0 - 6, 470, LH * len(DOT_S) + 12, WHITE, LINE, rx=6)]
    out += listing(60, Y0, DOT_S, 18, LH, marks=marks, width=430)
    for a, b, lines, color, _ in GROUPS:
        y0, y1 = Y0 + a * LH + 4, Y0 + (b + 1) * LH - 4
        out += vbrace(530, y0, y1, color, lines[0], 17)
        for i, s in enumerate(lines[1:], 1):
            out.append(text(556, (y0 + y1) / 2 + 6 + i * 26, s, 16, MUTED, anchor="start"))
    out.append(text(556, 40, "叶子函数：不建立栈帧，也没有寄存器恢复开销", 16, INK,
                    "bold", anchor="start"))
    return out


if __name__ == "__main__":
    save("dot-function", W, H, build())
