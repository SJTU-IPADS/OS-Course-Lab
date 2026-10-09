#!/usr/bin/env python3
"""main's frame drawn as a stack, high addresses at the top.

Same frame as main_frame.py (gcc -Og -fcf-protection=none -fno-stack-protector):
after `subq $40, %rsp`, x[0..3] at (%rsp)..12(%rsp), w[0..3] at 16..28(%rsp),
32..39 unused. What lies at 40(%rsp) and above was on the stack before main
started and is not named here.

Every row is 8 bytes and all rows have one height. An int is 4 bytes, so a
row holds two array elements, the higher address on the left as in CS:APP's
stack figures: addresses fall from left to right and from top to bottom, and
%rsp, drawn at the lower right, is the address of the bottom right cell. Each
cell carries its address as the listing writes it. The stack above the frame has no stated
size and is drawn taller than a row. Tall and narrow, for the side column
next to the listing.
Run it to refresh ../assets/main-stack.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE, arrow, mono, rect,
                    save, text, vbrace)

W, H = 540, 544
CX, CW = 92, 240                    # cell column: left edge, width
TOP, OLD, RH = 60, 92, 70           # stack top edge, old stack, one 8-byte row
ROWS = (32, 24, 16, 8, 0)           # offset of each row from %rsp, top to bottom


def address(off):
    return f"{off}(%rsp)" if off else "(%rsp)"


def cell(x, y, w, off, label):
    """One int, or the unused 8 bytes: its address above what it holds."""
    used = label.isascii()
    out = [rect(x, y, w, RH, FILL_ORANGE if used else WHITE, ORANGE if used else LINE,
                rx=2, width=1.4 if used else 1.6, dash=None if used else "5 4")]
    out.append(mono(x + w / 2, y + 25, address(off), 15, INK, anchor="middle"))
    if used:
        out.append(mono(x + w / 2, y + 53, label, 19, INK, "bold", anchor="middle"))
    else:
        out.append(text(x + w / 2, y + 52, label, 16, MUTED))
    return out


def build():
    out = [text(W / 2, 30, "main 的栈帧（每行 8 字节，高地址在上）", 19, INK, "bold")]
    # the stack before main started
    out.append(rect(CX, TOP, CW, OLD, FILL_ORANGE, ORANGE, rx=2, width=1.4))
    out.append(text(CX + CW / 2, TOP + OLD / 2 + 6, "进入 main 之前的栈", 16, MUTED))
    entry = TOP + OLD
    for i, base in enumerate(ROWS):
        y = entry + i * RH
        if base == 32:              # 32..39, not written by main
            out += cell(CX, y, CW, base, "未使用")
            continue
        for half in (0, 1):         # two ints, the higher address on the left
            k = base // 4 + 1 - half
            label = f"w[{k - 4}] = {k - 3}" if k >= 4 else f"x[{k}] = {k + 5}"
            out += cell(CX + half * CW / 2, y, CW / 2, 4 * k, label)
    bottom = entry + len(ROWS) * RH
    # the frame and where %rsp points
    out += vbrace(CX + CW + 10, entry + 2, bottom - 2, INK, "", 16)
    out.append(text(CX + CW + 30, (entry + bottom) / 2 - 4, "main 的栈帧", 17, INK,
                    "bold", anchor="start"))
    out.append(text(CX + CW + 30, (entry + bottom) / 2 + 22, "40 字节", 17, INK,
                    "bold", anchor="start"))
    out.append(arrow(W - 18, entry, CX + CW + 22, entry, LINE, 2.2, dash="5 4"))
    out.append(text(W - 18, entry - 10, "进入 main 时的 %rsp", 16, MUTED, "bold",
                    anchor="end"))
    out.append(arrow(W - 18, bottom, CX + CW + 22, bottom, INK, 2.2))
    out.append(text(W - 18, bottom + 24, "subq 之后的 %rsp", 16, INK, "bold",
                    anchor="end"))
    # direction of addresses
    out.append(arrow(24, bottom, 24, TOP + 6, MUTED, 1.8))
    out.append(text(34, TOP + 16, "高地址", 14, MUTED, anchor="start"))
    out.append(text(34, bottom + 20, "低地址", 14, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("main-stack", W, H, build())
