#!/usr/bin/env python3
"""How a CPU die and a GPU die spend their area, drawn schematically.

CPU: a large control block and cache, a few wide ALUs. GPU: rows of small
cores, each row with a thin strip of control and cache.
Run it to refresh ../assets/cpu-gpu-area.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, rect, save, text)

W, H = 1120, 210


def build():
    out = [text(270, 22, "CPU", 18, INK, "bold"), text(700, 22, "GPU", 18, INK, "bold")]
    # CPU
    out.append(rect(40, 34, 460, 170, WHITE, INK, rx=6, width=1.8))
    out.append(rect(50, 44, 210, 80, FILL_GREY, MUTED, rx=3))
    out.append(text(155, 90, "控制逻辑", 16, INK, "bold"))
    for k in range(4):
        x = 270 + (k % 2) * 112
        y = 44 + (k // 2) * 42
        out.append(rect(x, y, 104, 36, FILL_GREEN, GREEN, rx=3))
        out.append(text(x + 52, y + 24, "ALU", 15, INK, "bold"))
    out.append(rect(50, 132, 440, 62, FILL_BLUE, BLUE, rx=3))
    out.append(text(270, 169, "大容量 Cache", 16, INK, "bold"))
    # GPU
    out.append(rect(600, 34, 480, 170, WHITE, INK, rx=6, width=1.8))
    for r in range(6):
        y = 42 + r * 26
        out.append(rect(608, y, 22, 22, FILL_GREY, MUTED, rx=2, width=1))
        out.append(rect(632, y, 22, 22, FILL_BLUE, BLUE, rx=2, width=1))
        for c in range(17):
            out.append(rect(658 + c * 24.5, y, 21, 22, FILL_GREEN, GREEN, rx=2, width=1))
    for k, (name, fill, stroke) in enumerate([("控制", FILL_GREY, MUTED),
                                              ("Cache", FILL_BLUE, BLUE),
                                              ("运算单元", FILL_GREEN, GREEN)]):
        x = 900 + k * 70 - (10 if k == 2 else 0)
        out.append(rect(x - 30, 10, 14, 14, fill, stroke, rx=2, width=1))
        out.append(text(x - 12, 22, name, 13, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("cpu-gpu-area", W, H, build())
