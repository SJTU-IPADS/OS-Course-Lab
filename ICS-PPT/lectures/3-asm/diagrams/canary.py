#!/usr/bin/env python3
"""The stack canary in the stack of echo.c, built with the protector.

Offsets are echo.s's (gcc -Og -fcf-protection=none): after `pushq %rbx` and
`subq $16, %rsp`, buf[0..7] at 0..7, the canary at 8..15, the saved %rbx at
16..23. The return address pushed by main's call is at 24..31, and the
8 bytes main allocated at 32..39. gets writes upward from buf, so a line
longer than buf changes the canary before it reaches the saved register or
the return address; the check before `ret` then fails.

Same scale and colours as echo-frame.svg, whose row this one is compared
with. The canary is red.
Run it to refresh ../assets/canary.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from echo_frame import CH, Y, bx, frame
from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, FILL_RED, GREEN, LINE,
                    ORANGE, RED, WHITE, arrow, save, text)

W, H = 1120, 178

PARTS = [(0, 8, "buf[0..7]", FILL_ORANGE, ORANGE, None),
         (8, 8, "金丝雀值", FILL_RED, RED, None),
         (16, 8, "保存的 %rbx", FILL_GREEN, GREEN, None),
         (24, 8, "返回地址", FILL_BLUE, BLUE, None),
         (32, 8, "未使用", WHITE, LINE, "5 4")]


def build():
    out = frame(PARTS, top="金丝雀值")
    # a line longer than buf runs toward higher addresses and meets the canary first
    y = Y + CH + 56
    out.append(arrow(bx(0), y, bx(16), y, RED, 2.4))
    out.append(text(bx(16) + 12, y + 5, "超出 buf 的写入先改写金丝雀值", 15, RED, "bold",
                    anchor="start"))
    return out


if __name__ == "__main__":
    save("canary", W, H, build(), left=4)    # centres the ink on the canvas
