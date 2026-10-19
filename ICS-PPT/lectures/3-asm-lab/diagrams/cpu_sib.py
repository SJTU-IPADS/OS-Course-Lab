#!/usr/bin/env python3
"""The SIB byte: scale, index and base, on d5 of add 0x403020(,%rdx,8),%r8
(guest/matmul, 40122e) and on 24 of mov (%rsp),%rdi (guest/hello, 401000).

Run it to refresh ../assets/cpu_sib.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, LINE, MUTED, WHITE, line, rect, save, text
from cpu_kit import (BASE, DISP, INDEX, MODRM, OPCODE, PREFIX, PURPLE, SCALE, SIB,
                     bit_fields, byte_row, code, note)

W, H = 1120, 300


def build():
    out = [code(20, 36, "4c 03 04 d5 20 30 40 00   add 0x403020(,%rdx,8),%r8", 16, INK),
           note(560, 36, "guest/matmul，地址 40122e")]
    shapes, right = byte_row(20, 54, [(["4c"], PREFIX), (["03"], OPCODE), (["04"], MODRM),
                                      (["d5"], SIB), (["20", "30", "40", "00"], DISP)])
    out += shapes
    sx = 20 + 3 * 38 + 19
    out.append(line(sx, 86, sx, 110, PURPLE, 1.6, "4 3"))
    out.append(text(20, 132, "SIB", 16, PURPLE, "bold", anchor="start"))
    shapes, bx = bit_fields(96, 112, [("11", SCALE, "scale"), ("010", INDEX, "index"),
                                      ("101", BASE, "base")])
    out += shapes
    out.append(code(bx + 14, 133, "= 0xd5", 15, MUTED))
    X = 400
    out.append(text(X, 76, "scale", 15, PURPLE, "bold", anchor="start"))
    out.append(text(X + 60, 76, "00、01、10、11：比例因子 1、2、4、8（m->scale）", 14, INK, anchor="start"))
    out.append(text(X, 104, "index", 15, PURPLE, "bold", anchor="start"))
    out.append(text(X + 60, 104, "变址寄存器，REX.X 作第 3 位；得到 4（100 且无 REX.X）表示没有变址，scale 记 1",
                    14, INK, anchor="start"))
    out.append(text(X, 132, "base", 15, PURPLE, "bold", anchor="start"))
    out.append(text(X + 60, 132, "基址寄存器，REX.B 作第 3 位；字段 101 且 mod 00：没有基址，随后 4 字节偏移",
                    14, INK, anchor="start"))
    out.append(text(X, 164, "内存操作数的地址 = disp + base + index × scale", 15, INK, "bold",
                    anchor="start"))
    # the two examples read off
    out.append(rect(20, 192, 1080, 92, WHITE, LINE, rx=8, width=1.2, dash="5 4"))
    out.append(text(36, 218, "d5", 15, PURPLE, "bold", anchor="start", font="monospace"))
    out.append(text(100, 218, "scale 11 → 8　　index 010 → %rdx　　base 101 且 mod 00 → 没有基址，偏移 00403020",
                    14, INK, anchor="start"))
    out.append(text(100, 242, "地址 = 0x403020 + %rdx × 8；用掉 ModRM、SIB 与偏移共 6 字节", 14, INK,
                    anchor="start"))
    out.append(text(36, 270, "24", 15, PURPLE, "bold", anchor="start", font="monospace"))
    out.append(text(100, 270, "scale 00 → 1　　index 100 → 没有变址　　base 100 → %rsp；"
                    "mov (%rsp),%rdi 的地址 = %rsp，用掉 2 字节", 14, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("cpu_sib", W, H, build())
