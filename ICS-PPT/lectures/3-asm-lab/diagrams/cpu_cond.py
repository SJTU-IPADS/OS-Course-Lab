#!/usr/bin/env python3
"""From the flags to a conditional jump: cond(f, cc) reads the flags, jcc adds
the offset to R[RIP] once the offset is read. The instruction is the
7d f4  jge 401120 at 40112a of guest/sum.

Run it to refresh ../assets/cpu_cond.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE, MUTED, ORANGE,
                    WHITE, arrow, box, cells, line, rect, save, text)
from cpu_kit import IMM, OPCODE, TODO, byte_row, code, note

W, H = 1120, 300


def build():
    out = []
    # flags -> cond -> jump
    out += cells(20, 40, ["CF", "PF", "ZF", "SF", "OF"], 44, 30, [FILL_ORANGE] * 5, ORANGE, 14)[0]
    out.append(note(130, 92, "R[RFLAGS]", 13, anchor="middle"))
    out.append(arrow(240, 55, 286, 55, ORANGE, 2))
    out += box(290, 30, 200, 50, "cond(f, cc)", *TODO, 16, font="monospace")
    out.append(note(390, 100, "cc 取低 4 位，可以直接传 op", 12, anchor="middle"))
    out.append(arrow(490, 55, 536, 55, ORANGE, 2))
    out += box(540, 30, 260, 50, "1：跳转　0：不跳转", WHITE, INK, 15)
    # four of the sixteen conditions
    out.append(text(20, 136, "16 个条件码中的 4 个", 14, INK, "bold", anchor="start"))
    conds = [("4 e", "ZF"), ("5 ne", "!ZF"), ("c l", "SF != OF"), ("d ge", "SF == OF")]
    for k, (cc, rule) in enumerate(conds):
        y = 150 + k * 24
        out.append(code(20, y + 16, cc, 14, INK, "bold"))
        out.append(code(80, y + 16, rule, 14, INK))
    out.append(note(20, 262, "全部 16 个在 interp.c 的注释与 mcpu.h 中", 12))
    out.append(note(20, 282, "x86_other 的 cmovcc、setcc 也调用 cond", 12))
    # the jump
    x = 360
    out.append(line(x - 20, 120, x - 20, H - 10, LINE, 1.2, "5 4"))
    out.append(code(x, 140, "7d f4   jge 401120", 16, INK, "bold"))
    out.append(note(x + 200, 140, "guest/sum，地址 40112a"))
    shapes, right = byte_row(x, 154, [(["7d"], OPCODE), (["f4"], IMM)])
    out += shapes
    out.append(note(right + 12, 175, "op 0x7d：条件 d（ge）；rel = (int8_t)0xf4 = -12", 13, INK))
    # address line
    ax, ay = x, 236
    out.append(line(ax, ay, ax + 720, ay, INK, 1.6))
    marks = [(0, "401120", "目标"), (300, "40112a", "jge 自己"), (360, "40112c", "下一条 = R[RIP]")]
    for dx, addr, label in marks:
        out.append(line(ax + dx, ay - 8, ax + dx, ay + 8, INK, 1.6))
        out.append(code(ax + dx, ay + 26, addr, 13, INK, anchor="middle"))
        out.append(note(ax + dx, ay + 44, label, 12, anchor="middle"))
    out.append(arrow(ax + 360, ay - 22, ax + 4, ay - 22, BLUE, 2))
    out.append(text(ax + 180, ay - 30, "R[RIP] += rel：0x40112c - 12 = 0x401120", 13, BLUE, "bold"))
    out.append(note(ax + 440, 200, "位移读完之后 R[RIP] 才是下一条指令的地址，", 13, INK))
    out.append(note(ax + 440, 220, "因此先读 rel，再加到 R[RIP] 上", 13, INK))
    return out


if __name__ == "__main__":
    save("cpu_cond", W, H, build())
