#!/usr/bin/env python3
"""Two idioms from page 22: the xor zero idiom and lea for a constant multiply.

Left: xorl %eax, %eax is recognised at rename; the RAT maps %eax to the
physical zero and the instruction never reaches an execution unit. Right: x*5
as imull (3 cycles) against leal (%rax,%rax,4) (1 cycle) on one clock.
Run it to refresh ../assets/xor-lea.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, arrow, box, cells, line,
                    mono, path, rect, save, text)

W, H = 1120, 262


def rename():
    out = [mono(20, 30, "xorl %eax, %eax", 18, INK, "bold"),
           text(210, 30, "在重命名阶段完成", 16, MUTED, anchor="start")]
    out += box(20, 70, 110, 56, "译码", FILL_BLUE, BLUE, 17)
    out += box(170, 70, 150, 56, "重命名", FILL_GREEN, GREEN, 17, sub="RAT")
    out.append(rect(360, 70, 110, 56, FILL_GREY, LINE, rx=6, width=1.4, dash="5 4"))
    out.append(text(415, 104, "ALU", 17, MUTED, "bold"))
    out += box(510, 70, 100, 56, "完成", FILL_BLUE, BLUE, 17)
    out.append(arrow(130, 98, 166, 98, INK, 2))
    out.append(path("M 320 84 C 380 30, 450 30, 506 84", GREEN, 2.4))
    out.append(text(415, 148, "0 周期，不占 ALU", 14, GREEN, "bold"))
    # the RAT entry
    out.append(text(20, 170, "寄存器别名表（RAT）", 15, INK, "bold", anchor="start"))
    out += cells(20, 184, ["架构寄存器", "物理寄存器"], 150, 30, FILL_GREY, LINE, 14,
                 font="sans-serif")[0]
    out += cells(20, 214, ["%eax", "物理零"], 150, 30, [WHITE, FILL_GREEN], LINE, 15)[0]
    out.append(text(340, 222, "编码 2 字节：31 c0", 14, MUTED, anchor="start"))
    out.append(text(340, 246, "movl $0：5 字节", 14, MUTED, anchor="start"))
    return out


def timing():
    x0, cw = 780, 110
    out = [mono(650, 30, "x * 5", 18, INK, "bold"),
           text(720, 30, "的两种写法", 16, MUTED, anchor="start")]
    # clock
    d = f"M {x0} 90"
    for k in range(3):
        a = x0 + k * cw
        d += f" L {a} 64 L {a + cw / 2} 64 L {a + cw / 2} 90 L {a + cw} 90"
    out.append(path(d, MUTED, 1.6, head=False))
    for k in range(3):
        out.append(text(x0 + k * cw + cw / 2, 56, f"周期 {k + 1}", 13, MUTED))
    for k in range(4):
        out.append(line(x0 + k * cw, 96, x0 + k * cw, 236, LINE, 1, "3 3"))
    out.append(mono(650, 136, "imull", 17, INK, "bold"))
    out.append(rect(x0, 112, 3 * cw, 36, FILL_ORANGE, ORANGE, rx=4, width=1.6))
    out.append(text(x0 + 1.5 * cw, 136, "乘法器：3 个周期", 15, INK))
    out.append(mono(650, 200, "leal", 17, INK, "bold"))
    out.append(mono(650, 222, "(%rax,%rax,4)", 13, MUTED))
    out.append(rect(x0, 176, cw, 36, FILL_GREEN, GREEN, rx=4, width=1.6))
    out.append(text(x0 + cw / 2, 200, "1 个周期", 15, INK))
    out.append(text(x0 + 1.5 * cw + cw / 2, 200, "x + x × 4", 15, MUTED))
    return out


def build():
    return rename() + [line(630, 16, 630, 246, LINE, 1)] + timing()


if __name__ == "__main__":
    save("xor-lea", W, H, build())
