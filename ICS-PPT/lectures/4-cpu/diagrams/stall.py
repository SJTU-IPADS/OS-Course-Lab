#!/usr/bin/env python3
"""Load-use: imull needs %ecx while movl is still in the memory stage.

The value is not ready at the start of that cycle, so imull waits one cycle
between decode and execute. Run it to refresh ../assets/stall.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_RED,
                    GREEN, INK, ORANGE, RED, mono, rect, save, text)

W, H = 1120, 250
CW, CH = 90, 44
LX, TOP = 200, 64


def build():
    out = [text(W / 2, 28, "movl 还在访存，imull 的执行用不上 %ecx", 18, INK, "bold")]
    for c in range(7):
        out.append(text(LX + c * CW + CW / 2, TOP - 10, str(c + 1), 14, INK))
    rows = [
        ("movl", "取译执访写"),
        ("imull", " 取译停执访写"),
    ]
    tone = {
        "取": (FILL_BLUE, BLUE), "译": (FILL_GREY, INK), "执": (FILL_ORANGE, ORANGE),
        "访": (FILL_GREEN, GREEN), "写": (FILL_BLUE, BLUE), "停": (FILL_RED, RED),
    }
    for r, (name, stages) in enumerate(rows):
        y = TOP + 16 + r * (CH + 24)
        out.append(mono(24, y + 28, name, 16, INK))
        for c, ch in enumerate(stages):
            if ch == " ":
                continue
            fill, stroke = tone[ch]
            x = LX + c * CW
            out.append(rect(x + 4, y, CW - 8, CH, fill, stroke, rx=3, width=1.6))
            out.append(text(x + CW / 2, y + 28, ch, 18, stroke, "bold"))
    out.append(text(W / 2, 228, "停的那一拍，movl 在访存，imull 还不能进入执行", 16, RED, "bold"))
    return out


if __name__ == "__main__":
    save("stall", W, H, build())
