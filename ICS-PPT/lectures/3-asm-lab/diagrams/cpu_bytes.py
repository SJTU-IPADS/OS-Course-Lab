#!/usr/bin/env python3
"""How the bytes of an instruction are recognised: four real instructions of
the guests split into prefix, opcode, ModRM, SIB, displacement and immediate,
the op value each opcode becomes, and which code reads each segment.

The instructions are lines of objdump -d guest/matmul (40122e), guest/deep
(sub $0x408), guest/hello (401046) and guest/simd (40123e).
Run it to refresh ../assets/cpu_bytes.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, GREEN, INK, LINE, MUTED, ORANGE, line, save, text
from cpu_kit import (CH, CW, DISP, IMM, MODRM, OPCODE, PREFIX, SIB, byte_row, code,
                     legend, note)

W, H = 1120, 350
AX, BX = 20, 300                    # assembly, left edge of the bytes
ROW0, PITCH = 44, 46

ROWS = [
    ("add 0x403020(,%rdx,8),%r8",
     [(["4c"], PREFIX), (["03"], OPCODE), (["04"], MODRM), (["d5"], SIB),
      (["20", "30", "40", "00"], DISP)],
     "op = 0x03", "一字节操作码：op 就是这个字节"),
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
    y = ROW0 + 4 * PITCH + 2
    out.append(line(AX, y, W - 20, y, LINE, 1.2, "5 4"))
    # who reads which segment
    y += 36
    out.append(text(AX, y, "读这一段的代码", 15, INK, "bold", anchor="start"))
    shapes, x = legend(150, y, [("前缀", PREFIX), ("操作码", OPCODE)])
    out += shapes
    out.append(text(x, y, "x86_begin：x.rex、x.size，返回 op", 14, INK, anchor="start"))
    y += 34
    shapes, x = legend(150, y, [("ModRM", MODRM), ("SIB", SIB), ("位移", DISP)])
    out += shapes
    out.append(text(x, y, "modrm(&x) → 你的 modrm_decode：x.reg、x.sub、x.rm 与内存地址", 14, INK,
                    anchor="start"))
    y += 34
    shapes, x = legend(150, y, [("立即数或跳转的位移", IMM)])
    out += shapes
    out.append(text(x, y, "case 中的 fetch_imm、fetch8、fetch32、fetch64", 14, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("cpu_bytes", W, H, build())
