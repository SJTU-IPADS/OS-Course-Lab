#!/usr/bin/env python3
"""call and ret: where the two functions sit, and the stack at three moments.

Left, the address space, high addresses at the top, with main and dot_product
in the code segment. The call site is A from the return-address figure: call
at 0x4010, next instruction at 0x4015, which is main's `addq $40, %rsp` from
the stack-frame listing. dot_product's addresses (0x4100 entry, ret at
0x4130) are illustrative. The arrows beside the code segment are the two
transfers of control; circled numbers mark where %rip points at each of the
three moments. Right, the top of the stack before call, after call and
after ret, drawn as a close-up of the stack segment.
The figure sits in the side column, which reaches the right edge of the slide,
so the canvas keeps a blank margin on the right to line the drawing up with the
text margin.
Run it to refresh ../assets/call-ret.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, FONT, GREEN, INK,
                    LINE, MONO, MUTED, ORANGE, WHITE, arrow, label_line, line,
                    mono, path, rect, save, text)

W, H = 568, 628
AX, AW = 12, 236                    # address-space column: left edge, width
PX, PW = 270, 264                   # stack close-up panel: left edge, width
SX, SW, RH = 350, 134, 32           # stack close-up: left edge, width, slot height

# address space, top to bottom: name, height, fill, stroke, dashed
SEGS = [("运行时栈", 110, FILL_ORANGE, ORANGE, False),
        ("未使用", 120, WHITE, LINE, True),
        ("数据段与堆", 50, FILL_GREY, MUTED, False),
        ("", 292, FILL_BLUE, BLUE, False)]
TOP = 40
LH = 26                             # listing line height

# stack close-up: caption, row %rsp points at, the lower slot
STATES = [("①", "call", "之前", 0, None),
          ("②", "call", "之后", 1, "ret-addr"),
          ("③", "ret", "之后", 0, "read")]


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
            out.append(text(AX + 26, yy + 1, mark, 15, ORANGE, "bold"))
        out.append(mono(AX + 38, yy, addr, 13, MUTED))
        out.append(mono(AX + 92, yy, ins, 13, tone))
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
            bold = "normal" if dashed else "bold"
            out.append(text(AX + AW / 2, y + h / 2 + 6, name, 16,
                            MUTED if dashed else INK, bold))
        edges.append(y)
        y += h
    code = edges[3]
    out.append(text(AX + AW / 2, code + 22, "代码段", 16, INK, "bold"))
    shapes, dys, end = listing(code + 34, "dot_product:",
                               [("②", "0x4100", "（入口）", MUTED), ("", "", "…", MUTED),
                                ("", "0x4130", "ret", INK)])
    out += shapes
    shapes, mys, _ = listing(end + 14, "main:",
                             [("", "", "…", MUTED), ("①", "0x4010", "call dot_product", INK),
                              ("③", "0x4015", "addq $40, %rsp", INK)])
    out += shapes
    # the two transfers of control, beside the code segment
    right = AX + AW
    out.append(path(f"M {right - 6} {mys[1]} C {right + 50} {mys[1]}, "
                    f"{right + 50} {dys[0]}, {right - 4} {dys[0]}", BLUE, 2.2))
    out.append(mono(right + 44, dys[0] + 2, "call", 15, BLUE, "bold"))
    out.append(text(right + 44, dys[0] + 22, "跳到 dot_product", 14, BLUE, anchor="start"))
    out.append(path(f"M {right - 6} {dys[2]} C {right + 110} {dys[2]}, "
                    f"{right + 110} {mys[2]}, {right - 4} {mys[2]}", GREEN, 2.2, dash="6 4"))
    out.append(mono(right + 96, mys[2] - 8, "ret", 15, GREEN, "bold"))
    out.append(text(right + 96, mys[2] + 12, "回到 0x4015", 14, GREEN, anchor="start"))
    out.append(text(right + 96, (dys[2] + mys[1]) / 2 - 2, "①②③：各时刻", 14, ORANGE,
                    anchor="start"))
    out.append(mono(right + 96, (dys[2] + mys[1]) / 2 + 18, "%rip", 14, ORANGE, "bold"))
    out.append(text(right + 136, (dys[2] + mys[1]) / 2 + 18, "所指的指令", 14, ORANGE,
                    anchor="start"))
    return out, edges[0], edges[1]


def stack_state(y, k):
    num, op, when, rsp_row, low = STATES[k]
    out = label_line(PX + 14, y + 16, [(num + " 执行 ", FONT, INK, "bold"),
                                       (op, MONO, INK, "bold"),
                                       (" " + when, FONT, INK, "bold")], 15)
    top = y + 26
    out.append(rect(SX, top, SW, RH, FILL_GREY, LINE, rx=2))
    out.append(text(SX + SW / 2, top + 21, "调用方的栈数据", 14, MUTED))
    if low == "ret-addr":
        out.append(rect(SX, top + RH, SW, RH, FILL_ORANGE, ORANGE, rx=2, width=1.8))
        out += label_line(SX + 18, top + RH + 21, [("返回地址 ", FONT, INK, "bold"),
                                                  ("0x4015", MONO, INK, "bold")], 14)
    else:
        out.append(rect(SX, top + RH, SW, RH, WHITE, LINE, rx=2, dash="5 4"))
        if low == "read":
            out.append(text(SX + SW / 2, top + RH + 21, "0x4015（已读出）", 14, MUTED))
    ry = top + RH + rsp_row * RH    # %rsp is the low edge of the slot at the top
    out.append(arrow(PX + 54, ry, SX - 5, ry, INK, 2))
    out.append(mono(PX + 14, ry + 5, "%rsp", 14, INK, "bold"))
    if k == 0:
        out.append(text(SX + SW + 6, top + 12, "高地址", 13, MUTED, anchor="start"))
        out.append(text(SX + SW + 6, top + 2 * RH - 4, "低地址", 13, MUTED, anchor="start"))
    return out, top + 2 * RH


def build():
    out, stack_top, stack_bottom = address_space()
    top = TOP - 6
    panel = []
    y = top + 10
    for k in range(3):
        shapes, bottom = stack_state(y, k)
        panel += shapes
        y = bottom + 26
    # the close-up of the stack segment, joined to it
    out.append(rect(PX, top, PW, bottom + 12 - top, WHITE, ORANGE, rx=6, width=1.4))
    mid = (stack_top + stack_bottom) / 2
    out.append(line(AX + AW, mid, PX, mid, ORANGE, 1.4, dash="4 4"))
    return out + panel


if __name__ == "__main__":
    save("call-ret", W, H, build())
