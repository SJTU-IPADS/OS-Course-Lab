#!/usr/bin/env python3
"""call and ret: where the two functions sit, and the stack at three moments.

Left, the address space, high addresses at the top, with the segment colours
of address_space.py: the stack orange, the heap grey, the data segment green,
the code segment blue. main and dot_product are listings in the code segment.
The addresses are those of the program the page's demo builds
(gcc 15.2 -Og -fcf-protection=none -fno-stack-protector -no-pie main.c dot.c,
read with objdump -d): call at 0x401156, next instruction at 0x40115b, which
is main's `addq $40, %rsp`; dot_product from 0x401160 to its ret at 0x401199.
Rebuild with another toolchain and they move; ADDR below is the one place to
change. The arrows beside the code segment are the two transfers of control;
circled numbers mark where %rip points at each of the three moments.

Right, a close-up of the stack segment, joined to it by dashed guides from the
top and bottom edges of the stack box: the top of the stack before call,
after call and after ret. The caller's data is a tall orange region (the
stack's colour); the return address is one 8-byte slot in the code segment's
blue, since it is an address in the code segment.

The figure sits in the side column, which reaches the right edge of the slide,
so the canvas keeps a blank margin on the right to line the drawing up with the
text margin. Run it to refresh ../assets/call-ret.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, FONT,
                    GREEN, INK, LINE, MONO, MUTED, ORANGE, WHITE, arrow,
                    label_line, line, mono, path, rect, save, text)

W, H = 568, 628
AX, AW = 8, 236                     # address-space column: left edge, width
PX, PW = 268, 266                   # stack close-up panel: left edge, width
SX, SW = 316, 210                   # stack close-up: slot left edge, width
DATA, SLOT = 76, 20                 # caller's data region, the 8-byte slot
SNAP = 118                          # height of one snapshot

# address space, top to bottom: name, height, fill, stroke, dashed
SEGS = [("运行时栈", 110, FILL_ORANGE, ORANGE, False),
        ("未使用", 122, WHITE, LINE, True),
        ("堆", 40, FILL_GREY, MUTED, False),
        ("数据段", 40, FILL_GREEN, GREEN, False),
        ("", 270, FILL_BLUE, BLUE, False)]
TOP = 40
LH = 24                             # listing line height
# call, the instruction after it, dot_product's entry and its ret
ADDR = {"call": 0x401156, "next": 0x40115b, "entry": 0x401160, "ret": 0x401199}
RETADDR = f"0x{ADDR['next']:x}"

# stack close-up: number, instruction, when, slot state (None, "ret-addr", "read")
STATES = [("①", "call", "之前", None),
          ("②", "call", "之后", "ret-addr"),
          ("③", "ret", "之后", "read")]


def listing(y, name, rows):
    """A function's box in the code segment. Returns shapes and each row's y.

    A row is (mark, address, instruction, tone); the mark is the circled number
    of the moment at which %rip points at that row.
    """
    h = LH * (len(rows) + 1) + 12
    out = [rect(AX + 10, y, AW - 20, h, WHITE, BLUE, rx=3, width=1.2),
           mono(AX + 20, y + 22, name, 14, INK, "bold")]
    ys = []
    for i, (mark, addr, ins, tone) in enumerate(rows):
        yy = y + 22 + LH * (i + 1)
        if mark:
            out.append(text(AX + 24, yy + 1, mark, 15, ORANGE, "bold"))
        out.append(mono(AX + 36, yy, addr, 13, MUTED))
        out.append(mono(AX + 90, yy, ins, 13, tone))
        ys.append(yy - 5)
    return out, ys, y + h


def address_space():
    out = [text(AX + AW / 2, 24, "地址空间（高地址在上）", 16, INK, "bold")]
    y = TOP
    edges = []
    for name, h, fill, stroke, dashed in SEGS:
        out.append(rect(AX, y, AW, h, fill, stroke, rx=2, width=1.6,
                        dash="6 4" if dashed else None))
        if name:
            out.append(text(AX + AW / 2, y + h / 2 + 6, name, 16,
                            MUTED if dashed else INK, "normal" if dashed else "bold"))
        edges.append(y)
        y += h
    code = edges[4]
    out.append(text(AX + AW / 2, code + 22, "代码段", 16, INK, "bold"))
    shapes, dys, end = listing(code + 34, "dot_product:",
                               [("②", f"{ADDR['entry']:x}", "（入口）", MUTED),
                                ("", "", "…", MUTED),
                                ("", f"{ADDR['ret']:x}", "ret", INK)])
    out += shapes
    shapes, mys, _ = listing(end + 14, "main:",
                             [("", "", "…", MUTED),
                              ("①", f"{ADDR['call']:x}", "call dot_product", INK),
                              ("③", f"{ADDR['next']:x}", "addq $40, %rsp", INK)])
    out += shapes
    # the two transfers of control, beside the code segment
    right = AX + AW
    out.append(path(f"M {right - 6} {mys[1]} C {right + 50} {mys[1]}, "
                    f"{right + 50} {dys[0]}, {right - 4} {dys[0]}", BLUE, 2.2))
    out.append(mono(right + 46, dys[0] + 28, "call", 15, BLUE, "bold"))
    out.append(text(right + 46, dys[0] + 48, "跳到 dot_product", 14, BLUE, anchor="start"))
    out.append(path(f"M {right - 6} {dys[2]} C {right + 110} {dys[2]}, "
                    f"{right + 110} {mys[2]}, {right - 4} {mys[2]}", GREEN, 2.2, dash="6 4"))
    out.append(mono(right + 96, mys[2] - 8, "ret", 15, GREEN, "bold"))
    out.append(text(right + 96, mys[2] + 12, f"回到 {RETADDR}", 14, GREEN, anchor="start"))
    mid = (dys[2] + mys[1]) / 2
    out.append(text(right + 100, mid - 2, "①②③：各时刻", 14, ORANGE, anchor="start"))
    out.append(mono(right + 100, mid + 18, "%rip", 14, ORANGE, "bold"))
    out.append(text(right + 140, mid + 18, "所指的指令", 14, ORANGE, anchor="start"))
    return out, edges[0], edges[1]


def snapshot(y, k):
    """One moment: the caller's data, the slot below it, %rsp. Returns shapes."""
    num, op, when, state = STATES[k]
    out = [text(PX + 10, y + 14, num, 15, ORANGE, "bold", anchor="start")]
    out += label_line(PX + 28, y + 14, [("执行 ", FONT, INK, "bold"),
                                        (op, MONO, INK, "bold"),
                                        (" " + when, FONT, INK, "bold")], 15)
    top = y + 22
    out.append(rect(SX, top, SW, DATA, FILL_ORANGE, ORANGE, rx=2, width=1.6))
    out.append(text(SX + SW / 2, top + DATA / 2 + 5, "调用者的栈数据", 14, INK))
    slot = top + DATA
    if state == "ret-addr":
        out.append(rect(SX, slot, SW, SLOT, FILL_BLUE, BLUE, rx=2, width=1.8))
        out += label_line(SX + 34, slot + 15, [("返回地址 ", FONT, INK, "bold"),
                                               (RETADDR, MONO, INK, "bold")], 14)
    else:
        out.append(rect(SX, slot, SW, SLOT, WHITE, LINE, rx=2, dash="5 4"))
        if state == "read":
            out.append(text(SX + SW / 2, slot + 15, f"{RETADDR}（已读出，已释放）", 14, MUTED))
    # %rsp is the low edge of the slot at the top of the stack
    ry = slot + SLOT if state == "ret-addr" else slot
    out.append(arrow(PX + 8, ry, SX - 4, ry, INK, 2))
    out.append(mono(PX + 8, ry - 8, "%rsp", 14, INK, "bold"))
    if k == 0:
        out.append(text(PX + 8, top + 11, "高地址", 13, MUTED, anchor="start"))
        out.append(text(PX + 8, slot + SLOT - 2, "低地址", 13, MUTED, anchor="start"))
    return out


def build():
    out, stack_top, stack_bottom = address_space()
    top = TOP - 6
    panel = [text(PX + PW / 2, top + 20, "运行时栈的顶部（放大）", 14, INK, "bold")]
    y = top + 28
    for k in range(3):
        panel += snapshot(y, k)
        y += SNAP + 10
    bottom = y - 10 + 6
    # the close-up of the stack segment, joined to its top and bottom edges
    out.append(rect(PX, top, PW, bottom - top, WHITE, ORANGE, rx=6, width=1.4))
    out.append(line(AX + AW, stack_top, PX, top, ORANGE, 1.4, dash="4 4"))
    out.append(line(AX + AW, stack_bottom, PX, bottom, ORANGE, 1.4, dash="4 4"))
    return out + panel


if __name__ == "__main__":
    save("call-ret", W, H, build())
