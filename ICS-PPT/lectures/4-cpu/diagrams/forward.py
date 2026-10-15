#!/usr/bin/env python3
"""imull writes %ecx in execute; addl reads it two stages later and can forward.

When addl is in execute, imull is in memory. The product was computed in the
previous cycle and is forwarded; addl does not wait for write-back.
Run it to refresh ../assets/forward.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, ORANGE, arrow, mono, rect, save, text)

W, H = 1120, 250
CW, CH = 100, 44
LX, TOP = 250, 70


def cell(col, row, name, fill, stroke):
    x = LX + col * CW
    y = TOP + row * (CH + 28)
    return [rect(x + 4, y, CW - 8, CH, fill, stroke, rx=3, width=1.6),
            text(x + CW / 2, y + 28, name, 18, stroke, "bold")], (x + CW / 2, y)


def build():
    out = [text(W / 2, 28, "imull 在执行阶段算出乘积，addl 下一拍就能用", 18, INK, "bold")]
    out.append(mono(24, TOP + 28, "imull", 16, INK))
    out.append(mono(24, TOP + CH + 28 + 28, "addl", 16, INK))
    colors = [("取", FILL_BLUE, BLUE), ("译", FILL_GREY, INK), ("执", FILL_ORANGE, ORANGE),
              ("访", FILL_GREEN, GREEN), ("写", FILL_BLUE, BLUE)]
    centres = []
    for row in range(2):
        for col, (name, fill, stroke) in enumerate(colors):
            shapes, top = cell(col + row, row, name, fill, stroke)
            out += shapes
            if name == "执":
                centres.append((top[0], top[1], top[1] + CH))
    x1, top1, bot1 = centres[0]
    x2, top2, bot2 = centres[1]
    out.append(arrow(x1, bot1 + 2, x2, top2 - 2, ORANGE, 2.4))
    out.append(text(x2 + 16, (bot1 + top2) / 2 + 5, "转发 %ecx", 15, ORANGE, "bold",
                    anchor="start"))
    return out


if __name__ == "__main__":
    save("forward", W, H, build())
