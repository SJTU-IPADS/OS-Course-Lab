#!/usr/bin/env python3
"""A process's address space, high addresses at the top.

The stack sits at the high end and grows toward lower addresses; %rsp marks
its top. Code and data sit at the low end, the heap above them, growing toward
higher addresses. Tall and narrow, for the side column next to the text.
Run it to refresh ../assets/address-space.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, arrow, mono, rect, save, text)

W, H = 370, 624
CX, CW = 24, 210                    # segment column: left edge, width

# top to bottom: name, subtitle, height, fill, stroke
SEGS = [("内核", "", 50, FILL_GREY, MUTED),
        ("运行时栈", "Stack", 110, FILL_ORANGE, ORANGE),
        ("", "", 180, WHITE, LINE),
        ("堆", "malloc", 70, FILL_GREY, MUTED),
        ("数据段", ".data / .bss", 70, FILL_GREEN, GREEN),
        ("代码段", ".text", 70, FILL_BLUE, BLUE)]
TOP = 60


def build():
    out = [text(W / 2, 30, "地址空间（高地址在上）", 19, INK, "bold")]
    y = TOP
    edges = []
    for name, sub, h, fill, stroke in SEGS:
        out.append(rect(CX, y, CW, h, fill, stroke, rx=2, width=1.6,
                        dash="6 4" if not name else None))
        mid = y + h / 2
        if name:
            out.append(text(CX + CW / 2, mid + (-4 if sub else 6), name, 17, INK, "bold"))
        if sub:
            out.append(mono(CX + CW / 2, mid + 20, sub, 14, MUTED, anchor="middle"))
        edges.append(y)
        y += h
    bottom = y
    gap, heap = edges[2], edges[3]
    # growth of the stack and of the heap, inside the unused range
    out.append(arrow(CX + 50, gap + 6, CX + 50, gap + 66, ORANGE, 2.6))
    out.append(text(CX + 64, gap + 42, "向低地址生长", 15, ORANGE, "bold", anchor="start"))
    out.append(text(CX + CW / 2, gap + 102, "未使用的地址", 15, MUTED))
    out.append(arrow(CX + 50, heap - 4, CX + 50, heap - 50, MUTED, 2, "5 4"))
    out.append(text(CX + 64, heap - 20, "向高地址生长", 15, MUTED, anchor="start"))
    # %rsp marks the lowest address the stack uses
    out.append(arrow(W - 24, gap, CX + CW + 8, gap, INK, 2.2))
    out.append(mono(W - 24, gap - 10, "%rsp：栈顶", 16, INK, "bold", anchor="end"))
    # direction of addresses
    out.append(text(CX + CW + 10, TOP + 16, "高地址", 14, MUTED, anchor="start"))
    out.append(text(CX + CW + 10, bottom - 6, "低地址", 14, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("address-space", W, H, build())
