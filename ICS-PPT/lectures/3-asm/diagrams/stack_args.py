#!/usr/bin/env python3
"""The stack on entry to last2, called by use8 with eight arguments.

examples/args8.c (gcc -Og -fcf-protection=none): use8 executes pushq $8,
pushq $7, loads the first six arguments into registers and calls last2. On
entry to last2 the return address is at (%rsp), argument 7 at 8(%rsp) and
argument 8 at 16(%rsp); last2 reads them as `movq 8(%rsp), %rax` and
`subq 16(%rsp), %rax`. Each slot is labelled on the left with its address as
last2 writes it and on the right with the instruction of use8 that wrote it.
All three slots belong to use8's frame.

Colours as in the other stack figures: stack data orange, the return address
blue. Tall and narrow, for the side column.
Run it to refresh ../assets/stack-args.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, FONT, INK, LINE, MONO, MUTED, ORANGE,
                    arrow, label_line, mono, rect, save, text, vbrace)

W, H = 540, 430
CX, CW = 112, 190                   # cell column: left edge, width
TOP, DATA, SLOT = 62, 130, 48       # top edge, use8's other data, one 8-byte slot

SLOTS = [("参数 8：8", "16(%rsp)", FILL_ORANGE, ORANGE, "pushq $8"),
         ("参数 7：7", "8(%rsp)", FILL_ORANGE, ORANGE, "pushq $7"),
         ("返回地址", "(%rsp)", FILL_BLUE, BLUE, "call last2")]


def build():
    out = [text(W / 2, 30, "进入 last2 时的栈（高地址在上）", 18, INK, "bold")]
    out.append(rect(CX, TOP, CW, DATA, FILL_ORANGE, ORANGE, rx=2, width=1.4))
    out += label_line(CX + 36, TOP + DATA / 2 + 6, [("use8", MONO, MUTED, "normal"),
                                                    (" 的其他数据", FONT, MUTED, "normal")], 16)
    y = TOP + DATA
    for name, addr, fill, stroke, wrote in SLOTS:
        out.append(rect(CX, y, CW, SLOT, fill, stroke, rx=2, width=1.6))
        out.append(text(CX + CW / 2, y + SLOT / 2 + 6, name, 17, INK, "bold"))
        out.append(mono(CX - 12, y + SLOT / 2 + 5, addr, 15, INK, anchor="end"))
        out.append(mono(CX + CW + 34, y + SLOT / 2 + 5, wrote, 15, INK))
        y += SLOT
    bottom = y
    out += vbrace(CX + CW + 8, TOP + 2, bottom - 2, MUTED, "", 16)
    out += label_line(CX + CW + 34, TOP + DATA / 2 + 6, [("use8", MONO, MUTED, "bold"),
                                                         (" 的栈帧", FONT, MUTED, "bold")], 16)
    out.append(arrow(W - 60, bottom, CX + CW + 4, bottom, INK, 2.2))
    out += label_line(CX + CW + 34, bottom + 26, [("进入 ", FONT, INK, "bold"),
                                                  ("last2", MONO, INK, "bold"),
                                                  (" 时的 ", FONT, INK, "bold"),
                                                  ("%rsp", MONO, INK, "bold")], 15)
    # direction of addresses
    out.append(arrow(24, bottom, 24, TOP + 6, LINE, 1.6))
    out.append(text(34, TOP + 16, "高地址", 14, MUTED, anchor="start"))
    out.append(text(34, bottom + 22, "低地址", 14, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("stack-args", W, H, build())
