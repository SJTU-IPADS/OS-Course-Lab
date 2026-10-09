#!/usr/bin/env python3
"""Where the writes of gets land in the stack of echo.c.

Offsets are echo.s's (gcc -Og -fcf-protection=none -fno-stack-protector):
after `pushq %rbx` and `subq $16, %rsp`, 8 unused bytes at 0..7, buf[0..7] at
8..15, the saved %rbx at 16..23. The return address pushed by main's call is
at 24..31, and the 8 bytes main allocated with `subq $8, %rsp` at 32..39.
gets writes upward from buf, one byte per character and one for the
terminator, so the length of the line decides how far the writes reach:
7 characters fill buf, 15 reach the saved %rbx, 23 reach the return address.

Colours as in the other stack figures: a local array orange, the saved
register green, the return address blue. A write past the end of buf is red.
Run it to refresh ../assets/echo-frame.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, LINE,
                    MUTED, ORANGE, RED, WHITE, arrow, brace, mono, rect, save, text)

W, H = 1120, 252
X0, B = 120, 22                     # left edge, pixels per byte
Y, CH = 44, 56                      # frame row: top, height

# the stack, low addresses first: offset, size, name, fill, stroke, dash
PARTS = [(0, 8, "未使用", WHITE, LINE, "5 4"),
         (8, 8, "buf[0..7]", FILL_ORANGE, ORANGE, None),
         (16, 8, "保存的 %rbx", FILL_GREEN, GREEN, None),
         (24, 8, "返回地址", FILL_BLUE, BLUE, None),
         (32, 8, "未使用", WHITE, LINE, "5 4")]

# how far each line writes: characters, end offset, result, tone
RUNS = [(7, 16, "与结束符共 8 字节，填满 buf", INK),
        (15, 24, "覆盖保存的 %rbx", RED),
        (23, 32, "覆盖返回地址，SIGSEGV", RED)]


def bx(off):
    return X0 + off * B


def frame(parts, top=None):
    """The row of cells, its offsets and the two frames it belongs to."""
    out = []
    for off, n, name, fill, stroke, dash in parts:
        w = n * B
        out.append(rect(bx(off), Y, w, CH, fill, stroke, rx=2, dash=dash,
                        width=3 if name == top else 1.6))
        if name.isascii():
            out.append(mono(bx(off) + w / 2, Y + 35, name, 16, INK, "bold",
                            anchor="middle"))
        else:
            out.append(text(bx(off) + w / 2, Y + 34, name, 15,
                            MUTED if dash else RED if name == top else INK,
                            "normal" if dash else "bold"))
    for off in range(0, 41, 8):
        s = "(%rsp)" if off == 0 else f"{off}(%rsp)"
        out.append(mono(bx(off), Y + CH + 22, s, 13, MUTED, anchor="middle"))
    # the return address is pushed by main's call and belongs to main's frame
    out += brace(bx(0) + 2, bx(24) - 2, Y - 4, MUTED, "echo 的栈帧", 14, depth=6,
                 below=False)
    out += brace(bx(24) + 2, bx(40) - 2, Y - 4, MUTED, "main 的栈帧", 14, depth=6,
                 below=False)
    out.append(text(bx(0), Y - 16, "低地址", 14, MUTED, anchor="start"))
    out.append(text(bx(40), Y - 16, "高地址", 14, MUTED, anchor="end"))
    return out


def runs(y0):
    """One row per line of input: an arrow from buf to where its writes stop."""
    out = []
    for k, (count, end, note, tone) in enumerate(RUNS):
        y = y0 + k * 40
        out.append(text(bx(8) - 12, y + 5, f"{count} 个字符", 15, tone, "bold",
                        anchor="end"))
        out.append(arrow(bx(8), y, bx(end), y, tone, 2.4))
        out.append(text(bx(end) + 12, y + 5, note, 15, tone, "bold", anchor="start"))
    return out


def build():
    return frame(PARTS) + runs(Y + CH + 56)


if __name__ == "__main__":
    save("echo-frame", W, H, build(), left=4)    # centres the ink on the canvas
