#!/usr/bin/env python3
"""The three files of lab 1, the four scored parts, and who calls what.

Run it to refresh ../assets/cpu_parts.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREY, INK, LINE, MUTED, ORANGE, WHITE, arrow, elbow, rect,
                    save, text)
from cpu_kit import GIVEN, TODO, code, note

W, H = 1120, 330

FILES = [
    ("src/lab1/modrm.c", "第 1 部分 · 30 分",
     [("modrm_decode", TODO)],
     "ModRM、SIB 与位移的解析"),
    ("src/lab1/alu.c", "第 2 部分 · 30 分",
     [("x86_szp  x86_logic  x86_sub", TODO), ("x86_dec  x86_neg  x86_mul", TODO),
      ("shr、sar in x86_shift", TODO), ("x86_add  x86_inc  x86_div  shl", GIVEN)],
     "整数运算的结果与五个标志位"),
    ("src/lab1/interp.c", "第 3 部分 · 30 分，第 4 部分 · 10 分",
     [("cond", TODO), ("整数指令的 case", TODO), ("五条向量指令的 case", TODO),
      ("主循环，0x81/0x83、push、jmp", GIVEN), ("vphaddd、vextracti128", GIVEN)],
     "解释器：按操作码修改寄存器与内存"),
]


def file_box(x, y, w, name, part, items, what):
    out = [rect(x, y, w, 216, WHITE, INK, rx=8, width=1.6),
           code(x + 14, y + 26, name, 16, INK, "bold"),
           text(x + 14, y + 48, part, 14, ORANGE, "bold", anchor="start"),
           note(x + 14, y + 68, what, 13)]
    for k, (label, (fill, stroke)) in enumerate(items):
        yy = y + 80 + k * 26
        out.append(rect(x + 14, yy, w - 28, 22, fill, stroke, rx=3, width=1.2))
        out.append(code(x + 22, yy + 16, label, 13, INK))
    return out


def build():
    out = []
    xs = [20, 330, 690]
    ws = [290, 340, 410]
    for (name, part, items, what), x, w in zip(FILES, xs, ws):
        out += file_box(x, 14, w, name, part, items, what)
    # legend
    out.append(rect(20, 248, 22, 16, *TODO, rx=3, width=1.2))
    out.append(text(48, 261, "你写的", 14, INK, anchor="start"))
    out.append(rect(120, 248, 22, 16, *GIVEN, rx=3, width=1.2))
    out.append(text(148, 261, "已经写出的示例，评测同样检查", 14, INK, anchor="start"))
    # who calls what
    out.append(text(20, 294, "调用关系", 15, INK, "bold", anchor="start"))
    out.append(text(100, 294, "解释器的 modrm() 与反汇编器 --disasm 都调用 modrm_decode；"
                    "解释器的 case 经 x86_alu 调用 alu.c 的前四个函数；", 14, INK, anchor="start"))
    out.append(text(100, 316, "inc、dec、neg、移位、乘除法的操作数由框架的 x86_other 取出，"
                    "结果与标志位由 alu.c 的其余六个函数计算；向量指令的 case 写在同一个 switch 中",
                    14, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("cpu_parts", W, H, build())
