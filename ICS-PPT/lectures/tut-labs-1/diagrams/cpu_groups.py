#!/usr/bin/env python3
"""The integer instructions of interp.c in five groups, by the bytes after
the opcode: what a case reads, with one instruction of guest/sum each
(guest_main at 4010fb, and the ret at 4010e1 of putint).

Run it to refresh ../assets/cpu_groups.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, LINE, MUTED, ORANGE, line, rect, save, text
from cpu_kit import CH, CW, DISP, IMM, MODRM, OPCODE, PREFIX, SIB, byte_row, code, note

W, H = 1120, 340
ROW0, PITCH = 40, 58

GROUPS = [
    ("只有操作码", [(["c3"], OPCODE)], "ret",
     "不读；push/pop 的寄存器编号在 op 的低 3 位", "R[RIP] = 栈顶 8 字节，R[RSP] += 8"),
    ("操作数字节", [(["49"], PREFIX), (["01"], OPCODE), (["c0"], MODRM)], "add %rax,%r8",
     "modrm(&x)", "rm = rm + reg；运算编号是 op >> 3"),
    ("操作数字节，立即数", [(["48"], PREFIX), (["83"], OPCODE), (["c0"], MODRM), (["01"], IMM)],
     "add $0x1,%rax", "modrm(&x)，再 (int8_t)fetch8 或 fetch_imm", "rm = rm + imm；运算编号是 x.sub"),
    ("立即数", [(["ba"], OPCODE), (["e8", "03", "00", "00"], IMM)], "mov $0x3e8,%edx",
     "fetch_imm；x.size 为 8 时 fetch64", "寄存器 = imm；寄存器编号同 push"),
    ("跳转的偏移", [(["7e"], OPCODE), (["12"], IMM)], "jle 401116",
     "(int8_t)fetch8 或 (int32_t)fetch32", "cond 为 1 时 R[RIP] += rel"),
]


def build():
    out = [note(20, 24, "操作码之后的字节"), note(392, 24, "guest/sum 中的一条"),
           note(565, 24, "case 中读指令的调用"), note(862, 24, "做的事")]
    for k, (name, groups, asm, reads, does) in enumerate(GROUPS):
        y = ROW0 + k * PITCH
        out.append(text(20, y + CH / 2 + 5, name, 15, INK, "bold", anchor="start"))
        shapes, right = byte_row(180, y, groups)
        out += shapes
        out.append(code(392, y + CH / 2 + 5, asm, 15, INK))
        out.append(note(565, y + CH / 2 + 5, reads, 13, INK))
        out.append(note(862, y + CH / 2 + 5, does, 12, INK))
        if k < 4:
            out.append(line(20, y + CH + 12, W - 20, y + CH + 12, LINE, 1, "4 4"))
    return out


if __name__ == "__main__":
    save("cpu_groups", W, H, build())
