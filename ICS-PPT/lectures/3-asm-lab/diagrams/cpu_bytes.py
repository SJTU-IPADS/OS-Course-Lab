#!/usr/bin/env python3
"""How the bytes of an instruction are recognised: four real instructions of
the guests split into prefix, opcode, ModRM, SIB, displacement and immediate,
and the op value each opcode becomes. The first two rows have the same REX
and opcode and differ in the ModRM byte: 04 puts the other operand in memory,
with a SIB and a displacement after it, c0 makes it the register %rax. Below, the first of them, add, through
one pass of interp_run: the functions called in order, the bytes each reads,
and what it does. The calls are those of case 0x03 in the reference
interp.c, with the values of this instruction.

The instructions are lines of objdump -d guest/matmul (40122e), guest/deep
(sub $0x408), guest/hello (401046) and guest/simd (40123e). The second row
is add %rax,%r8 in its 03 form: guest/sum has the same instruction at 401120
as 49 01 c0, the form as chooses; "{load} add %rax,%r8" makes as emit
4c 03 c0, checked with objdump -d.
Run it to refresh ../assets/cpu_bytes.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, GREEN, INK, LINE, MUTED, ORANGE, line, rect, save, text
from cpu_kit import (CH, CW, DISP, IMM, MODRM, OPCODE, PREFIX, SIB, byte_row, code,
                     legend, note)

W, H = 1120, 480
AX, BX = 20, 300                    # assembly, left edge of the bytes
ROW0, PITCH = 40, 36

ROWS = [
    ("add 0x403020(,%rdx,8),%r8",
     [(["4c"], PREFIX), (["03"], OPCODE), (["04"], MODRM), (["d5"], SIB),
      (["20", "30", "40", "00"], DISP)],
     "op = 0x03", "ModRM 04：rm 是内存，后面有 SIB 与偏移"),
    ("add %rax,%r8",
     [(["4c"], PREFIX), (["03"], OPCODE), (["c0"], MODRM)],
     "op = 0x03", "ModRM c0：rm 是寄存器 %rax，指令到此结束"),
    ("sub $0x408,%rsp",
     [(["48"], PREFIX), (["81"], OPCODE), (["ec"], MODRM), (["08", "04", "00", "00"], IMM)],
     "op = 0x81", "立即数由 case 中的 fetch_imm 读出"),
    ("js 4010e2",
     [(["0f", "88"], OPCODE), (["96", "00", "00", "00"], IMM)],
     "op = 0x188", "0f 开头：0x100 | 第二个字节"),
    ("vpmulld %ymm1,%ymm0,%ymm0",
     [(["c4", "e2", "7d"], PREFIX), (["40"], OPCODE), (["c1"], MODRM)],
     "op = 0x1240", "VEX：0x1000 | map << 8 | 字节，map 2 即 0f 38"),
]


# one pass of interp_run for the add: call, bytes it reads, what it does, indent
STEPS = [
    ("op = x86_begin(c, &x)", [(["4c"], PREFIX), (["03"], OPCODE)],
     "x.rex = 0x4c，x.size = 8，返回 op = 0x03", 0),
    ("case 0x03:", [], "操作码 03 是 add 的一种形式：reg = reg + rm，rm 是寄存器或内存", 0),
    ("modrm(&x)", [(["04"], MODRM), (["d5"], SIB), (["20", "30", "40", "00"], DISP)],
     "你的 modrm_decode：x.reg = 8 即 %r8，x.rm = -1 即内存，地址 0x403020 + %rdx × 8", 1),
    ("a = get(&x, x.reg)", [], "读 %r8", 1),
    ("b = rm_get(&x)", [], "x.rm = -1，读地址 0x403020 + %rdx × 8 处的 8 字节；第二条 x.rm = 0，读 %rax", 1),
    ("v = x86_alu(&R[RFLAGS], 0, a, b, 8)", [], "运算编号 0x03 >> 3 = 0，即 add：返回 a + b，设置标志位", 1),
    ("set(&x, x.reg, v)", [], "把结果写入 %r8", 1),
    ("x86_end(&x)", [], "返回 0；R[RIP] 已指向下一条指令，下一次循环执行它", 0),
]


def build():
    out = [text(AX, 24, "objdump -d 打印的指令", 14, MUTED, anchor="start"),
           text(BX, 24, "指令的字节，按段着色", 14, MUTED, anchor="start"),
           text(660, 24, "case 的标号", 14, MUTED, anchor="start")]
    for k, (asm, groups, op, why) in enumerate(ROWS):
        y = ROW0 + k * PITCH
        out.append(code(AX, y + CH / 2 + 5, asm, 16, INK))
        shapes, right = byte_row(BX, y, groups)
        out += shapes
        out.append(code(660, y + CH / 2 + 5, op, 16, ORANGE, "bold"))
        out.append(note(770, y + CH / 2 + 5, why, 13))
    mx = BX + 2 * CW                    # the ModRM byte of the first two rows
    out.append(rect(mx - 4, ROW0 - 4, CW + 8, PITCH + CH + 8, "none", GREEN, rx=5, width=2.2))
    y = ROW0 + len(ROWS) * PITCH + 14
    shapes, x = legend(BX, y, [("前缀", PREFIX), ("操作码", OPCODE), ("ModRM", MODRM), ("SIB", SIB),
                               ("偏移", DISP), ("立即数或跳转的偏移", IMM)], size=13)
    out += shapes
    y += 14
    out.append(line(AX, y, W - 20, y, LINE, 1.2, "5 4"))

    # the first instruction through one pass of interp_run
    y += 26
    out.append(text(AX, y, "以第一条 add 为例：interp_run 的一次循环依次调用的函数", 15, INK, "bold",
                    anchor="start"))
    y += 6
    for call, groups, what, indent in STEPS:
        y += 24
        bold = "bold" if call == "case 0x03:" else "normal"
        out.append(code(AX + 16 + 24 * indent, y, call, 14, ORANGE if bold == "bold" else INK, bold))
        if groups:
            shapes, _ = byte_row(372, y - 16, groups, cw=26, ch=21, size=12)
            out += shapes
        out.append(text(548, y, what, 13, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("cpu_bytes", W, H, build())
