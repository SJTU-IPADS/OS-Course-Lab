#!/usr/bin/env python3
"""One multiply-add on the left, the 4096 iterations it has to become on the right.

The dashed arc over the sequence is the missing piece: a way to send execution
back to the same instructions instead of writing them out 4096 times.
Run it to refresh ../assets/loop-unroll.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, MUTED, ORANGE,
                    RED, WHITE, arrow, box, mono, path, rect, save, text)

W, H = 1120, 190


def build():
    out = [rect(20, 70, 240, 100, FILL_ORANGE, ORANGE, rx=8, width=1.6),
           text(140, 96, "单次乘加（i = 0）", 16, INK, "bold")]
    for k, s in enumerate(["movl", "imull", "addl"]):
        out.append(mono(140, 120 + k * 18, s, 15, INK, anchor="middle"))
    out.append(arrow(268, 120, 330, 120, INK, 2.2))
    steps = ["i = 0", "i = 1", "i = 2", "i = 3", "…", "i = 4095"]
    for k, s in enumerate(steps):
        x = 340 + k * 128
        if s == "…":
            out.append(text(x + 56, 126, s, 20, MUTED, "bold"))
            continue
        out += box(x, 92, 112, 56, s, FILL_BLUE, BLUE, 16, font="monospace")
        if k < len(steps) - 1:
            out.append(arrow(x + 114, 120, x + 126, 120, BLUE, 1.6))
    out.append(text(740, 176, "展开后共 4096 份相同的乘加指令", 15, MUTED))
    out.append(path("M 1036 88 C 1000 40, 440 40, 400 88", RED, 2.2, dash="7 5"))
    out.append(text(720, 30, "回环：同一段指令重复执行 4096 次", 17, RED, "bold"))
    return out


if __name__ == "__main__":
    save("loop-unroll", W, H, build())
