#!/usr/bin/env python3
"""The stack canary in main's frame and the three steps that use it.

Offsets are main.s's (gcc -Og -fcf-protection=none): the arrays at 0..31, the
canary at 40(%rsp), the return address at 56(%rsp). An overflow of the arrays
writes toward higher addresses and reaches the canary before the return
address; the exit check then fails.
Run it to refresh ../assets/canary.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, FILL_RED, INK, LINE,
                    MONO, MUTED, ORANGE, RED, WHITE, arrow, box, mono, path, rect,
                    save, text)

W, H = 1120, 320
X0, B = 48, 16
Y, CH = 96, 56


def bx(off):
    return X0 + off * B


def build():
    out = []
    parts = [(0, 32, "局部数组 w, x（0 ~ 31）", FILL_BLUE, BLUE, INK, None),
             (32, 8, "填充", WHITE, LINE, MUTED, "5 4"),
             (40, 8, "金丝雀值", FILL_RED, RED, RED, None),
             (48, 8, "填充", WHITE, LINE, MUTED, "5 4"),
             (56, 8, "返回地址", FILL_ORANGE, ORANGE, INK, None)]
    for off, n, name, fill, stroke, color, dash in parts:
        w = n * B
        out.append(rect(bx(off), Y, w, CH, fill, stroke, rx=2,
                        width=3 if off == 40 else 1.6, dash=dash))
        out.append(text(bx(off) + w / 2, Y + 35, name, 16, color,
                        "normal" if dash else "bold"))
    for off, s in [(40, "40(%rsp)"), (56, "56(%rsp)")]:
        out.append(mono(bx(off) + 4 * B, Y + CH + 20, s, 14, MUTED, anchor="middle"))
    out.append(text(bx(0), Y - 12, "低地址", 14, MUTED, anchor="start"))
    out.append(text(bx(64), Y - 12, "高地址", 14, MUTED, anchor="end"))
    # the overflow runs toward higher addresses and hits the canary first
    out.append(path(f"M {bx(20)} {Y - 6} C {bx(26)} {Y - 50} {bx(38)} {Y - 50} "
                    f"{bx(44)} {Y - 6}", RED, 3))
    out.append(text(bx(32), Y - 50, "越界写入：先改写金丝雀值，才能到达返回地址", 16, RED,
                    "bold"))
    # the three steps
    steps = [("① 入口写入", ["movq %fs:40, %rax", "movq %rax, 40(%rsp)"]),
             ("② 返回前检验", ["movq 40(%rsp), %rdx", "subq %fs:40, %rdx"]),
             ("③ 不相等则终止", ["jne .L4", "call __stack_chk_fail@PLT"])]
    for k, (title, lines) in enumerate(steps):
        x = 40 + k * 360
        out.append(rect(x, 206, 320, 100, WHITE, RED if k == 2 else LINE, rx=6, width=1.6))
        out.append(text(x + 16, 232, title, 15, RED if k == 2 else INK, "bold",
                        anchor="start"))
        for j, s in enumerate(lines):
            out.append(mono(x + 16, 262 + j * 26, s, 15, INK))
        if k < 2:
            out.append(arrow(x + 324, 256, x + 356, 256, INK, 2))
    return out


if __name__ == "__main__":
    save("canary", W, H, build())
