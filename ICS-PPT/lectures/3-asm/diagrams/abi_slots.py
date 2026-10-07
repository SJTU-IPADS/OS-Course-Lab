#!/usr/bin/env python3
"""System V AMD64: where integer arguments and the return value live.

Arguments 1-6 go in registers in a fixed order; arguments 7 and 8 are on the
stack, at 8(%rsp) and 16(%rsp) on entry to the callee, above the return
address at 0(%rsp). The return value uses the part of %rax that matches its
width.
Run it to refresh ../assets/abi-slots.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE,
                    arrow, mono, rect, save, text)

W, H = 1120, 450
REGS = ["%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"]
CARD, STEP = 124, 136
SX = 22 + 6 * STEP                 # left edge of the stack part


def cards():
    out = [text(22 + 3 * STEP - 6, 24, "参数 1 ~ 6：寄存器（按序分配）", 16, INK, "bold"),
           text(SX + STEP - 6, 24, "参数 7 及以后：栈", 16, INK, "bold")]
    for k in range(8):
        x = 22 + k * STEP
        reg = k < 6
        out.append(rect(x, 38, CARD, 104, FILL_BLUE if reg else WHITE, BLUE if reg else LINE,
                        rx=6, width=1.6, dash=None if reg else "5 4"))
        out.append(text(x + CARD / 2, 64, f"参数 {k + 1}", 15, MUTED))
        if reg:
            out.append(mono(x + CARD / 2, 106, REGS[k], 22, INK, "bold", anchor="middle"))
        else:
            out.append(mono(x + CARD / 2, 106, f"{8 * (k - 5)}(%rsp)", 18, INK, "bold",
                            anchor="middle"))
    return out


def stack():
    x, w, sh = SX, 2 * STEP - 12, 44
    rows = [("调用者的栈数据", "", FILL_ORANGE, ORANGE),
            ("参数 8", "16(%rsp)", FILL_ORANGE, ORANGE),
            ("参数 7", "8(%rsp)", FILL_ORANGE, ORANGE),
            ("返回地址", "0(%rsp)", FILL_BLUE, BLUE)]
    out = [text(x + w / 2, 196, "进入被调用函数时的栈顶", 15, INK, "bold")]
    for k, (name, off, fill, stroke) in enumerate(rows):
        y = 214 + k * sh
        out.append(rect(x, y, w, sh, fill, stroke, rx=2, width=1.6))
        out.append(text(x + 14, y + 28, name, 15, INK if off else MUTED, anchor="start"))
        if off:
            out.append(mono(x + w - 14, y + 28, off, 15, INK, "bold", anchor="end"))
    ry = 214 + 4 * sh               # %rsp is the low edge of the return address
    out.append(arrow(x - 70, ry, x - 6, ry, INK, 2.4))
    out.append(mono(x - 76, ry + 6, "%rsp", 16, INK, "bold", anchor="end"))
    out.append(text(x + w / 2, 420, "由调用者在 call 之前从右向左压入", 14, MUTED))
    return out


def result():
    right, bit, top = 680, 7, 214
    rows = [("%rax", 64, "long / 指针"), ("%eax", 32, "int"),
            ("%ax", 16, "short"), ("%al", 8, "char")]
    out = [text(right - 32 * bit, 196, "返回值：%rax，按宽度使用", 15, INK, "bold")]
    for k, (name, bits, ctype) in enumerate(rows):
        y, w = top + k * 44, bits * bit
        out.append(rect(right - w, y + 4, w, 36, FILL_ORANGE, ORANGE, rx=3, width=1.6))
        out.append(mono(right - w / 2, y + 28, name, 17, INK, "bold", anchor="middle"))
        out.append(text(right - w - 12, y + 28, ctype, 15, MUTED, anchor="end"))
    return out


def build():
    return cards() + stack() + result()


if __name__ == "__main__":
    save("abi-slots", W, H, build())
