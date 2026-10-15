#!/usr/bin/env python3
"""The rop.c chain: padding, then two addresses with the integer 42 between them.

vuln's ret pops the first address and lands on gadget_pop. pop %rdi consumes
42. gadget_pop's ret pops the address of print_num, which prints that 42.
The middle cell is orange because it is a value, not an address.
Run it to refresh ../assets/rop-demo.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_ORANGE, FILL_RED, FONT, INK, MONO, MUTED, ORANGE,
                    RED, WHITE, arrow, elbow, label_line, mono, rect, save, text)

W, H = 1120, 300
Y, CH = 26, 48
PAD_X, PAD_W = 36, 240
SW, GAP = 230, 16
SX = PAD_X + PAD_W + GAP          # first of the three cells the input writes
GY, GH = 168, 86


def slot(k):
    return SX + k * (SW + GAP)


def build():
    out = [text(PAD_X, Y - 10, "低地址", 13, MUTED, anchor="start"),
           text(slot(2) + SW, Y - 10, "高地址", 13, MUTED, anchor="end")]

    out.append(rect(PAD_X, Y, PAD_W, CH, WHITE, RED, rx=2, width=1.6, dash="5 4"))
    out.append(text(PAD_X + PAD_W / 2, Y + 30, "24 个 A", 16, MUTED))

    cells = [(0, FILL_RED, RED, "gadget_pop 的地址"),
             (1, FILL_ORANGE, ORANGE, "整数 42"),
             (2, FILL_RED, RED, "print_num 的地址")]
    for k, fill, stroke, name in cells:
        x = slot(k)
        out.append(rect(x, Y, SW, CH, fill, stroke, rx=2, width=2.2))
        out.append(text(x + SW / 2, Y + 30, name, 16, stroke, "bold"))

    # the two pieces of code already in the program
    out.append(rect(slot(0), GY, SW, GH, WHITE, BLUE, rx=4, width=1.8))
    out.append(mono(slot(0) + 14, GY + 24, "gadget_pop:", 15, BLUE, "bold"))
    out.append(mono(slot(0) + 14, GY + 48, "pop %rdi", 16, INK))
    out.append(mono(slot(0) + 14, GY + 72, "ret", 16, INK, "bold"))

    out.append(rect(slot(2), GY, SW, GH, WHITE, BLUE, rx=4, width=1.8))
    out.append(mono(slot(2) + 14, GY + 24, "print_num:", 15, BLUE, "bold"))
    out.append(mono(slot(2) + 14, GY + 48, r'printf("%ld\n", n)', 15, INK))
    out.append(mono(slot(2) + 14, GY + 72, "exit(0)", 16, INK))

    # ① vuln's ret pops the first address and lands on gadget_pop
    out.append(arrow(slot(0) + SW / 2, Y + CH + 2, slot(0) + SW / 2, GY - 2, RED, 2.2))
    out.append(text(slot(0) + SW / 2 + 8, (Y + CH + GY) / 2 + 4, "①", 15, RED, "bold",
                    anchor="start"))

    # ③ gadget_pop's ret pops the address of print_num
    out.append(arrow(slot(2) + SW / 2, Y + CH + 2, slot(2) + SW / 2, GY - 2, RED, 2.2))
    out.append(text(slot(2) + SW / 2 + 8, (Y + CH + GY) / 2 + 4, "③", 15, RED, "bold",
                    anchor="start"))

    # ② pop %rdi takes 42, so the arrow turns into that instruction
    cx = slot(1) + SW / 2
    out.append(elbow([(cx, Y + CH + 2), (cx, GY + 48), (slot(0) + SW + 6, GY + 48)], ORANGE, 2.2))

    out += label_line(PAD_X, GY + GH + 28,
                      [("① ", FONT, RED, "bold"),
                       ("vuln", MONO, RED, "bold"),
                       (" 的 ret 跳到 gadget_pop　　", FONT, RED, "bold"),
                       ("② ", FONT, ORANGE, "bold"),
                       ("pop %rdi", MONO, ORANGE, "bold"),
                       (" 取走 42　　", FONT, ORANGE, "bold"),
                       ("③ ", FONT, RED, "bold"),
                       ("ret", MONO, RED, "bold"),
                       (" 跳到 print_num，打印 42", FONT, RED, "bold")], 15)
    return out


if __name__ == "__main__":
    save("rop-demo", W, H, build())
