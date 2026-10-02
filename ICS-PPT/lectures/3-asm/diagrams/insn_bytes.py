#!/usr/bin/env python3
"""The bytes of an instruction, field by field: opcode, registers, constant.

Upper half, x86-64: three instructions whose fields fall on byte boundaries,
then, in grey, a load with a base register and an offset, where the register
numbers and the addressing form share one byte. Lower half: the same load in
Y86-64, the teaching instruction set of CS:APP chapter 4, where every field
is a whole number of hex digits, so the bytes can be read off.

The x86-64 bytes are what the assembler prints:
    echo 'ret; movl $0, %eax; xorl %eax, %eax; movq 8(%rdi), %rcx' |
        gcc -c -x assembler - -o t.o && objdump -d t.o
The Y86-64 bytes follow CS:APP figure 4.2: mrmovq D(rB), rA is 50, then rA:rB,
then D as 8 little-endian bytes; %rcx is register 1 and %rdi register 7, the
numbers x86-64 uses.
Run it to refresh ../assets/insn-bytes.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK,
                    LINE, MUTED, ORANGE, brace, cells, line, mono, save, text)

W, H = 1120, 350
AX, BX = 24, 262                    # assembly text, left edge of the bytes
CW, CH = 38, 34                     # one byte
NW = 46                             # one hex digit of a Y86-64 byte
LX = BX + 5 * CW + 22               # field labels of the x86-64 rows
SIZE = 16

OP = (FILL_ORANGE, ORANGE)
REG = (FILL_GREEN, GREEN)
CONST = (FILL_BLUE, BLUE)
PLAIN = (FILL_GREY, MUTED)

# tone, assembly, [(bytes, colours)], [(x offset, label, colour, weight)]
X86 = [
    (INK, "ret", [(["c3"], OP)],
     [(0, "操作码", ORANGE, "bold")]),
    (INK, "movl $0, %eax", [(["b8"], OP), (["00"] * 4, CONST)],
     [(0, "操作码", ORANGE, "bold"), (3 * SIZE, "：0xb8 + %eax 的编号 0", INK, "normal"),
      (256, "4 字节立即数 0", BLUE, "bold")]),
    (INK, "xorl %eax, %eax", [(["31"], OP), (["c0"], REG)],
     [(0, "操作码", ORANGE, "bold"), (256, "两个寄存器操作数", GREEN, "bold")]),
    (MUTED, "movq 8(%rdi), %rcx", [(["48", "8b", "4f", "08"], PLAIN)],
     [(0, "寄存器编号 1、7 与寻址方式按位合并在 4f 中", MUTED, "normal")]),
]


def row(y, asm, groups, tone=INK):
    """One instruction: its text, then its bytes. Returns (shapes, byte count)."""
    out = [mono(AX, y + CH / 2 + 6, asm, 17, tone)]
    x, n = BX, 0
    for values, (fill, stroke) in groups:
        shapes, x = cells(x, y, values, CW, CH, fill, stroke, SIZE, tone=tone)
        out += shapes
        n += len(values)
    return out, n


def length(y, n, tone=INK):
    return text(W - 24, y + CH / 2 + 6, f"{n} 字节", SIZE, tone, "bold", anchor="end")


def x86_rows():
    out = [text(AX, 22, "x86-64", 17, INK, "bold", anchor="start")]
    for k, (tone, asm, groups, labels) in enumerate(X86):
        y = 38 + k * 46
        shapes, n = row(y, asm, groups, tone)
        out += shapes
        for dx, label, color, weight in labels:
            out.append(text(LX + dx, y + CH / 2 + 6, label, SIZE, color, weight,
                            anchor="start"))
        out.append(length(y, n, tone))
    return out


def y86_row():
    y = 272
    out = [text(AX, 254, "Y86-64：同一操作的编码", 17, INK, "bold", anchor="start"),
           text(AX + 212, 254, "CS:APP 的教学指令集", SIZE, MUTED, anchor="start"),
           mono(AX, y + CH / 2 + 6, "mrmovq 8(%rdi), %rcx", 17, INK)]
    shapes, x = cells(BX, y, ["5", "0"], NW, CH, *OP, SIZE)
    out += shapes
    shapes, x = cells(x, y, ["1", "7"], NW, CH, *REG, SIZE)
    out += shapes
    shapes, right = cells(x, y, ["08"] + ["00"] * 7, CW, CH, *CONST, SIZE)
    out += shapes
    by = y + CH + 3
    out += brace(BX + 2, BX + 2 * NW - 2, by, ORANGE, "操作码", SIZE, 7)
    out += brace(BX + 2 * NW + 2, BX + 3 * NW - 2, by, GREEN, "%rcx", SIZE, 7)
    out += brace(BX + 3 * NW + 2, BX + 4 * NW - 2, by, GREEN, "%rdi", SIZE, 7)
    out += brace(x + 2, right - 2, by, BLUE, "8 字节偏移：数值 8，小端", SIZE, 7)
    out.append(length(y, 10))
    return out


def build():
    return x86_rows() + [line(AX, 228, W - 24, 228, LINE, 1.4, "6 5")] + y86_row()


if __name__ == "__main__":
    save("insn-bytes", W, H, build())
