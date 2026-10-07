#!/usr/bin/env python3
"""The x86-64 encoding rules the disassembly exercise needs, on one card.

Left: the four opcodes of the function `mac` and the assembly each stands for.
Right: how the operand byte (ModR/M) splits into 2 + 3 + 3 bits and what the
three groups select. Below: the 3-bit register numbers. Only the two modes the
exercise meets are given, 11 (register) and 00 (memory, address in a register);
the other modes and the special cases of 00 are left out.
The bytes the card must decode are those of
    echo 'movl (%rsi), %eax; imull (%rdi), %eax; addl %edx, %eax; ret' |
        gcc -c -x assembler - -o t.o && objdump -d t.o
Colours follow insn_bytes.py: opcode, operand byte.
Run it to refresh ../assets/x86-rules.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_ORANGE, GREEN, INK, MUTED, ORANGE, cells, mono,
                    rect, save, text)

W, H = 1120, 270
CW, CH = 38, 30                     # one byte; also one bit of the operand byte
MW = 112                            # the operand byte in an opcode rule
LEFT, RIGHT = 24, 400               # left edges of the two blocks
ROW0, PITCH = 40, 40
SIZE = 15

OP = (FILL_ORANGE, ORANGE)
REG = (FILL_GREEN, GREEN)

# opcode bytes, has an operand byte, assembly
RULES = [(["8b"], True, "movl  B, R"),
         (["0f", "af"], True, "imull B, R"),
         (["01"], True, "addl  R, B"),
         (["c3"], False, "ret")]

OPERANDS = ["R：rrr 号寄存器",
            "B：mm 为 11 时，是 bbb 号寄存器",
            "B：mm 为 00 时，是内存操作数，地址在 bbb 号寄存器中，例如 (%rsi)"]

NAMES = ["%rax", "%rcx", "%rdx", "%rbx", "%rsp", "%rbp", "%rsi", "%rdi"]


def mid(y):
    return y + CH / 2 + 5


def opcodes():
    out = []
    for k, (op, operand, asm) in enumerate(RULES):
        y = ROW0 + k * PITCH
        shapes, right = cells(LEFT, y, op, CW, CH, *OP, SIZE)
        out += shapes
        if operand:
            out.append(rect(right, y, MW, CH, *REG, rx=2, width=1.2))
            out.append(text(right + MW / 2, mid(y), "操作数字节", SIZE, INK))
        out.append(mono(LEFT + 2 * CW + MW + 18, mid(y) + 1, asm, 16, INK))
    return out


def operand_byte():
    out, x = [], RIGHT
    for bits, name in ((2, "mm"), (3, "rrr"), (3, "bbb")):
        out.append(rect(x, ROW0, bits * CW, CH, *REG, rx=2, width=1.2))
        out.append(mono(x + bits * CW / 2, mid(ROW0), name, SIZE, INK, "bold",
                        anchor="middle"))
        x += bits * CW
    out.append(text(x + 16, mid(ROW0), "操作数字节写成二进制，分成 2 位、3 位、3 位三段", SIZE,
                    INK, anchor="start"))
    for k, line in enumerate(OPERANDS):
        out.append(text(RIGHT, mid(ROW0 + (k + 1) * PITCH), line, SIZE, INK,
                        anchor="start"))
    return out


def registers(y):
    out = [text(LEFT, mid(y) + 1, "寄存器编号", 16, INK, "bold", anchor="start")]
    cw, x0 = 116, 130
    for k, name in enumerate(NAMES):
        x = x0 + k * cw
        out.append(rect(x, y, cw, CH, *REG, rx=2, width=1.2))
        out.append(mono(x + 12, mid(y), f"{k:03b}", SIZE, GREEN, "bold"))
        out.append(mono(x + 52, mid(y), name, SIZE, INK))
    out.append(text(130, y + CH + 22,
                    "32 位操作数使用 32 位名字，例如 000 是 %eax，010 是 %edx；内存地址使用 64 位名字",
                    SIZE, MUTED, anchor="start"))
    return out


def build():
    out = [text(LEFT, 22, "x86-64 的编码规则", 17, INK, "bold", anchor="start"),
           text(LEFT + 160, 22, "只列出本题用到的 4 条指令", SIZE, MUTED, anchor="start")]
    return out + opcodes() + operand_byte() + registers(ROW0 + 4 * PITCH + 4)


if __name__ == "__main__":
    save("x86-rules", W, H, build(), left=-19)    # centres the ink on the canvas
