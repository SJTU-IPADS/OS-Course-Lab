#!/usr/bin/env python3
"""The stack at the base case of dot_product_rec(w, x, 4).

From gcc -Og -fcf-protection=none: each level with n > 0 is entered by call
(return address) and pushes %r15, %r14, %rbx, so it takes 32 bytes; the n = 0
level returns before the pushes. The return address of the n = 4 level
points into main, the others just after the call in dot_product_rec. Arrows
on the right are the returns, deepest first.
Run it to refresh ../assets/rec-stack.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, MUTED,
                    ORANGE, arrow, mono, path, rect, save, text)

W, H = 1120, 416
X, RW, SWD = 230, 250, 110          # bar left, return-address cell, saved cell
RH, STEP, TOP = 42, 54, 112


def frame(y, ret, pushes):
    out = [rect(X, y, RW, RH, FILL_BLUE, BLUE, rx=2, width=1.6),
           mono(X + RW / 2, y + 27, ret, 15, INK, "bold", anchor="middle")]
    if pushes:
        for k, r in enumerate(["%r15", "%r14", "%rbx"]):
            x = X + RW + k * SWD
            out.append(rect(x, y, SWD, RH, FILL_GREEN, GREEN, rx=2, width=1.4))
            out.append(mono(x + SWD / 2, y + 27, r, 15, INK, anchor="middle"))
    return out


def build():
    right = X + RW + 3 * SWD
    out = [text(X + RW / 2, 60, "返回地址（call 压入）", 15, BLUE, "bold"),
           text(X + RW + 1.5 * SWD, 60, "被调用者保存的寄存器（pushq）", 15, GREEN, "bold"),
           text(90, 60, "调用层", 15, MUTED)]
    out.append(rect(X, 70, right - X, 32, FILL_ORANGE, ORANGE, rx=2, width=1.4))
    out.append(text((X + right) / 2, 92, "main 的栈帧", 15, MUTED))
    out.append(mono(90, 92, "main", 15, MUTED, anchor="middle"))
    ends = [(right, 86)]
    for k, n in enumerate([4, 3, 2, 1, 0]):
        y = TOP + k * STEP
        ret = "返回 main" if n == 4 else "返回 dot_product_rec"
        out += frame(y, ret, n > 0)
        out.append(mono(90, y + 27, f"n = {n}", 15, INK, "bold", anchor="middle"))
        ends.append((right if n > 0 else X + RW, y + RH / 2))
    # returns: each level goes back to the one above it
    for k in range(len(ends) - 1, 0, -1):
        (x0, y0), (x1, y1) = ends[k], ends[k - 1]
        c = max(x0, x1) + 40
        out.append(path(f"M {x0 + 6} {y0} C {c} {y0} {c} {y1} {x1 + 8} {y1}", GREEN, 2))
        out.append(mono(c - 6, (y0 + y1) / 2 + 5, "ret", 14, GREEN, "bold"))
    out.append(text(right + 110, 200, "逐层返回：", 16, GREEN, "bold", anchor="start"))
    out.append(text(right + 110, 226, "pop 恢复寄存器，", 15, INK, anchor="start"))
    out.append(text(right + 110, 250, "ret 回到上一层，", 15, INK, anchor="start"))
    out.append(text(right + 110, 274, "%eax 带回部分和", 15, INK, anchor="start"))
    # growth direction and the stack top
    out.append(arrow(20, 110, 20, 370, MUTED, 1.8))
    out.append(text(30, 396, "低地址", 14, MUTED, anchor="start"))
    y0 = TOP + 4 * STEP + RH
    out.append(arrow(X + 40, y0 + 34, X + 40, y0 + 4, INK, 2.4))
    out.append(text(X + 50, y0 + 36, "%rsp（n = 0 时）", 15, INK, "bold", anchor="start"))
    return out


if __name__ == "__main__":
    save("rec-stack", W, H, build())
