#!/usr/bin/env python3
"""The programmer-visible state.

%rip, the 16 general registers and RFLAGS inside the CPU, and the bus to the
virtual memory space.
Run it to refresh ../assets/visible-state.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED,
                    ORANGE, WHITE, arrow, box, cells, mono, rect, save, text)

W, H = 1120, 262
REGS = ["%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%rbp", "%rsp",
        "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"]


def cpu():
    out = [rect(20, 6, 760, 250, FILL_GREY, LINE, rx=10),
           text(40, 34, "CPU", 18, INK, "bold", anchor="start")]
    out += box(40, 50, 220, 58, "%rip", FILL_ORANGE, ORANGE, 20,
               font="monospace", sub="下一条指令的地址")
    out.append(text(150, 140, "RFLAGS", 17, INK, "bold"))
    out += cells(46, 152, ["CF", "ZF", "SF", "OF"], 52, 38, WHITE, ORANGE, 16)[0]
    out.append(text(150, 214, "最近一次运算的状态", 14, MUTED))
    out.append(text(520, 34, "通用寄存器堆：16 × 64 位", 17, INK, "bold"))
    for k, r in enumerate(REGS):
        x, y = 300 + (k % 4) * 115, 50 + (k // 4) * 50
        out.append(rect(x, y, 105, 42, FILL_BLUE, BLUE, rx=4, width=1.4))
        out.append(mono(x + 52, y + 27, r, 17, INK, "bold", anchor="middle"))
    return out


def memory():
    out = [rect(900, 6, 200, 250, WHITE, BLUE, rx=10, width=1.6),
           text(1000, 34, "虚拟内存空间", 17, INK, "bold")]
    for k, s in enumerate(["代码", "全局变量", "栈"]):
        out.append(rect(920, 50 + k * 66, 160, 56, FILL_BLUE, LINE, rx=4, width=1.2))
        out.append(text(1000, 84 + k * 66, s, 16, INK))
    out.append(arrow(784, 104, 896, 104, INK, 2.2))
    out.append(text(840, 92, "地址", 14, MUTED))
    out.append(arrow(784, 172, 896, 172, INK, 2.2, both=True))
    out.append(text(840, 160, "数据", 14, MUTED))
    out.append(text(840, 208, "总线", 16, INK, "bold"))
    return out


def build():
    return cpu() + memory()


if __name__ == "__main__":
    save("visible-state", W, H, build())
