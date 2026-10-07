#!/usr/bin/env python3
"""One multiply-add on the left, the 4096 iterations it has to become on the right.

Written out, the three instructions would have to appear 4096 times.
Run it to refresh ../assets/loop-unroll.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, MUTED, ORANGE, arrow, box,
                    mono, rect, save, text)

W, H = 1120, 132


def build():
    out = [rect(20, 12, 240, 100, FILL_ORANGE, ORANGE, rx=8, width=1.6),
           text(140, 38, "单次乘加（i = 0）", 16, INK, "bold")]
    for k, s in enumerate(["movl", "imull", "addl"]):
        out.append(mono(140, 62 + k * 18, s, 15, INK, anchor="middle"))
    out.append(arrow(268, 62, 330, 62, INK, 2.2))
    steps = ["i = 0", "i = 1", "i = 2", "i = 3", "…", "i = 4095"]
    for k, s in enumerate(steps):
        x = 340 + k * 128
        if s == "…":
            out.append(text(x + 56, 68, s, 20, MUTED, "bold"))
            continue
        out += box(x, 34, 112, 56, s, FILL_BLUE, BLUE, 16, font="monospace")
        if k < len(steps) - 1:
            out.append(arrow(x + 114, 62, x + 126, 62, BLUE, 1.6))
    out.append(text(740, 118, "展开后共 4096 份相同的乘加指令", 15, MUTED))
    return out


if __name__ == "__main__":
    save("loop-unroll", W, H, build(), left=-4)    # centres the ink on the canvas
