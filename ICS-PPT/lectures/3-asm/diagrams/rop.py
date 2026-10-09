#!/usr/bin/env python3
"""Return-oriented programming: ret chains pieces of existing code.

The top row is the stack, low addresses on the left. The overflow fills the
buffer and then writes three addresses: A over the return address, then B and
C above it. Each address is that of a gadget, a few instructions of the
existing code that end in ret. The function's ret pops A and jumps to gadget
A; the ret at the end of gadget A pops B; the ret of gadget B pops C. No
code is written to the stack.

Colours as in exploit.svg: stack data orange, what the input wrote red, code
blue. The three address slots are 8 bytes each and are drawn equal.
Run it to refresh ../assets/rop.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_RED, FONT, INK, MONO, MUTED, RED, WHITE,
                    arrow, label_line, mono, rect, save, text)

W, H = 1120, 262
Y, CH = 30, 44                      # the stack row
GY, GH = 146, 70                    # the gadgets
PAD_X, PAD_W = 60, 300
SW, GAP = 150, 60                   # one address slot / gadget, and the gap between gadgets
SX = PAD_X + PAD_W                  # first address slot

STEPS = [("bar", " 的 ret 弹出 A"), ("片段 A", " 的 ret 弹出 B"), ("片段 B", " 的 ret 弹出 C")]


def gx(k):
    """Left edge of gadget k; gadget 0 sits under slot 0."""
    return SX + k * (SW + GAP)


def build():
    out = [text(PAD_X, Y - 12, "低地址", 13, MUTED, anchor="start"),
           text(SX + 3 * SW, Y - 12, "高地址", 13, MUTED, anchor="end"),
           text(SX + SW / 2, Y - 12, "原来的返回地址", 13, BLUE, "bold")]
    out.append(rect(PAD_X, Y, PAD_W, CH, WHITE, RED, rx=2, width=1.6, dash="5 4"))
    out.append(text(PAD_X + PAD_W / 2, Y + 30, "填充（覆盖缓冲区）", 15, MUTED))
    for k, name in enumerate("ABC"):
        x = SX + k * SW
        out.append(rect(x, Y, SW, CH, FILL_RED, RED, rx=2, width=1.6))
        out += label_line(x + 22, Y + 30, [("片段 ", FONT, RED, "bold"),
                                           (name, MONO, RED, "bold"),
                                           (" 的地址", FONT, RED, "bold")], 15)
    out.append(text(SX + 3 * SW + 14, Y + 30, "输入写入栈中的数据", 15, RED, "bold",
                    anchor="start"))
    # the existing code and its three gadgets
    out.append(rect(SX - 40, GY - 22, 3 * SW + 2 * GAP + 80, GH + 58, FILL_BLUE, BLUE,
                    rx=8, width=1.4))
    out.append(text(SX - 24, GY + GH + 24, "程序中已有的代码（例如 libc）", 15, BLUE, "bold",
                    anchor="start"))
    for k, name in enumerate("ABC"):
        x = gx(k)
        out.append(rect(x, GY, SW, GH, WHITE, BLUE, rx=4, width=1.6))
        out += label_line(x + 12, GY + 22, [("片段 ", FONT, INK, "bold"),
                                            (name, MONO, INK, "bold")], 15)
        out.append(mono(x + 12, GY + 42, "...", 15, INK))
        out.append(mono(x + 12, GY + 61, "ret", 15, INK, "bold"))
        # the address in slot k leads to gadget k
        sx = SX + k * SW + SW / 2
        tx = x + SW / 2
        out.append(f'<path d="M {sx:.1f} {Y + CH + 2} L {sx:.1f} {Y + CH + 12} '
                   f'L {tx:.1f} {Y + CH + 30} L {tx:.1f} {GY - 3}" fill="none" '
                   f'stroke="{RED}" stroke-width="2.2" marker-end="url(#ah-{RED[1:]})"/>')
    for k, (who, what) in enumerate(STEPS):
        x = gx(k) + SW / 2 + 12
        mark = "①②③"[k]
        font = MONO if who == "bar" else FONT
        out += label_line(x, GY - 30,
                          [(mark + " ", FONT, RED, "bold"), (who, font, RED, "bold"),
                           (what, FONT, RED, "bold")], 13)
    return out


if __name__ == "__main__":
    save("rop", W, H, build(), left=-31)    # centres the ink on the canvas
