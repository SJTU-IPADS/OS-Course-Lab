#!/usr/bin/env python3
"""main's frame drawn as a stack, high addresses at the top.

Same frame as main_frame.py (gcc -Og -fcf-protection=none -fno-stack-protector):
after `subq $40, %rsp`, x[0..3] at (%rsp)..12(%rsp), w[0..3] at 16..28(%rsp),
32..39 unused. What lies at 40(%rsp) and above was on the stack before main
started and is not named here. Each int is one row, labelled with its address
as the listing writes it. Tall and narrow, for the side column next to the
listing.
Run it to refresh ../assets/main-stack.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE, arrow, mono, rect,
                    save, text, vbrace)

W, H = 560, 600
CX, CW = 128, 200                   # cell column: left edge, width
TOP, OLD, GAP, RH = 64, 56, 60, 46  # stack top edge, old-stack, unused, int row


def build():
    out = [text(W / 2, 30, "main 的栈帧（高地址在上）", 19, INK, "bold")]
    # the stack before main started
    out.append(rect(CX, TOP, CW, OLD, FILL_ORANGE, ORANGE, rx=2, width=1.4))
    out.append(text(CX + CW / 2, TOP + OLD / 2 + 6, "进入 main 之前的栈", 16, MUTED))
    y = TOP + OLD
    entry = y
    # the unused 8 bytes at 32..39
    out.append(rect(CX, y, CW, GAP, WHITE, LINE, rx=2, width=1.6, dash="5 4"))
    out.append(text(CX + CW / 2, y + GAP / 2 + 6, "未使用（8 字节）", 16, MUTED))
    out.append(mono(CX - 12, y + GAP / 2 + 6, "32(%rsp)", 15, MUTED, anchor="end"))
    y += GAP
    # one row per int, w[3] at the top, x[0] at the bottom
    for k in range(7, -1, -1):
        name, value = (f"w[{k - 4}]", k - 3) if k >= 4 else (f"x[{k}]", k + 5)
        out.append(rect(CX, y, CW, RH, FILL_ORANGE, ORANGE, rx=2, width=1.4))
        out.append(mono(CX + CW / 2, y + RH / 2 + 7, f"{name} = {value}", 19, INK, "bold",
                        anchor="middle"))
        addr = f"{4 * k}(%rsp)" if k else "(%rsp)"
        out.append(mono(CX - 12, y + RH / 2 + 6, addr, 15, INK, anchor="end"))
        y += RH
    bottom = y
    # the frame and where %rsp points
    out += vbrace(CX + CW + 10, entry + 2, bottom - 2, INK, "", 16)
    out.append(text(CX + CW + 30, (entry + bottom) / 2 - 4, "main 的栈帧", 17, INK,
                    "bold", anchor="start"))
    out.append(text(CX + CW + 30, (entry + bottom) / 2 + 22, "40 字节", 17, INK,
                    "bold", anchor="start"))
    out.append(arrow(W - 30, entry, CX + CW + 22, entry, LINE, 2.2, dash="5 4"))
    out.append(text(W - 30, entry - 10, "进入 main 时的 %rsp", 16, MUTED, "bold",
                    anchor="end"))
    out.append(arrow(W - 30, bottom, CX + CW + 22, bottom, INK, 2.2))
    out.append(text(W - 30, bottom + 24, "subq 之后的 %rsp", 16, INK, "bold",
                    anchor="end"))
    # direction of addresses
    out.append(arrow(24, bottom, 24, TOP + 6, MUTED, 1.8))
    out.append(text(34, TOP + 6, "高地址", 14, MUTED, anchor="start"))
    out.append(text(34, bottom + 20, "低地址", 14, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("main-stack", W, H, build())
