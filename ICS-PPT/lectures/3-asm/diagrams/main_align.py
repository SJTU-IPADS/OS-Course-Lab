#!/usr/bin/env python3
"""Why main allocates 40 bytes for 32 bytes of arrays.

The strip is main's frame from main_frame.py plus the 8-byte return address
that the call into main pushed at 40(%rsp). Before that call %rsp was a
multiple of 16, so on entry it is 8 mod 16; 40 more bytes bring the total to
48 and %rsp back to 0 mod 16 for call dot_product. 16-byte boundaries are the
long ticks. Low addresses are on the left.
Run it to refresh ../assets/main-align.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from main_frame import B, CH, Y, arrays, bx
from svgkit import (BLUE, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE, MUTED, ORANGE,
                    WHITE, arrow, brace, line, mono, rect, save, text)

W, H = 1120, 380


def build():
    out = [text(560, 30, "main 的栈帧与返回地址（左侧为低地址）", 18, INK, "bold")]
    out += arrays(Y)
    out.append(rect(bx(32), Y, 8 * B, CH, WHITE, LINE, rx=2, width=1.6, dash="5 4"))
    out.append(text(bx(36), Y + 42, "填充", 16, MUTED))
    out.append(rect(bx(40), Y, 8 * B, CH, FILL_ORANGE, ORANGE, rx=2, width=1.6))
    out.append(text(bx(44), Y + 42, "返回地址", 16, INK, "bold"))
    out.append(rect(bx(48), Y, 1080 - bx(48), CH, FILL_GREY, LINE, rx=2, width=1.4))
    out.append(text((bx(48) + 1080) / 2, Y + 42, "调用 main 之前的栈", 16, MUTED))
    out += brace(bx(0) + 2, bx(40) - 2, Y - 12, BLUE, "subq $40, %rsp 分配的 40 字节",
                 15, below=False)
    out += brace(bx(40) + 2, bx(48) - 2, Y - 12, ORANGE, "call main 压入", 15,
                 below=False)
    # byte offsets; 16-byte boundaries are long and bold
    for off in range(0, 49, 8):
        edge = off % 16 == 0
        out.append(line(bx(off), Y + CH, bx(off), Y + CH + (20 if edge else 10),
                        INK if edge else LINE, 2 if edge else 1.6))
        out.append(mono(bx(off), Y + CH + 40, str(off), 15, INK if edge else MUTED,
                        "bold" if edge else "normal", anchor="middle"))
    # %rsp before the call into main, on entry, and after subq
    ya = 300
    for off, name, mod, color in [(0, "subq 之后的 %rsp", "≡ 0 (mod 16)", GREEN),
                                  (40, "进入 main 时的 %rsp", "≡ 8 (mod 16)", ORANGE)]:
        out.append(arrow(bx(off), ya - 18, bx(off), Y + CH + 50, color, 2.2))
        out.append(text(bx(off) + (70 if off == 0 else 0), ya + 4, name, 16, color,
                        "bold"))
        out.append(text(bx(off) + (70 if off == 0 else 0), ya + 26, mod, 16, color,
                        "bold"))
    out.append(text(560, 364, "40 + 8 = 48，是 16 的倍数：subq 之后 %rsp ≡ 0 (mod 16)，"
                    "call dot_product 时满足对齐要求", 16, INK))
    return out


if __name__ == "__main__":
    save("main-align", W, H, build())
