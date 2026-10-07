#!/usr/bin/env python3
"""The four layers of page 76 as a pyramid, C at the top, hardware at the base.

The note beside each layer is what that layer fixes in this lecture.
Run it to refresh ../assets/system-layers.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, line, mono, save, text)

W, H = 1120, 284
CX, TOPW, GROW, LH = 400, 200, 140, 66

LAYERS = [("高级语言（C/C++）", "sum += w[i] * x[i];", True, FILL_GREY, MUTED),
          ("ABI", "调用规约、寄存器保护、栈对齐", False, FILL_BLUE, BLUE),
          ("ISA", "指令编码、架构寄存器、寻址模式", False, FILL_ORANGE, ORANGE),
          ("微架构与硬件电路", "流水线、ALU / AGU、执行端口", False, FILL_GREEN, GREEN)]


def build():
    out = []
    for k, (name, note, code, fill, stroke) in enumerate(LAYERS):
        y0, y1 = 8 + k * LH, 8 + k * LH + LH - 6
        a, b = (TOPW + k * GROW) / 2, (TOPW + (k + 1) * GROW) / 2
        d = (f"M {CX - a} {y0} L {CX + a} {y0} L {CX + b} {y1} L {CX - b} {y1} Z")
        out.append(f'<path d="{d}" fill="{fill}" stroke="{stroke}" stroke-width="1.8"/>')
        out.append(text(CX, (y0 + y1) / 2 + 7, name, 19, INK, "bold"))
        out.append(line(CX + b + 10, (y0 + y1) / 2, 820, (y0 + y1) / 2, LINE, 1, "3 3"))
        if code:
            out.append(mono(830, (y0 + y1) / 2 + 6, note, 16, INK, "bold"))
        else:
            out.append(text(830, (y0 + y1) / 2 + 6, note, 15, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("system-layers", W, H, build(), left=-24)    # centres the ink on the canvas
