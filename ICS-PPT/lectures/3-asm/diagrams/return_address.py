#!/usr/bin/env python3
"""Call sites A (0x4010) and B (0x4080) enter one function; where does ret go?

A call instruction is 5 bytes here, so the instruction after each call is at
0x4015 and 0x4085. A jmp at the end of the function can hold only one of them.
Run it to refresh ../assets/return-address.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE,
                    MUTED, ORANGE, RED, WHITE, arrow, box, mono, path, rect,
                    save, text)

W, H = 1120, 250

ROWS = [(0x4010, "call dot_product", "调用点 A", BLUE),
        (0x4015, "下一条指令", None, BLUE),
        (0x4080, "call dot_product", "调用点 B", GREEN),
        (0x4085, "下一条指令", None, GREEN)]


def build():
    out = []
    ys = [20, 64, 140, 184]
    for (addr, ins, tag, color), y in zip(ROWS, ys):
        call = tag is not None
        out.append(rect(130, y, 280, 40, FILL_BLUE if call else WHITE, color, rx=4,
                        width=1.6 if call else 1.2))
        out.append(mono(18, y + 26, f"0x{addr:x}", 17, INK if call else MUTED, "bold"))
        if call:
            out.append(mono(146, y + 26, ins, 17, INK))
            out.append(text(420, y + 26, tag, 15, color, "bold", anchor="start"))
        else:
            out.append(text(146, y + 26, ins, 16, MUTED, anchor="start"))
    out.append(text(270, 124, "⋮", 18, MUTED))

    out += box(640, 40, 250, 150, "dot_product", FILL_ORANGE, ORANGE, 19,
               font="monospace", sub=None)
    out.append(rect(660, 140, 210, 36, WHITE, RED, rx=4, width=1.6))
    out.append(text(765, 164, "ret：回到哪里？", 16, RED, "bold"))
    out.append(path("M 500 40 C 560 40, 580 70, 636 74", BLUE, 2))
    out.append(path("M 500 160 C 560 160, 580 110, 636 102", GREEN, 2))
    out.append(path("M 660 158 C 560 158, 520 84, 414 84", BLUE, 1.8, dash="6 4"))
    out.append(path("M 660 170 C 560 170, 520 204, 414 204", GREEN, 1.8, dash="6 4"))
    out.append(text(910, 90, "0x4015 还是 0x4085，", 16, INK, anchor="start"))
    out.append(text(910, 116, "取决于这次是谁调用的", 16, INK, anchor="start"))
    out.append(text(910, 152, "固定目标的 jmp 只能", 16, MUTED, anchor="start"))
    out.append(text(910, 178, "写死其中一个", 16, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("return-address", W, H, build())
