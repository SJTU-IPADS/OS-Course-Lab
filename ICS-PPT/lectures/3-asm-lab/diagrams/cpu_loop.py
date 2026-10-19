#!/usr/bin/env python3
"""The main loop of interp_run: x86_begin, the switch, x86_end, and again.

A side figure: the code of the loop stands beside it on the slide.
Run it to refresh ../assets/cpu_loop.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, MUTED, ORANGE, BLUE,
                    arrow, elbow, rect, save, text)
from cpu_kit import code, note

W, H = 520, 470
X0, BW = 60, 400


def step(y, h, name, lines, fill, stroke):
    out = [rect(X0, y, BW, h, fill, stroke, rx=8, width=1.6),
           code(X0 + 18, y + 30, name, 19, INK, "bold")]
    for k, s in enumerate(lines):
        out.append(text(X0 + 18, y + 62 + k * 26, s, 17, INK, anchor="start"))
    return out


def build():
    out = []
    out += step(14, 100, "x86_begin(c, &x)",
                ["跳过前缀，记下 x.rex、x.size",
                 "读操作码，返回 op"],
                FILL_GREY, MUTED)
    out.append(arrow(X0 + BW / 2, 114, X0 + BW / 2, 140, INK, 2))
    out += step(144, 126, "switch (op)",
                ["case：读其余字节，计算，",
                 "写寄存器与内存",
                 "default：x86_other"],
                FILL_ORANGE, ORANGE)
    out.append(arrow(X0 + BW / 2, 270, X0 + BW / 2, 296, INK, 2))
    out += step(300, 100, "x86_end(&x)",
                ["没有异常：返回 0",
                 "有异常：恢复寄存器，返回 -1"],
                FILL_GREY, MUTED)
    out.append(elbow([(X0 + BW / 2, 400), (X0 + BW / 2, 426), (X0 - 30, 426),
                      (X0 - 30, 64), (X0 - 4, 64)], BLUE, 2))
    out.append(text(X0 + BW / 2 + 10, 454, "返回 0：下一条指令", 17, BLUE, "bold", anchor="start"))
    out.append(text(X0 + BW + 8, 352, "-1", 17, MUTED, "bold", anchor="start"))
    out.append(text(X0 + BW + 8, 374, "结束", 17, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("cpu_loop", W, H, build())
