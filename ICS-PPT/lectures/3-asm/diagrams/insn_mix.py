#!/usr/bin/env python3
"""The six instructions of the -O2 scalar loop body, and their split.

imull and addl do the multiply-add (1/3); movl, addq $4, cmpq and jne load,
step and control the loop (2/3). The ring has one segment per instruction.
Run it to refresh ../assets/insn-mix.svg.
"""

import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED,
                    ORANGE, WHITE, mono, rect, save, text)

W, H = 1120, 450

# name, is it part of the multiply-add
BODY = [("movl", False), ("imull", True), ("addq $4", False),
        ("addl", True), ("cmpq", False), ("jne", False)]
CX, CY, RO, RI = 290, 280, 140, 84


def sector(a0, a1, fill, stroke):
    """A ring segment from angle a0 to a1 (degrees, clockwise from 12 o'clock)."""
    def pt(r, a):
        t = math.radians(a - 90)
        return CX + r * math.cos(t), CY + r * math.sin(t)
    big = 1 if a1 - a0 > 180 else 0
    (x0, y0), (x1, y1) = pt(RO, a0), pt(RO, a1)
    (x2, y2), (x3, y3) = pt(RI, a1), pt(RI, a0)
    d = (f"M {x0:.1f} {y0:.1f} A {RO} {RO} 0 {big} 1 {x1:.1f} {y1:.1f} "
         f"L {x2:.1f} {y2:.1f} A {RI} {RI} 0 {big} 0 {x3:.1f} {y3:.1f} Z")
    return f'<path d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="1.6"/>'


def build():
    out = [text(20, 30, "-O2 标量循环体（共 6 条）", 17, INK, "bold", anchor="start")]
    cw = 1080 / 6
    for k, (name, mac) in enumerate(BODY):
        x = 20 + k * cw
        out.append(rect(x, 44, cw, 50, FILL_ORANGE if mac else FILL_GREY,
                        ORANGE if mac else MUTED, rx=2, width=1.6))
        out.append(mono(x + cw / 2, 75, name, 17, INK, "bold", anchor="middle"))
    # the ring: the two multiply-add instructions first, then the other four
    order = [n for n, m in BODY if m] + [n for n, m in BODY if not m]
    for k, name in enumerate(order):
        mac = k < 2
        out.append(sector(k * 60, (k + 1) * 60, FILL_ORANGE if mac else FILL_GREY,
                          ORANGE if mac else MUTED))
    out.append(text(CX, CY - 4, "6 条", 26, INK, "bold"))
    out.append(text(CX, CY + 22, "每轮循环", 14, MUTED))
    out.append(text(CX + 170, 170, "33%", 26, ORANGE, "bold", anchor="start"))
    out.append(text(CX - 170, 170, "67%", 26, MUTED, "bold", anchor="end"))
    # legend
    lx = 600
    rows = [(200, FILL_ORANGE, ORANGE, "乘加运算：2 条", ["imull（乘法）、addl（累加）"]),
            (290, FILL_GREY, MUTED, "访存与控制：4 条",
             ["movl（读内存）、addq $4（指针步进）、", "cmpq + jne（循环判断）"])]
    for y, fill, stroke, head, body in rows:
        out.append(rect(lx, y - 18, 22, 22, fill, stroke, rx=3, width=1.4))
        out.append(text(lx + 34, y, head, 18, INK, "bold", anchor="start"))
        for j, s in enumerate(body):
            out.append(text(lx + 34, y + 30 + j * 26, s, 15, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("insn-mix", W, H, build())
