#!/usr/bin/env python3
"""Two call sites, A in main and B in another function, enter dot_product.

When dot_product is done it has to go back to the instruction after A or the
one after B; a jmp at its end can hold only one of them. The page comes before
call and ret are introduced, so the figure names neither and carries no
addresses.
Run it to refresh ../assets/return-address.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, GREEN, INK, MUTED, RED, WHITE, path, rect,
                    save, text)

W, H = 1120, 220

ROWS = [("main 中", "调用 dot_product", "调用点 A", BLUE),
        ("", "A 的下一条指令", None, BLUE),
        ("另一个函数中", "调用 dot_product", "调用点 B", GREEN),
        ("", "B 的下一条指令", None, GREEN)]


def build():
    out = []
    ys = [8, 52, 128, 172]
    for (where, ins, tag, color), y in zip(ROWS, ys):
        call = tag is not None
        out.append(rect(130, y, 280, 40, FILL_BLUE if call else WHITE, color, rx=4,
                        width=1.6 if call else 1.2))
        if call:
            out.append(text(120, y + 26, where, 16, INK, "bold", anchor="end"))
            out.append(text(146, y + 26, ins, 16, INK, anchor="start"))
            out.append(text(420, y + 26, tag, 15, color, "bold", anchor="start"))
        else:
            out.append(text(146, y + 26, ins, 16, MUTED, anchor="start"))
    out.append(text(270, 112, "⋮", 18, MUTED))

    # the function: its name at the top, the ret at the bottom
    out.append(rect(640, 68, 250, 98, FILL_BLUE, BLUE, rx=6, width=1.6))
    out.append(text(765, 98, "dot_product", 19, INK, "bold", font="monospace"))
    out.append(rect(660, 116, 210, 36, WHITE, RED, rx=4, width=1.6))
    out.append(text(765, 140, "执行完毕：回到哪里？", 16, RED, "bold"))
    out.append(path("M 500 28 C 560 28, 580 78, 636 84", BLUE, 2))
    out.append(path("M 500 148 C 560 148, 580 112, 636 106", GREEN, 2))
    out.append(path("M 660 134 C 560 134, 520 72, 414 72", BLUE, 1.8, dash="6 4"))
    out.append(path("M 660 146 C 560 146, 520 192, 414 192", GREEN, 1.8, dash="6 4"))
    out.append(text(910, 78, "回到 A 之后还是 B 之后，", 16, INK, anchor="start"))
    out.append(text(910, 104, "取决于这次是谁调用的", 16, INK, anchor="start"))
    out.append(text(910, 140, "jmp 的目标写在指令中，", 16, MUTED, anchor="start"))
    out.append(text(910, 166, "只能是其中一个", 16, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("return-address", W, H, build(), left=-9)    # centres the ink on the canvas
