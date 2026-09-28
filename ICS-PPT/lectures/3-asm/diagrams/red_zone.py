#!/usr/bin/env python3
"""The 128-byte red zone below %rsp, used by leaf_example.

High addresses are at the top. On entry %rsp points at the return address;
the red zone is [-128, -1](%rsp). leaf_example (gcc -Og and -O2) stores a at
-8(%rsp) and b at -4(%rsp) there and never moves %rsp.
Run it to refresh ../assets/red-zone.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_RED,
                    GREEN, INK, LINE, MUTED, ORANGE, RED, WHITE, arrow, listing,
                    mono, rect, save, text, vbrace)

W, H = 1120, 470
X, SW = 300, 300

ASM = ["leaf_example:",
       "\tmovl\t%edi, -8(%rsp)",
       "\tmovl\t%esi, -4(%rsp)",
       "\tmovl\t-8(%rsp), %eax",
       "\tmovl\t-4(%rsp), %edx",
       "\taddl\t%edx, %eax",
       "\tret"]


def slot(y, h, label, off, fill, stroke, color=INK, dash=None):
    out = [rect(X, y, SW, h, fill, stroke, rx=2, width=1.6, dash=dash),
           text(X + 14, y + h / 2 + 6, label, 15, color, anchor="start")]
    if off:
        out.append(mono(X + SW - 14, y + h / 2 + 6, off, 15, INK, "bold", anchor="end"))
    return out


def build():
    out = [text(X + SW / 2, 30, "进入 leaf_example 时的栈顶（上方为高地址）", 17, INK, "bold")]
    out += slot(50, 46, "调用者的栈数据", "", FILL_GREY, LINE, MUTED)
    out += slot(96, 46, "返回地址", "0(%rsp)", FILL_ORANGE, ORANGE)
    # the red zone
    top, bottom = 142, 420
    out.append(rect(X, top, SW, bottom - top, FILL_RED, RED, rx=2, width=2, dash="7 4"))
    out += slot(top + 8, 40, "y（参数 b）", "-4(%rsp)", FILL_GREEN, GREEN)
    out += slot(top + 48, 40, "x（参数 a）", "-8(%rsp)", FILL_BLUE, BLUE)
    out.append(text(X + SW / 2, 300, "其余空间：-128(%rsp) ~ -9(%rsp)", 15, MUTED))
    out += vbrace(X - 10, top, bottom, RED, "红区 128 字节", 17, depth=10, right=False)
    out.append(text(X - 28, (top + bottom) / 2 + 32, "中断与信号处理程序", 14, RED,
                    anchor="end"))
    out.append(text(X - 28, (top + bottom) / 2 + 52, "不会覆写", 14, RED, anchor="end"))
    # %rsp stays where it is
    out.append(arrow(X - 70, 119, X - 6, 119, BLUE, 2.4))
    out.append(mono(X - 76, 125, "%rsp", 16, BLUE, "bold", anchor="end"))
    # the listing; each store shares its slot's colour
    lx = 700
    out.append(text(lx - 16, 44, "gcc -Og / -O2 的输出", 15, MUTED, anchor="start"))
    out.append(rect(lx - 16, 60, 380, 7 * 30 + 24, WHITE, LINE, rx=6))
    out += listing(lx, 72, ASM, 16, 30, marks={1: FILL_BLUE, 2: FILL_GREEN})
    out.append(text(lx - 16, 340, "没有 subq / addq：%rsp 全程不变", 16, INK, "bold",
                    anchor="start"))
    out.append(text(lx - 16, 368, "局部变量直接用负偏移寻址", 15, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("red-zone", W, H, build())
