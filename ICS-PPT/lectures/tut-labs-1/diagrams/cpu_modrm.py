#!/usr/bin/env python3
"""The ModRM byte: mod, reg and rm, on the byte 3c of mov (%rsp),%rdi.

Run it to refresh ../assets/cpu_modrm.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import GREEN, INK, LINE, MUTED, ORANGE, WHITE, arrow, line, rect, save, text
from cpu_kit import MOD, MODRM, OPCODE, PREFIX, REG, RM, SIB, bit_fields, byte_row, code, note

W, H = 1120, 300


def build():
    out = [code(20, 36, "48 8b 3c 24   mov (%rsp),%rdi", 16, INK),
           note(340, 36, "guest/hello 的第一条指令，地址 401000")]
    shapes, right = byte_row(20, 54, [(["48"], PREFIX), (["8b"], OPCODE), (["3c"], MODRM),
                                      (["24"], SIB)])
    out += shapes
    # the ModRM byte exploded into bits
    out.append(line(20 + 2 * 38 + 19, 86, 20 + 2 * 38 + 19, 110, GREEN, 1.6, "4 3"))
    out.append(text(20, 132, "ModRM", 16, GREEN, "bold", anchor="start"))
    shapes, bx = bit_fields(96, 112, [("00", MOD, "mod"), ("111", REG, "reg"), ("100", RM, "rm")])
    out += shapes
    out.append(code(bx + 14, 133, "= 0x3c", 15, MUTED))
    # what each field says
    X = 400
    out.append(text(X, 76, "mod", 15, GREEN, "bold", anchor="start"))
    out.append(text(X + 50, 76, "11：rm 是寄存器　00、01、10：另一个操作数在内存中，偏移为 0、1、4 字节",
                    14, INK, anchor="start"))
    out.append(text(X, 104, "reg", 15, GREEN, "bold", anchor="start"))
    out.append(text(X + 50, 104, "寄存器操作数的编号（m->reg）；0x81、0x83 这类组指令用它扩展操作码（m->sub）",
                    14, INK, anchor="start"))
    out.append(text(X, 132, "rm", 15, GREEN, "bold", anchor="start"))
    out.append(text(X + 50, 132, "mod 11：寄存器编号　其余：基址寄存器的编号，两个例外：",
                    14, INK, anchor="start"))
    out.append(text(X + 50, 158, "rm 100：随后是 SIB 字节，基址与变址由它给出", 14, INK, anchor="start"))
    out.append(text(X + 50, 182, "rm 101 且 mod 00：没有基址，随后 4 字节偏移，地址相对于下一条指令（%rip）",
                    14, INK, anchor="start"))
    # the example read off
    out.append(rect(20, 212, 1080, 66, WHITE, LINE, rx=8, width=1.2, dash="5 4"))
    out.append(text(36, 238, "这一条", 15, INK, "bold", anchor="start"))
    out.append(text(100, 238, "mod 00：内存操作数，没有偏移　　reg 111：%rdi（REX.R 为 0）　　"
                    "rm 100：读随后的 SIB 字节 24", 14, INK, anchor="start"))
    out.append(text(100, 264, "操作码 8b 规定 reg 是目标：reg = rm，即 %rdi = 内存中的 8 字节（REX.W）",
                    14, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("cpu_modrm", W, H, build())
