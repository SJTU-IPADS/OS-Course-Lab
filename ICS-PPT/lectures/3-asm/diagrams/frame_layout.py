#!/usr/bin/env python3
"""What one call puts on the stack, high addresses at the top.

Above the boundary, the end of the caller's frame: the arguments from the
seventh on, then the return address pushed by call. Below it, the callee's
frame: the callee-saved registers it pushed, then its local variables; %rsp is
the low edge. The circled numbers are the steps of the page's list that write
each region (2 push the arguments, 4 call, 5 pushq, 6 subq); the same regions
are released in the opposite order by steps 8, 9 and 10.

Colours as in the other stack figures: stack data orange, the return address
blue, saved registers green. Heights do not carry sizes here: how many
arguments, registers and bytes of locals there are depends on the function.
Tall and narrow, for the side column.
Run it to refresh ../assets/frame-layout.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, LINE, MUTED,
                    ORANGE, arrow, mono, rect, save, text, vbrace)

W, H = 470, 468
CX, CW = 92, 200                    # cell column: left edge, width
TOP = 62

# label, height, fill, stroke, the step that writes it
ROWS = [("调用者的其他数据", 96, FILL_ORANGE, ORANGE, ""),
        ("第 7 个起的参数", 64, FILL_ORANGE, ORANGE, "②"),
        ("返回地址", 44, FILL_BLUE, BLUE, "④"),
        ("保存的寄存器", 64, FILL_GREEN, GREEN, "⑤"),
        ("局部变量", 96, FILL_ORANGE, ORANGE, "⑥")]


def build():
    out = [text(W / 2, 30, "一次调用在栈上存放的数据（高地址在上）", 18, INK, "bold")]
    y = TOP
    edges = [y]
    for k, (name, h, fill, stroke, step) in enumerate(ROWS):
        out.append(rect(CX, y, CW, h, fill, stroke, rx=2, width=1.6))
        out.append(text(CX + CW / 2, y + h / 2 + 6, name, 17, MUTED if k == 0 else INK,
                        "normal" if k == 0 else "bold"))
        if step:
            out.append(text(CX - 22, y + h / 2 + 7, step, 19, ORANGE, "bold"))
        y += h
        edges.append(y)
    out += vbrace(CX + CW + 10, edges[0] + 2, edges[3] - 2, MUTED, "", 16)
    out.append(text(CX + CW + 28, (edges[0] + edges[3]) / 2 + 6, "调用者的栈帧", 16, MUTED,
                    "bold", anchor="start"))
    out += vbrace(CX + CW + 10, edges[3] + 2, edges[5] - 2, INK, "", 16)
    out.append(text(CX + CW + 28, (edges[3] + edges[5]) / 2 + 6, "被调用者的栈帧", 16, INK,
                    "bold", anchor="start"))
    out.append(arrow(W - 40, edges[5], CX + CW + 4, edges[5], INK, 2.2))
    out.append(mono(CX + CW + 28, edges[5] + 24, "%rsp", 16, INK, "bold"))
    # direction of addresses
    out.append(arrow(24, edges[5], 24, TOP + 6, LINE, 1.6))
    out.append(text(34, TOP + 16, "高地址", 14, MUTED, anchor="start"))
    out.append(text(34, edges[5] + 20, "低地址", 14, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("frame-layout", W, H, build())
