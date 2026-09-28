#!/usr/bin/env python3
"""Linux (System V) and Windows (MS x64) at the moment a callee is entered.

Linux passes six integer arguments in registers and the stack top holds only
the return address. Windows passes four, and the caller has reserved 32
bytes of shadow space above the return address, one 8-byte home slot per
register argument (rcx at 8(%rsp) up to r9 at 32(%rsp)).
Run it to refresh ../assets/abi-frames.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, RED, arrow, line, mono, rect,
                    save, text, vbrace)

W, H = 1120, 400
SH = 34
SW = 220


def chips(x, y, regs):
    out = []
    for k, r in enumerate(regs):
        out.append(rect(x + k * 78, y, 70, 32, FILL_BLUE, BLUE, rx=4, width=1.4))
        out.append(mono(x + k * 78 + 35, y + 22, r, 15, INK, "bold", anchor="middle"))
    return out


def slot(x, y, label, off, fill, stroke, dash=None, color=INK):
    out = [rect(x, y, SW, SH, fill, stroke, rx=2, width=1.4, dash=dash),
           text(x + 12, y + 23, label, 14, color, anchor="start")]
    if off:
        out.append(mono(x + SW - 12, y + 23, off, 14, INK, anchor="end"))
    return out


def column(x0, title, regs, shadow, saved):
    out = [text(x0 + 250, 30, title, 19, INK, "bold"),
           text(x0, 68, "整型参数", 15, MUTED, anchor="start")]
    out += chips(x0 + 80, 46, regs)
    sx = x0 + 150
    out.append(text(sx + SW / 2, 114, "进入被调用函数时的栈顶", 15, INK, "bold"))
    y = 128
    out += slot(sx, y, "调用者的栈数据", "", FILL_GREY, LINE, color=MUTED)
    y += SH
    if shadow:
        top = y
        for k, r in enumerate(["%r9", "%r8", "%rdx", "%rcx"]):
            out += slot(sx, y, f"{r} 的归属槽", f"{32 - 8 * k}(%rsp)", FILL_GREEN, GREEN)
            y += SH
        out += vbrace(sx - 10, top + 2, y - 2, GREEN, "影子空间 32 字节", 15, right=False)
    out += slot(sx, y, "返回地址", "0(%rsp)", FILL_ORANGE, ORANGE)
    ry = y + SH                     # %rsp is the low edge of the return address
    out.append(arrow(sx + SW + 60, ry, sx + SW + 6, ry, BLUE, 2.4))
    out.append(mono(sx + SW + 66, ry + 6, "%rsp", 16, BLUE, "bold"))
    who, color = saved
    out.append(mono(x0 + 250, 382, "%rsi, %rdi", 16, INK, "bold", anchor="end"))
    out.append(text(x0 + 258, 382, "：" + who, 16, color, "bold", anchor="start"))
    return out


def build():
    out = column(20, "Linux（System V）", ["%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"],
                 False, ("调用者保存", ORANGE))
    out += column(590, "Windows（MS x64）", ["%rcx", "%rdx", "%r8", "%r9"],
                  True, ("被调用者保存", RED))
    out.append(line(575, 16, 575, 390, LINE, 1))
    return out


if __name__ == "__main__":
    save("abi-frames", W, H, build())
