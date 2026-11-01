#!/usr/bin/env python3
"""The address space of the running bomb, high addresses at the top, in the
layout of 3-asm's address-space figure with the read-only data added: the
instructions of phase_1 are in .text, the string it compares with is in
.rodata, the line read in is in the stack. The addresses are the file
addresses objdump shows (offsets from the address the program is loaded
at), taken from examples/bomb. Run it to refresh
../assets/bomb_address_space.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, arrow, mono, rect, save, text)

W, H = 450, 560
CX, CW = 112, 190                   # segment column: left edge, width
TOP = 56

# top to bottom: name, subtitle, height, fill, stroke, start address (file)
SEGS = [("内核", "", 40, FILL_GREY, MUTED, ""),
        ("运行时栈", "Stack", 90, FILL_ORANGE, ORANGE, ""),
        ("", "", 96, WHITE, LINE, ""),
        ("堆", "malloc", 46, FILL_GREY, MUTED, ""),
        ("数据段", ".data / .bss", 50, FILL_GREEN, GREEN, "0x103000"),
        ("只读数据", ".rodata", 62, FILL_GREEN, GREEN, "0xcb000"),
        ("代码段", ".text", 84, FILL_BLUE, BLUE, "0xa1c0")]


def build():
    out = [text(W / 2, 28, "bomb 进程的地址空间", 18, INK, "bold")]
    y = TOP
    edges = []
    for name, sub, h, fill, stroke, addr in SEGS:
        out.append(rect(CX, y, CW, h, fill, stroke, rx=2, width=1.6,
                        dash="6 4" if not name else None))
        mid = y + h / 2
        if name:
            out.append(text(CX + CW / 2, mid + (-3 if sub else 6), name, 16, INK, "bold"))
        if sub:
            out.append(mono(CX + CW / 2, mid + 17, sub, 13, MUTED, anchor="middle"))
        if addr:
            out.append(mono(CX - 8, y + h - 5, addr, 13, MUTED, anchor="end"))
        edges.append(y)
        y += h
    bottom = y
    gap, heap = edges[2], edges[3]
    out.append(arrow(CX + 40, gap + 6, CX + 40, gap + 46, ORANGE, 2.4))
    out.append(text(CX + 52, gap + 30, "向低地址生长", 13, ORANGE, "bold", anchor="start"))
    out.append(text(CX + CW / 2, gap + 68, "未使用的地址", 13, MUTED))
    # what the page reads, beside the segments it sits in
    stack_mid = (edges[1] + edges[2]) / 2
    out.append(text(CX + CW + 10, stack_mid - 4, "phase_1 的栈帧", 13, ORANGE, "bold",
                    anchor="start"))
    out.append(mono(CX + CW + 10, stack_mid + 14, "-0x8(%rbp)：line", 13, ORANGE,
                    anchor="start"))
    data_mid = (edges[4] + edges[5]) / 2
    out.append(mono(CX + CW + 10, data_mid - 4, "line", 14, GREEN, "bold", anchor="start"))
    out.append(text(CX + CW + 10, data_mid + 14, "读入的一行（.bss）", 13, GREEN,
                    anchor="start"))
    ro_mid = (edges[5] + edges[6]) / 2
    out.append(mono(CX + CW + 10, ro_mid - 4, "0xcb030", 14, GREEN, "bold", anchor="start"))
    out.append(text(CX + CW + 10, ro_mid + 14, "比较用的字符串", 13, GREEN, anchor="start"))
    text_mid = (edges[6] + bottom) / 2
    out.append(mono(CX + CW + 10, text_mid - 4, "0xab8d", 14, BLUE, "bold", anchor="start"))
    out.append(text(CX + CW + 10, text_mid + 14, "phase_1 的指令", 13, BLUE, anchor="start"))
    # direction of addresses
    out.append(text(CX - 8, TOP - 6, "高地址", 13, MUTED, anchor="end"))
    out.append(text(CX - 8, bottom + 16, "低地址", 13, MUTED, anchor="end"))
    return out


if __name__ == "__main__":
    save("bomb_address_space", W, H, build())
