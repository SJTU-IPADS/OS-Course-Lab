#!/usr/bin/env python3
"""What modrm_decode leaves in modrm_t, and how modrm() of frame/x86.c turns
it into the operands a case reads.

Run it to refresh ../assets/cpu_modrm_t.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE, MUTED,
                    ORANGE, WHITE, arrow, rect, save, text)
from cpu_kit import FILL_GREEN, code, note

W, H = 1120, 300

FIELDS = [("mod", "0..3"), ("sub", "reg 字段本身，0..7"), ("reg", "加 REX.R，0..15"),
          ("rm", "寄存器编号，内存时 -1"), ("base", "0..15，没有时 -1"),
          ("index", "0..15，没有时 -1"), ("scale", "1、2、4、8"), ("rip", "1：相对于下一条指令"),
          ("dsize", "0、1、4"), ("disp", "符号扩展后的位移")]


def build():
    out = [rect(20, 14, 330, 272, FILL_ORANGE, ORANGE, rx=8, width=1.6),
           code(36, 40, "modrm_t m", 16, INK, "bold"),
           note(140, 40, "你的 modrm_decode 填写全部字段")]
    for k, (name, what) in enumerate(FIELDS):
        y = 66 + k * 22
        out.append(code(36, y, name, 14, INK, "bold"))
        out.append(note(100, y, what, 13, INK))
    out.append(arrow(350, 150, 396, 150, ORANGE, 2.2))
    out.append(rect(400, 14, 340, 272, FILL_GREY, MUTED, rx=8, width=1.6))
    out.append(code(416, 40, "modrm(&x)", 16, INK, "bold"))
    out.append(note(520, 40, "frame/x86.c，框架"))
    lines = ["x.sub = m.sub", "x.reg = m.reg", "x.rm  = m.rm",
             "内存操作数（m.rm 为 -1）时：",
             "x.addr = m.disp + R[m.base]", "         + R[m.index] * m.scale",
             "x.riprel = m.rip", "R[RIP] 前进用掉的字节数"]
    for k, s in enumerate(lines):
        y = 70 + k * 24
        if s.startswith("内存") or s.startswith("R[RIP]"):
            out.append(text(416, y, s, 13, INK, anchor="start"))
        else:
            out.append(code(416, y, s, 14, INK))
    out.append(note(416, 270, "没有基址或变址的项不加；读字节不足时记缺页", 12))
    out.append(arrow(740, 150, 786, 150, MUTED, 2.2))
    out.append(rect(790, 14, 310, 272, FILL_BLUE, BLUE, rx=8, width=1.6))
    out.append(text(806, 40, "case 中读到的操作数", 16, INK, "bold", anchor="start"))
    ops = [("get(&x, x.reg)", "reg 寄存器"), ("rm_get(&x)", "x.rm ≥ 0：寄存器 x.rm"),
           ("", "x.rm < 0：内存 ea(&x)"), ("ea(&x)", "x.addr；x.riprel 时再加 R[RIP]"),
           ("x.sub", "0x81、0x83 的运算编号")]
    for k, (fn, what) in enumerate(ops):
        y = 70 + k * 30
        if fn:
            out.append(code(806, y, fn, 14, INK, "bold"))
        out.append(note(806, y + 16 if fn else y, what, 13, INK))
    out.append(note(806, 254, "反汇编器 translate.c 也读这个 modrm_t；", 12))
    out.append(note(806, 270, "函数只读字节，不访问机器", 12))
    return out


if __name__ == "__main__":
    save("cpu_modrm_t", W, H, build())
