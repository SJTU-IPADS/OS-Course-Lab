#!/usr/bin/env python3
"""Why the packed-addition trick has no counterpart in the matrix unit.

Left: two 4-bit numbers sharing one byte, so the low field's carry lands in the
high field — the situation the mask on the previous slide works around.
Right: the same 4-bit numbers fed to the multiply-accumulate unit one at a time,
with the running sum kept in a 32-bit register. Nothing shares a field, so
nothing can cross one.
Run it to refresh ../assets/int4-accumulate.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MONO,
                    MUTED, ORANGE, cells, line, rect, svg, text)

W, H = 920, 226
SPLIT = 452                                   # the divider between the panels

BIT, BH = 30, 26                              # one bit cell on the left
LX, LY = 56, 70                               # first bit of the first row
ROWS_Y = (LY, LY + 38, LY + 94)               # a, b, and the sum set apart

RX, RY = 500, 74                              # the operand column on the right
OP, OH = 44, 30
GAP = 22                                      # room for the × between a and b
ACC_Y, ACC_H = 184, 32


def panel_left():
    """Two 4-bit fields in one byte, and the carry that leaves the low one."""
    out = [text(LX + 4 * BIT, 30, "软件：两个 4 位数共用一个字节", 14, INK, "bold")]
    rows = [("a", "00101001", "高 2 · 低 9"), ("b", "00111000", "高 3 · 低 8"),
            ("a + b", "01100001", "高 6，应为 5")]
    for y, (name, bits, note) in zip(ROWS_Y, rows):
        out.append(text(LX - 8, y + BH / 2 + 4, name, 12.5, MUTED, anchor="end",
                        font=MONO))
        hi, _ = cells(LX, y, list(bits[:4]), BIT, BH, FILL_BLUE, BLUE)
        lo, end = cells(LX + 4 * BIT, y, list(bits[4:]), BIT, BH, FILL_ORANGE,
                        ORANGE)
        out += hi + lo
        out.append(text(end + 10, y + BH / 2 + 4, note, 12.5, MUTED, anchor="start"))

    # the boundary the carry crosses, and the arrow that crosses it
    bx = LX + 4 * BIT
    out.append(text(bx, 56, "字段边界", 12.5, ORANGE, "bold"))
    out.append(line(bx, 62, bx, ROWS_Y[2] + BH + 8, ORANGE, 1.4, "4 3"))
    ay = ROWS_Y[2] - 6
    out.append(f'<path d="M {bx + 22:.1f} {ay:.1f} C {bx + 12:.1f} {ay - 24:.1f} '
               f'{bx - 12:.1f} {ay - 24:.1f} {bx - 22:.1f} {ay:.1f}" fill="none" '
               f'stroke="{ORANGE}" stroke-width="1.8"/>')
    out.append(f'<path d="M {bx - 22:.1f} {ay:.1f} l 5 -9 l 4 9 z" fill="{ORANGE}"/>')
    out.append(text(LX + 4 * BIT, ROWS_Y[2] + BH + 26,
                    "低字段 9+8 的进位加进了高字段", 12.5, MUTED))
    return out


def panel_right():
    """One 4-bit pair at a time, and the 32-bit register holding the sum."""
    out = [text(RX + 170, 30, "硬件：4 位输入，32 位累加器", 14, INK, "bold"),
           text(RX + 170, 56, "每个 4 位数单独取出，各自参与乘法", 12.5, MUTED)]

    pairs = [("a₀", "b₀"), ("a₁", "b₁"), ("…", "…"), ("a₃₁", "b₃₁")]
    step = 96
    for i, (a, b) in enumerate(pairs):
        x = RX + i * step
        for j, v in enumerate((a, b)):
            cy = RY + j * (OH + GAP)
            out.append(rect(x, cy, OP, OH, FILL_BLUE, BLUE))
            out.append(text(x + OP / 2, cy + OH / 2 + 5, v, 13, INK))
        out.append(text(x + OP / 2, RY + OH + 17, "×", 15, MUTED))
        # each product drops into the accumulator below
        out.append(line(x + OP / 2, RY + 2 * OH + GAP + 4, x + OP / 2, ACC_Y - 5,
                        LINE, 1.4, "4 4"))
    acc_w = 3 * step + OP
    out.append(rect(RX, ACC_Y, acc_w, ACC_H, FILL_GREY, INK, width=1.8))
    out.append(text(RX + acc_w / 2, ACC_Y + ACC_H / 2 + 5,
                    "累加器 accumulator · 32 位寄存器", 13.5, INK, "bold"))
    return out


def build():
    body = panel_left() + panel_right()
    body.append(line(SPLIT, 20, SPLIT, H - 16, LINE, 1.2, "5 5"))
    return svg(W, H, body)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "int4-accumulate.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
