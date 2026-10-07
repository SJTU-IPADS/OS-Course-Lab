#!/usr/bin/env python3
"""The bytes of an instruction, field by field: opcode, registers, constant.

Upper half, x86-64: three instructions whose fields fall on byte boundaries.
Lower half: `movl $0, %eax` again in Y86-64, the teaching instruction set of
CS:APP chapter 4, where every field is a whole number of hex digits, so the
bytes can be read off.

The x86-64 bytes are what the assembler prints:
    echo 'ret; movl $0, %eax; xorl %eax, %eax' |
        gcc -c -x assembler - -o t.o && objdump -d t.o
The Y86-64 bytes follow CS:APP figure 4.2: irmovq V, rB is 30, then F:rB
(F means no register), then V as 8 little-endian bytes; %rax is register 0,
the number x86-64 uses.
Run it to refresh ../assets/insn-bytes.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, LINE,
                    MUTED, ORANGE, brace, cells, line, mono, save, text)

W, H = 1120, 322
AX, BX = 24, 262                    # assembly text, left edge of the bytes
CW, CH = 38, 34                     # one byte
NW = 46                             # one hex digit of a Y86-64 byte
LX = BX + 5 * CW + 22               # field labels of the x86-64 rows
SIZE = 16

OP = (FILL_ORANGE, ORANGE)
REG = (FILL_GREEN, GREEN)
CONST = (FILL_BLUE, BLUE)

# tone, assembly, [(bytes, colours)], [(x offset, label, colour, weight)]
X86 = [
    (INK, "ret", [(["c3"], OP)],
     [(0, "操作码", ORANGE, "bold")]),
    (INK, "movl $0, %eax", [(["b8"], OP), (["00"] * 4, CONST)],
     [(0, "操作码", ORANGE, "bold"), (3 * SIZE, "：0xb8 + %eax 的编号 0", INK, "normal"),
      (256, "4 字节立即数 0", BLUE, "bold")]),
    (INK, "xorl %eax, %eax", [(["31"], OP), (["c0"], REG)],
     [(0, "操作码", ORANGE, "bold"), (256, "两个寄存器操作数", GREEN, "bold")]),
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
    y = 226
    out = [text(AX, 208, "Y86-64：同一操作的编码", 17, INK, "bold", anchor="start"),
           text(AX + 212, 208, "CS:APP 的教学指令集", SIZE, MUTED, anchor="start"),
           mono(AX, y + CH / 2 + 6, "irmovq $0, %rax", 17, INK)]
    shapes, x = cells(BX, y, ["3", "0"], NW, CH, *OP, SIZE)
    out += shapes
    shapes, x = cells(x, y, ["F", "0"], NW, CH, *REG, SIZE)
    out += shapes
    shapes, right = cells(x, y, ["00"] * 8, CW, CH, *CONST, SIZE)
    out += shapes
    by = y + CH + 3
    out += brace(BX + 2, BX + 2 * NW - 2, by, ORANGE, "操作码", SIZE, 7)
    # the two register digits are too narrow for both labels on one line
    out += brace(BX + 2 * NW + 2, BX + 3 * NW - 2, by, GREEN, "", SIZE, 7)
    out.append(text(BX + 2.5 * NW, by + 7 + 2 * (SIZE + 4), "无寄存器", SIZE, GREEN, "bold"))
    out += brace(BX + 3 * NW + 2, BX + 4 * NW - 2, by, GREEN, "%rax", SIZE, 7)
    out += brace(x + 2, right - 2, by, BLUE, "8 字节常数 0，小端", SIZE, 7)
    out.append(length(y, 10))
    return out


def build():
    return x86_rows() + [line(AX, 182, W - 24, 182, LINE, 1.4, "6 5")] + y86_row()


if __name__ == "__main__":
    save("insn-bytes", W, H, build())
