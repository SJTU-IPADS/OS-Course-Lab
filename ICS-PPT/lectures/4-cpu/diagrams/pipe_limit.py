#!/usr/bin/env python3
"""Why the clock is not five times faster: the slowest stage, and the registers.

Left: three arithmetic stages of unequal work; the clock waits for the middle
one. Right: a register sits between equal stages and takes part of the cycle.
The proportions are the textbook's illustration of the idea, not a measurement
of a core. Run it to refresh ../assets/pipe-limit.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, FILL_BLUE, FILL_ORANGE, INK, MUTED, ORANGE, box, save, text

W, H = 1120, 250


def build():
    out = [text(280, 36, "三段的工作量不一样", 18, INK, "bold"),
           text(840, 36, "段与段之间要加寄存器", 18, INK, "bold")]
    widths = (90, 220, 90)
    x = 70
    y, bh = 78, 80
    for i, w in enumerate(widths):
        out += box(x, y, w, bh, "甲乙丙"[i], FILL_ORANGE if i == 1 else FILL_BLUE,
                   ORANGE if i == 1 else BLUE, 20)
        x += w + 14
    out.append(text(280, 196, "时钟要等最慢的一段", 16, ORANGE, "bold"))
    out.append(text(280, 222, "另外两段提前做完，然后空等", 15, MUTED))

    x = 690
    for i in range(3):
        out += box(x, y, 80, bh, "甲乙丙"[i], FILL_BLUE, BLUE, 20)
        x += 80
        if i < 2:
            out += box(x, y + 22, 36, 36, "存", FILL_ORANGE, ORANGE, 14)
            x += 36
    out.append(text(840, 196, "寄存器本身要时间", 16, ORANGE, "bold"))
    out.append(text(840, 222, "级数变多，这部分占比变大", 15, MUTED))
    return out


if __name__ == "__main__":
    save("pipe-limit", W, H, build())
