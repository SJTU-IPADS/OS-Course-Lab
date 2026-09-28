#!/usr/bin/env python3
"""The programmer-visible state, and why the register file stays small.

Top: %rip, the 16 general registers, RFLAGS inside the CPU, and the bus to
the virtual memory space. Bottom: one register-file cell needs a word line and
a bit line per port, so its side grows with the port count and its area with
the square; 3 ports against 12 ports is 16 times the area.
Run it to refresh ../assets/visible-state.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE,
                    MUTED, ORANGE, WHITE, arrow, box, cells, line, mono, rect,
                    save, text)

W, H = 1120, 530
REGS = ["%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%rbp", "%rsp",
        "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"]


def cpu():
    out = [rect(20, 20, 760, 300, FILL_GREY, LINE, rx=10),
           text(40, 50, "CPU", 18, INK, "bold", anchor="start")]
    out += box(40, 70, 220, 64, "%rip", FILL_ORANGE, ORANGE, 20,
               font="monospace", sub="下一条指令的地址")
    out.append(text(150, 176, "RFLAGS", 17, INK, "bold"))
    out += cells(46, 188, ["CF", "ZF", "SF", "OF"], 52, 40, WHITE, ORANGE, 16)[0]
    out.append(text(150, 256, "最近一次运算的状态", 14, MUTED))
    out.append(text(520, 50, "通用寄存器堆：16 × 64 位", 17, INK, "bold"))
    for k, r in enumerate(REGS):
        x, y = 300 + (k % 4) * 115, 66 + (k // 4) * 58
        out.append(rect(x, y, 105, 46, FILL_BLUE, BLUE, rx=4, width=1.4))
        out.append(mono(x + 52, y + 29, r, 17, INK, "bold", anchor="middle"))
    return out


def memory():
    out = [rect(900, 20, 200, 300, WHITE, BLUE, rx=10, width=1.6),
           text(1000, 50, "虚拟内存空间", 17, INK, "bold")]
    for k, s in enumerate(["代码", "全局变量", "栈"]):
        out.append(rect(920, 72 + k * 78, 160, 64, FILL_BLUE, LINE, rx=4, width=1.2))
        out.append(text(1000, 110 + k * 78, s, 16, INK))
    out.append(arrow(784, 130, 896, 130, INK, 2.2))
    out.append(text(840, 118, "地址", 14, MUTED))
    out.append(arrow(784, 210, 896, 210, INK, 2.2, both=True))
    out.append(text(840, 198, "数据", 14, MUTED))
    out.append(text(840, 250, "总线", 16, INK, "bold"))
    return out


def port_cell(x, y, ports, px):
    """A storage cell crossed by one word line and one bit line per port."""
    side = ports * px
    out = [rect(x, y, side, side, WHITE, INK, rx=2, width=1.4)]
    for k in range(ports):
        t = (k + 0.5) * px
        out.append(line(x - 10, y + t, x + side + 10, y + t, BLUE, 1.2))
        out.append(line(x + t, y - 10, x + t, y + side + 10, ORANGE, 1.2))
    out.append(rect(x + side / 2 - 9, y + side / 2 - 9, 18, 18, INK, INK, rx=2, width=1))
    return out, side


def ports():
    out = [text(20, 370, "寄存器堆的一个存储单元：", 17, INK, "bold", anchor="start"),
           text(20, 398, "每个读写端口各需一条字线与一条位线", 15, MUTED, anchor="start"),
           line(20, 440, 50, 440, BLUE, 2), text(58, 445, "字线", 14, INK, anchor="start"),
           line(110, 440, 140, 440, ORANGE, 2), text(148, 445, "位线", 14, INK, anchor="start")]
    small, s1 = port_cell(420, 424, 3, 11)
    out += small
    out.append(text(420 + s1 / 2, 490, "3 个端口", 16, INK, "bold"))
    out.append(text(420 + s1 / 2, 512, "2 读 1 写", 14, MUTED))
    big, s2 = port_cell(620, 354, 12, 11)
    out += big
    out.append(text(620 + s2 + 30, 400, "12 个端口（8 读 4 写）", 16, INK, "bold",
                    anchor="start"))
    out.append(text(620 + s2 + 30, 428, "边长 4 倍，面积约 16 倍", 15, ORANGE, "bold",
                    anchor="start"))
    out.append(text(620 + s2 + 30, 456, "面积随端口数的平方增长", 15, MUTED,
                    anchor="start"))
    return out


def build():
    return cpu() + memory() + ports()


if __name__ == "__main__":
    save("visible-state", W, H, build())
