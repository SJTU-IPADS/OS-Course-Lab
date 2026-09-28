#!/usr/bin/env python3
"""One scalar imull and one vpmulld: same management cost, 1 vs 8 results.

Both pay fetch, decode, schedule and retire once. imull runs on the scalar
integer multiplier and yields one 32-bit product; vpmulld drives the 256-bit
vector unit and yields eight.
Run it to refresh ../assets/insn-results.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MONO, MUTED, ORANGE, WHITE, arrow, box, brace, mono,
                    rect, save, text)

W, H = 1120, 360
STAGES = ["取指", "译码", "调度", "退休"]


def lane(y, insn, sub, unit, n):
    out = [mono(20, y + 32, insn, 22, INK, "bold"),
           text(20, y + 58, sub, 14, MUTED, anchor="start")]
    for k, s in enumerate(STAGES):
        out += box(170 + k * 74, y + 10, 66, 50, s, FILL_GREY, MUTED, 15)
    out.append(arrow(466, y + 35, 500, y + 35, INK, 2))
    uw = 150 if n == 1 else 210
    out += box(504, y + 4, uw, 62, unit[0], FILL_ORANGE, ORANGE, 16, sub=unit[1])
    x = 504 + uw + 40
    out.append(arrow(504 + uw, y + 35, x - 4, y + 35, INK, 2))
    for k in range(8):
        cx = x + k * 40
        if k < n:
            out.append(rect(cx, y + 14, 38, 42, FILL_GREEN, GREEN, rx=3, width=1.6))
            out.append(mono(cx + 19, y + 41, "32", 13, INK, anchor="middle"))
    out.append(text(x, y + 88, f"{n} 个 32 位结果", 16, GREEN, "bold", anchor="start"))
    return out


def build():
    out = lane(30, "imull", "标量指令", ("标量乘法器", "32 位"), 1)
    out += lane(200, "vpmulld", "向量指令", ("256 位向量单元", "8 个 32 位通道"), 8)
    out += brace(170, 458, 170, MUTED, "每条指令的管理开销相近", 15, below=True)
    return out


if __name__ == "__main__":
    save("insn-results", W, H, build())
