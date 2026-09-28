#!/usr/bin/env python3
"""Operand forms split into non-memory and memory, each with its C form.

The memory side is one formula, Imm + r_b + r_i * s; each box is that formula
with some terms left out, which is why they share one panel.
Run it to refresh ../assets/addressing-modes.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED,
                    ORANGE, WHITE, arrow, box, mono, rect, save, text)

W, H = 1120, 540

LEFT = [("立即数", "$Imm", "常量 5"), ("寄存器", "r_a", "局部变量 sum")]
RIGHT = [("间接", "(r_a)", "*ptr"), ("基址 + 偏移", "Imm(r_b)", "p->field，栈变量"),
         ("变址", "(r_b,r_i)", "b[i]（char 数组）"), ("比例变址", "Imm(r_b,r_i,s)", "w[i]（int 数组）"),
         ("RIP 相对", "Imm(%rip)", "全局变量 w"), ("绝对", "Imm", "极少使用")]


def leaf(x, y, w, name, syn, c):
    return [rect(x, y, w, 92, WHITE, LINE, rx=6, width=1.4),
            text(x + 16, y + 28, name, 16, INK, "bold", anchor="start"),
            mono(x + 16, y + 58, syn, 19, BLUE, "bold"),
            text(x + 16, y + 82, "C：" + c, 14, MUTED, anchor="start")]


def build():
    out = box(470, 14, 180, 50, "操作数", FILL_GREY, INK, 19)
    out.append(rect(20, 100, 320, 430, FILL_GREY, "none", rx=10, width=0))
    out.append(rect(360, 100, 740, 430, FILL_BLUE, "none", rx=10, width=0))
    out.append(arrow(500, 66, 180, 96, INK, 2))
    out.append(arrow(620, 66, 730, 96, INK, 2))
    out.append(text(180, 134, "非内存操作数", 18, INK, "bold"))
    out.append(text(180, 158, "值就在指令或寄存器里", 14, MUTED))
    out.append(text(730, 134, "内存操作数", 18, INK, "bold"))
    out.append(mono(730, 160, "有效地址 = Imm + r_b + r_i × s", 16, ORANGE, "bold",
                    anchor="middle"))
    for k, (name, syn, c) in enumerate(LEFT):
        out += leaf(40, 180 + k * 110, 280, name, syn, c)
    for k, (name, syn, c) in enumerate(RIGHT):
        out += leaf(380 + (k % 2) * 360, 180 + (k // 2) * 110, 340, name, syn, c)
    return out


if __name__ == "__main__":
    save("addressing-modes", W, H, build())
