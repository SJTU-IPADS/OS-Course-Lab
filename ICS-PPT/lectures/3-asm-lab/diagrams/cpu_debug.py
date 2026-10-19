#!/usr/bin/env python3
"""The loop of a working session: run a guest, read the illegal-instruction
report, look the address up with objdump -d, add the case, run again.

A side figure. Run it to refresh ../assets/cpu_debug.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK, MUTED, ORANGE, BLUE,
                    WHITE, arrow, elbow, rect, save, text)
from cpu_kit import code, note

W, H = 480, 470
X0, BW, BH = 50, 380, 78


def step(y, title, lines, fill, stroke):
    out = [rect(X0, y, BW, BH, fill, stroke, rx=8, width=1.5),
           text(X0 + 16, y + 30, title, 19, INK, "bold", anchor="start")]
    for k, s in enumerate(lines):
        out.append(code(X0 + 16, y + 58 + k * 20, s, 16, INK))
    return out


def build():
    out = []
    out += step(14, "运行一个客户机程序", ["./mini-cpu guest/hello"], FILL_GREY, MUTED)
    out.append(arrow(X0 + BW / 2, 92, X0 + BW / 2, 112, INK, 2))
    out += step(116, "报告非法指令的地址", ["rip  0000000000401000"], FILL_ORANGE, ORANGE)
    out.append(arrow(X0 + BW / 2, 194, X0 + BW / 2, 214, INK, 2))
    out += step(218, "objdump -d 查这条指令", ["48 8b 3c 24   mov (%rsp),%rdi"], FILL_GREY, MUTED)
    out.append(arrow(X0 + BW / 2, 296, X0 + BW / 2, 316, INK, 2))
    out += step(320, "补上这个操作码的 case", ["case 0x8b: ...    make"], FILL_GREEN, GREEN)
    out.append(elbow([(X0 + BW / 2, 398), (X0 + BW / 2, 424), (X0 - 24, 424), (X0 - 24, 53),
                      (X0 - 4, 53)], BLUE, 2))
    out.append(text(X0 + BW / 2, 452, "再运行，报告下一条没有 case 的指令", 17, BLUE, "bold"))
    return out


if __name__ == "__main__":
    save("cpu_debug", W, H, build())
