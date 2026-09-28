#!/usr/bin/env python3
"""Call sites A (0x4010) and B (0x4080) enter one function; where does ret go?

A call instruction is 5 bytes here, so the instruction after each call is at
0x4015 and 0x4085. A jmp at the end of the function can hold only one of them.
Run it to refresh ../assets/return-address.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, GREEN, INK, MUTED, ORANGE, RED,
                    WHITE, mono, path, rect, save, text)

W, H = 1120, 220

ROWS = [(0x4010, "call dot_product", "调用点 A", BLUE),
        (0x4015, "下一条指令", None, BLUE),
        (0x4080, "call dot_product", "调用点 B", GREEN),
        (0x4085, "下一条指令", None, GREEN)]


def build():
    out = []
    ys = [8, 52, 128, 172]
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
    out.append(text(270, 112, "⋮", 18, MUTED))

    # the function: its name at the top, the ret at the bottom
    out.append(rect(640, 68, 250, 98, FILL_ORANGE, ORANGE, rx=6, width=1.6))
    out.append(text(765, 98, "dot_product", 19, INK, "bold", font="monospace"))
    out.append(rect(660, 116, 210, 36, WHITE, RED, rx=4, width=1.6))
    out.append(text(765, 140, "ret：回到哪里？", 16, RED, "bold"))
    out.append(path("M 500 28 C 560 28, 580 78, 636 84", BLUE, 2))
    out.append(path("M 500 148 C 560 148, 580 112, 636 106", GREEN, 2))
    out.append(path("M 660 134 C 560 134, 520 72, 414 72", BLUE, 1.8, dash="6 4"))
    out.append(path("M 660 146 C 560 146, 520 192, 414 192", GREEN, 1.8, dash="6 4"))
    out.append(text(910, 78, "0x4015 还是 0x4085，", 16, INK, anchor="start"))
    out.append(text(910, 104, "取决于这次是谁调用的", 16, INK, anchor="start"))
    out.append(text(910, 140, "固定目标的 jmp 只能", 16, MUTED, anchor="start"))
    out.append(text(910, 166, "写死其中一个", 16, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("return-address", W, H, build())
