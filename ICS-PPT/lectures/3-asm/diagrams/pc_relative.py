#!/usr/bin/env python3
"""A direct jump's target: the next %rip plus the signed offset in the code.

Bytes and addresses are those of `objdump -d dot.o` for the -Og build: jl at
0x34 is 7c ea, so the offset byte 0xea = -22 is added to the next %rip 0x36
and gives .L3 = 0x20. The right half works the same example out in text: the
three instructions around the jump, then the three steps of the addition.
Run it to refresh ../assets/pc-relative.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MONO, MONO_EM, MUTED, ORANGE, WHITE, arrow, box,
                    elbow, mono, rect, save, text)

W, H = 1120, 290
MONOFONT = MONO


def adder():
    out = []
    out += box(20, 44, 240, 66, "%rip = 0x36", FILL_BLUE, BLUE, 18, font=MONOFONT,
               sub="下一条指令的地址")
    out += box(20, 158, 240, 66, "偏移 = -22", FILL_ORANGE, ORANGE, 18,
               sub="指令字节 0xea，符号扩展")
    x, y0, y1 = 320, 26, 242
    m = (y0 + y1) / 2
    d = (f"M {x} {y0} L {x + 80} {y0 + 60} L {x + 80} {y1 - 60} L {x} {y1} "
         f"L {x} {m + 20} L {x + 18} {m} L {x} {m - 20} Z")
    out.append(f'<path d="{d}" fill="{FILL_GREY}" stroke="{INK}" stroke-width="2"/>')
    out.append(text(x + 48, m + 10, "+", 28, INK, "bold"))
    out.append(arrow(260, 77, 316, 77, BLUE, 2.2))
    out.append(arrow(260, 191, 316, 191, ORANGE, 2.2))
    out += box(460, 101, 180, 66, "目标 = 0x20", FILL_GREEN, GREEN, 18, font=MONOFONT,
               sub=".L3：循环体开头")
    out.append(arrow(400, 134, 456, 134, GREEN, 2.4))
    out.append(elbow([(550, 99), (550, 12), (140, 12), (140, 40)], GREEN, 2))
    out.append(text(560, 40, "写入 %rip", 15, GREEN, "bold", anchor="start"))
    return out


def example():
    x0, size = 690, 17
    ch = size * MONO_EM
    out = [text(x0, 30, "示例：dot.o（-Og）中的 jl .L3", 18, INK, "bold", anchor="start")]
    out.append(rect(x0, 44, 420, 110, WHITE, LINE, rx=6, width=1.4))
    out.append(rect(x0 + 6, 82, 408, 32, FILL_ORANGE, "none", rx=3, width=0))
    lines = [("0x32", "39 d0", "", "cmp  %edx, %eax"),
             ("0x34", "7c", "ea", "jl   .L3"),
             ("0x36", "44 89 c8", "", "mov  %r9d, %eax")]
    for k, (addr, code, off, asm) in enumerate(lines):
        y = 72 + k * 34
        out.append(mono(x0 + 14, y, addr, size, MUTED))
        out.append(mono(x0 + 14 + 6 * ch, y, code, size, INK))
        if off:
            out.append(mono(x0 + 14 + 9 * ch, y, off, size, ORANGE, "bold"))
        out.append(mono(x0 + 14 + 16 * ch, y, asm, size, INK))
    steps = [("① 下一条指令地址：0x34 + 2 = 0x36", BLUE),
             ("② 偏移字节 0xea 符号扩展：0xea - 0x100 = -22", ORANGE),
             ("③ 目标地址：0x36 + (-22) = 0x20，即 .L3", GREEN)]
    for k, (s, color) in enumerate(steps):
        out.append(text(x0, 190 + k * 32, s, 17, color, "bold", anchor="start"))
    out.append(text(x0, 278, "向后跳转偏移为负；代码整体搬移后偏移不变", 15, MUTED,
                    anchor="start"))
    return out


def build():
    return ['<g transform="translate(0,14)">'] + adder() + ["</g>"] + example()


if __name__ == "__main__":
    save("pc-relative", W, H, build())
