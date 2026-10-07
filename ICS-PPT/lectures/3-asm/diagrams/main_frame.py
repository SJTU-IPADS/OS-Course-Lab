#!/usr/bin/env python3
"""main's frame after `subq $40, %rsp`, byte offsets from the new %rsp.

From gcc -Og -fcf-protection=none -fno-stack-protector -S main.c: x at 0..15,
w at 16..31, 32..39 unused. What lies at 40 and above was on the stack before
main started and is not named here. Low addresses are on the left.
Run it to refresh ../assets/main-frame.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_ORANGE, GREEN, INK, LINE, MUTED, ORANGE, WHITE, arrow,
                    brace, line, mono, rect, save, text)

W, H = 1120, 370
X0, B = 60, 18                      # left edge, pixels per byte
Y, CH = 110, 70                     # top and height of the byte strip


def bx(off):
    return X0 + off * B


def arrays(y):
    """The eight int cells: x[0..3] at offsets 0..15, w[0..3] at 16..31."""
    out = []
    for k in range(8):
        name, value = (f"x[{k}]", k + 5) if k < 4 else (f"w[{k - 4}]", k - 3)
        out.append(rect(bx(4 * k), y, 4 * B, CH, FILL_ORANGE, ORANGE, rx=2, width=1.4))
        out.append(mono(bx(4 * k) + 2 * B, y + 27, name, 15, MUTED, anchor="middle"))
        out.append(mono(bx(4 * k) + 2 * B, y + 56, str(value), 20, INK, "bold",
                        anchor="middle"))
    return out


def build():
    out = [text(560, 30, "main 的栈帧（subq $40, %rsp 之后，左侧为低地址）", 18, INK, "bold")]
    out += arrays(Y)
    out.append(rect(bx(32), Y, 8 * B, CH, WHITE, LINE, rx=2, width=1.6, dash="5 4"))
    out.append(text(bx(36), Y + 42, "未使用", 16, MUTED))
    # the stack as it was before main started
    out.append(rect(bx(40), Y, 1060 - bx(40), CH, FILL_ORANGE, ORANGE, rx=2, width=1.4))
    out.append(text((bx(40) + 1060) / 2, Y + 42, "进入 main 之前已使用的栈", 16, MUTED))
    # where %rsp points: now (solid) and on entry (dashed)
    out.append(arrow(bx(0), 72, bx(0), Y - 4, INK, 2.2))
    out.append(text(bx(0) - 12, 64, "subq 之后的 %rsp", 16, INK, "bold", anchor="start"))
    out.append(arrow(bx(40), 72, bx(40), Y - 4, LINE, 2.2, dash="5 4"))
    out.append(text(bx(40), 64, "进入 main 时的 %rsp", 16, MUTED, "bold"))
    # byte offsets
    for off in range(0, 41, 4):
        out.append(line(bx(off), Y + CH, bx(off), Y + CH + 10, INK, 1.6))
        out.append(mono(bx(off), Y + CH + 30, str(off), 14, INK, anchor="middle"))
    out += brace(bx(0) + 2, bx(16) - 2, Y + CH + 40, ORANGE, "数组 x：偏移 0 ~ 15", 15)
    out += brace(bx(16) + 2, bx(32) - 2, Y + CH + 40, ORANGE, "数组 w：偏移 16 ~ 31", 15)
    # allocation and release move %rsp by the size of the frame
    out.append(text(bx(20), 292, "subq $40, %rsp：%rsp 减 40，分配栈帧", 16, ORANGE,
                    "bold"))
    out.append(arrow(bx(40), 304, bx(0), 304, ORANGE, 2.4))
    out.append(text(bx(20), 336, "addq $40, %rsp：%rsp 加 40，释放栈帧", 16, GREEN,
                    "bold"))
    out.append(arrow(bx(0), 348, bx(40), 348, GREEN, 2.4))
    return out


if __name__ == "__main__":
    save("main-frame", W, H, build())
