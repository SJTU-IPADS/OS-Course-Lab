#!/usr/bin/env python3
"""jl decides in execute; the next instruction has already been fetched.

Run it to refresh ../assets/branch.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, FILL_RED, INK, ORANGE,
                    RED, mono, rect, save, text)

W, H = 1120, 240
CW, CH = 100, 44
LX, TOP = 280, 64


def build():
    out = [text(W / 2, 28, "jl 在执行阶段才知道跳不跳，下一条已经被取进来", 18, INK, "bold")]
    rows = [("jl", "取译执"), ("下一条", " 取")]
    tone = {"取": (FILL_BLUE, BLUE), "译": (FILL_GREY, INK), "执": (FILL_ORANGE, ORANGE)}
    for r, (name, stages) in enumerate(rows):
        y = TOP + 16 + r * (CH + 28)
        out.append(mono(36, y + 28, name, 16, INK))
        for c, ch in enumerate(stages):
            if ch == " ":
                continue
            fill, stroke = tone[ch]
            x = LX + c * CW
            out.append(rect(x + 4, y, CW - 8, CH, fill, stroke, rx=3, width=1.6))
            out.append(text(x + CW / 2, y + 28, ch, 18, stroke, "bold"))
    # the fetched instruction may be thrown away
    y = TOP + 16 + CH + 28
    x = LX + 2 * CW
    out.append(rect(x + 4, y, CW - 8, CH, FILL_RED, RED, rx=3, width=1.6, dash="5 4"))
    out.append(text(x + CW / 2, y + 28, "作废", 16, RED, "bold"))
    out.append(text(W / 2, 214, "猜错时，已经取入的指令不作数", 16, RED, "bold"))
    return out


if __name__ == "__main__":
    save("branch", W, H, build())
