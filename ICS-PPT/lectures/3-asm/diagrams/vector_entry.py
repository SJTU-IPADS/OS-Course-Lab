#!/usr/bin/env python3
"""The entry of dot_product in dot_avx2.s as a three-way split on n.

n <= 0 goes to .L8 (result 0); n - 1 <= 6, i.e. n < 8, goes to the scalar
loop at .L9; otherwise the entry computes the iteration count and byte
limit, zeroes the index and the accumulator, and enters the vector loop .L4.
Run it to refresh ../assets/vector-entry.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MONO, MUTED, ORANGE, WHITE, arrow, box, listing,
                    mono, rect, save, text)

W, H = 1120, 420


def test(x, y, w, cond, lines):
    out = [rect(x, y, w, 40 + 24 * len(lines), FILL_ORANGE, ORANGE, rx=6, width=1.8),
           text(x + w / 2, y + 28, cond, 18, INK, "bold")]
    for k, s in enumerate(lines):
        out.append(mono(x + w / 2, y + 56 + k * 24, s, 14, INK, anchor="middle"))
    return out


def build():
    out = box(16, 70, 150, 56, "n", FILL_GREY, MUTED, 20, sub="%edx，备份到 %r8d", sub_size=12)
    out.append(arrow(166, 98, 186, 98, INK, 2))
    out += test(190, 60, 220, "n ≤ 0 ?", ["testl %edx, %edx", "jle .L8"])
    out.append(arrow(410, 98, 446, 98, INK, 2))
    out.append(text(428, 88, "否", 14, MUTED, "bold"))
    out += test(450, 60, 250, "n − 1 ≤ 6 ?（n < 8）",
                ["leal -1(%rdx), %eax", "cmpl $6, %eax", "jbe .L9"])
    out.append(arrow(700, 98, 736, 98, INK, 2))
    out.append(text(718, 88, "否", 14, MUTED, "bold"))
    # the vector set-up
    out.append(rect(740, 60, 360, 168, FILL_BLUE, BLUE, rx=6, width=1.8))
    out.append(text(920, 88, "n ≥ 8：准备向量循环", 18, INK, "bold"))
    setup = [("shrl $3, %edx", "轮数 = n ÷ 8"), ("xorl %eax, %eax", "字节索引清零"),
             ("vpxor %xmm1, ...", "累加器清零"), ("salq $5, %rdx", "字节上限 = 轮数 × 32")]
    for k, (ins, what) in enumerate(setup):
        y = 118 + k * 28
        out.append(mono(756, y, ins, 14, INK))
        out.append(text(1086, y, what, 14, MUTED, anchor="end"))
    # the three exits
    out.append(arrow(300, 150, 300, 300, GREEN, 2.2))
    out.append(text(310, 220, "是", 14, GREEN, "bold", anchor="start"))
    out += box(200, 304, 200, 70, ".L8", WHITE, GREEN, 18, font=MONO, sub="结果清零，返回")
    out.append(arrow(575, 174, 575, 300, ORANGE, 2.2))
    out.append(text(585, 220, "是", 14, ORANGE, "bold", anchor="start"))
    out += box(475, 304, 200, 70, ".L9", WHITE, ORANGE, 18, font=MONO,
               sub="标量循环")
    out.append(arrow(920, 228, 920, 300, BLUE, 2.2))
    out += box(820, 304, 200, 70, ".L4", FILL_BLUE, BLUE, 18, font=MONO,
               sub="向量主循环")
    out.append(text(560, 30, "dot_avx2.s 入口：按 n 分三路", 18, INK, "bold"))
    return out


if __name__ == "__main__":
    save("vector-entry", W, H, build())
