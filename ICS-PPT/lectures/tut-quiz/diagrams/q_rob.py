#!/usr/bin/env python3
"""The reorder buffer (ROB), for quiz question 4 (2).

The lecture does not teach the ROB, so the question explains it with this
figure. Four entries stand in program order, earliest on the left; each holds
an instruction, its destination register, its result and its state. Decode
puts a new instruction after the last entry (right), the execution units write
a result into the instruction's own entry (below), and commit takes the
earliest entry into the register file (left). The example is not the one the
question asks about: entries 2 and 4 are done but cannot commit, because
entry 1 is still executing. Below, one band of execution units: entry 1 has
started, entries 2 and 4 have written their results, entry 3 waits for %eax.
Uses the deck's svgkit, so the style is the deck's.
Run it to refresh ../assets/q-rob.svg.
"""

import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK,
                    MUTED, ORANGE, WHITE, arrow, box, elbow, label_line, mono, rect, save,
                    text, FONT, MONO)

W, H = 1150, 330
EX0, EW = 290, 162                  # left edge of entry 1, width of an entry
TOP = 34                            # top of the header row
HH, RH = 30, 34                     # header row, body rows
BODY = TOP + HH
BOTTOM = BODY + 4 * RH
MID = (BODY + BOTTOM) / 2

ENTRIES = [
    ("movl (%rdi), %eax", "%eax", "—", "正在执行"),
    ("addl $1, %ebx", "%ebx", "8", "已完成"),
    ("addl %eax, %ecx", "%ecx", "—", "未执行"),
    ("subl $2, %edx", "%edx", "5", "已完成"),
]
STATE_FILL = {"正在执行": FILL_ORANGE, "已完成": FILL_GREEN, "未执行": WHITE}
HEADS = ["条目 1（最早）", "条目 2", "条目 3", "条目 4（最晚）"]


def centre(k):
    return EX0 + k * EW + EW / 2


def rob():
    out = [text(EX0 + 2 * EW, TOP - 12, "ROB：条目按程序顺序排列", 16, INK, "bold")]
    for k, (inst, dst, res, state) in enumerate(ENTRIES):
        x = EX0 + k * EW
        out.append(rect(x, TOP, EW, HH, FILL_GREY, INK, rx=0, width=1.4))
        out.append(text(x + EW / 2, TOP + 21, HEADS[k], 14, INK, "bold"))
        rows = [
            [(inst, MONO, INK, "normal", 13)],
            [("目的寄存器 ", FONT, MUTED, "normal", 14), (dst, MONO, INK, "normal", 14)],
            [("结果 ", FONT, MUTED, "normal", 14), (res, MONO, INK, "normal", 14)],
        ]
        for r, parts in enumerate(rows):
            y = BODY + r * RH
            out.append(rect(x, y, EW, RH, WHITE, INK, rx=0, width=1.4))
            size = parts[0][4]
            width = sum(len(s) * (0.6 if f == MONO else 1.0) for s, f, _, _, _ in parts) * size
            out += label_line(x + (EW - width) / 2, y + 23, [q[:4] for q in parts], size=size)
        y = BODY + 3 * RH
        out.append(rect(x, y, EW, RH, STATE_FILL[state], INK, rx=0, width=1.4))
        out.append(text(x + EW / 2, y + 23, state, 15, INK, "bold"))
    out.append(rect(EX0, TOP, 4 * EW, HH + 4 * RH, "none", INK, rx=0, width=2.4))
    return out


def around():
    out = []
    # commit: the earliest entry leaves for the register file
    out += box(40, MID - 28, 140, 56, "寄存器堆", FILL_GREY, INK, size=16)
    out.append(arrow(EX0 - 4, MID, 184, MID, GREEN))
    out.append(text((EX0 + 184) / 2, MID - 10, "提交", 15, GREEN, "bold"))
    # decode: a new instruction goes after the last entry
    right = EX0 + 4 * EW
    out += box(right + 70, MID - 28, 120, 56, "译码", FILL_BLUE, BLUE, size=16)
    out.append(arrow(right + 68, MID, right + 4, MID, BLUE))
    out.append(text(right + 36, MID - 10, "进入", 15, BLUE, "bold"))
    # execution: one wide band, since every instruction whose operands are
    # ready gets a unit of its own; each entry has its own arrow
    ey = BOTTOM + 52
    out += box(EX0, ey, 4 * EW, 44, "执行部件：数量足够，多条指令可以同时执行", FILL_ORANGE,
               ORANGE, size=16)
    for k, (_, _, _, state) in enumerate(ENTRIES):
        x = centre(k) - 36
        if state == "正在执行":
            out.append(arrow(x, BOTTOM + 4, x, ey - 2, ORANGE))
            out.append(text(x + 12, BOTTOM + 32, "开始执行", 14, ORANGE, "bold", anchor="start"))
        elif state == "已完成":
            out.append(arrow(x, ey - 2, x, BOTTOM + 4, ORANGE))
            out.append(text(x + 12, BOTTOM + 32, "写入结果", 14, ORANGE, "bold", anchor="start"))
        else:
            out.append(text(centre(k), BOTTOM + 32, "操作数未就绪", 14, MUTED, "bold"))
    # the rule the example shows
    out.append(text(40, H - 12, "条目 1 尚未完成：条目 2、4 已完成，也不能提交", 15, INK,
                    "bold", anchor="start"))
    return out


if __name__ == "__main__":
    save("q-rob", W, H, rob() + around())
