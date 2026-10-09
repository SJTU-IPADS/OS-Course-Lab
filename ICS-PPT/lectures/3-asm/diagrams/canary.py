#!/usr/bin/env python3
"""The stack canary in the stack of echo.c, built with the protector.

Offsets are echo.s's (gcc -Og -fcf-protection=none): after `pushq %rbx` and
`subq $16, %rsp`, buf[0..7] at 0..7, the canary at 8..15, the saved %rbx at
16..23. The return address pushed by main's call is at 24..31. gets writes
upward from buf, so a line longer than buf changes the canary before it
reaches the saved register or the return address; the check before `ret`
then fails.

Drawn by echo_stack.py's parts, on the canvas of echo-stack-{1..4}.svg, whose
last frame this figure is compared with: the same rows, with buf one row
lower and the canary in the row above it. The canary is red.
Run it to refresh ../assets/canary.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from echo_stack import BOTTOM, H, LEFT, RH, W, caption, pointer, stack, writes
from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, FILL_RED, FONT, GREEN,
                    MONO, ORANGE, RED, save)

ROWS = [(24, [("返回地址", FONT)], FILL_BLUE, BLUE),
        (16, [("保存的 ", FONT), ("%rbx", MONO)], FILL_GREEN, GREEN),
        (8, [("金丝雀值", FONT)], FILL_RED, RED),
        (0, None, FILL_ORANGE, ORANGE)]                 # buf


def build():
    out = stack(ROWS, 0, 8)
    out += pointer(BOTTOM, "buf = %rsp", 36)    # short: the label ends left of the arrow
    # a line longer than buf runs toward higher addresses and meets the canary first
    tip = BOTTOM - 2 * RH
    out += writes(BOTTOM, tip, RED)
    out += caption(tip, "超出 buf 的写入", [("先改写金丝雀值", FONT)], RED)
    return out


if __name__ == "__main__":
    save("canary", W, H, build(), left=LEFT)
