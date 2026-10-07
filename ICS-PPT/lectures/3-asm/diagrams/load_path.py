#!/usr/bin/env python3
"""movl (%rsi), %eax as a path: address, TLB, L1 cache, read port, register.

The steps are numbered in the order they happen. DRAM hangs below the L1
cache on the miss path. The register at the end is drawn as all 64 bits of
%rax, so the zeroed upper half is visible. Bottom left is the form the
constraint on page 18 forbids: two memory operands in one mov.
Run it to refresh ../assets/load-path.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, FILL_RED, INK, LINE,
                    MUTED, ORANGE, RED, WHITE, arrow, box, circle, elbow, line,
                    mono, rect, save, text)

W, H = 1120, 256

STAGES = [(20, 180, "%rsi", "地址 = x", FILL_BLUE, BLUE),
          (250, 200, "TLB", "地址转换", WHITE, BLUE),
          (500, 170, "L1 Cache", "命中：直接读出", FILL_BLUE, BLUE),
          (720, 190, "读数据通路", "32 位数据 x[0]", FILL_ORANGE, ORANGE)]


def build():
    out = []
    for k, (x, w, label, sub, fill, stroke) in enumerate(STAGES):
        out += box(x, 24, w, 70, label, fill, stroke, 18, sub=sub, sub_size=15)
        out.append(circle(x + 2, 26, 14, WHITE, INK, 1.6))
        out.append(text(x + 2, 32, "①②③④"[k], 16, INK, "bold"))
        if k < len(STAGES) - 1:
            nx = STAGES[k + 1][0]
            out.append(arrow(x + w + 4, 59, nx - 18, 59, INK, 2.2))

    # the miss path to DRAM
    out += box(500, 160, 170, 56, "DRAM", FILL_GREY, MUTED, 18)
    out.append(arrow(555, 98, 555, 156, MUTED, 1.8, "6 4"))
    out.append(arrow(615, 156, 615, 98, MUTED, 1.8, "6 4"))
    out.append(text(548, 134, "未命中", 14, MUTED, anchor="end"))

    # %rax: upper half cleared, lower half is %eax
    x0, x1, y = 730, 1100, 160
    mid = (x0 + x1) / 2
    out.append(rect(x0, y, mid - x0, 48, FILL_GREY, MUTED, rx=2, width=1.6))
    out.append(rect(mid, y, x1 - mid, 48, FILL_ORANGE, ORANGE, rx=2, width=1.6))
    out.append(text((x0 + mid) / 2, y + 30, "高 32 位：清零", 16, INK))
    out.append(text((mid + x1) / 2, y + 30, "%eax = x[0]", 16, INK, "bold"))
    out.append(mono(x0, y + 68, "%rax", 15, MUTED, "bold"))
    out.append(mono(mid + 2, y + 68, "31", 14, MUTED))
    out.append(mono(x1, y + 68, "0", 14, MUTED, anchor="end"))
    out.append(elbow([(1008, 98), (1008, 156)], ORANGE, 2.4))
    out.append(text(1016, 134, "写回", 15, ORANGE, "bold", anchor="start"))

    # the illegal form
    out.append(rect(20, 140, 420, 96, FILL_RED, RED, rx=8, width=1.6))
    out.append(mono(40, 180, "movl (%rsi), (%rdi)", 20, INK, "bold"))
    out.append(line(36, 174, 290, 174, RED, 2.4))
    out.append(text(40, 214, "非法：源与目的不能同时为内存", 15, RED, "bold",
                    anchor="start"))
    cx, cy = 380, 188
    out.append(line(cx - 20, cy - 20, cx + 20, cy + 20, RED, 5))
    out.append(line(cx - 20, cy + 20, cx + 20, cy - 20, RED, 5))
    return out


if __name__ == "__main__":
    save("load-path", W, H, build())
