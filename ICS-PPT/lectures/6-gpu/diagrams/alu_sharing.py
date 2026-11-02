#!/usr/bin/env python3
"""How many ALUs one fetch-and-decode unit drives, in three kinds of core.

A scalar CPU core decodes one instruction for 1 ALU, a SIMD CPU core for 8
ALUs (one AVX instruction on 8 floats), a GPU for 32 ALUs. The fetch-and-decode
unit and the ALU are drawn the same size in all three. Fills as in
chip-area.svg, which stands beside this figure on the slide.
Run it to refresh ../assets/alu-sharing.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_ORANGE, FILL_PURPLE, INK, LINE, MUTED, ORANGE, PURPLE, WHITE, arrow,
                    line, rect, save, text)

W, H = 552, 268
PANELS = [  # (title, ALU rows, ALUs per row, panel width)
    ("标量 CPU 核心", 1, 1, 128),
    ("SIMD CPU 核心", 1, 8, 194),
    ("GPU", 4, 8, 194),
]
GAP = 12
CTRL_W, CTRL_H, CTRL_Y = 92, 34, 46
CELL, PITCH, FPAD = 18, 22, 8       # an ALU, its pitch, padding of the group frame
GROUP_Y = 116


def panel(x, w, title, rows, cols):
    cx = x + w / 2
    out = [text(cx, 20, title, 17, INK, "bold")]
    out.append(rect(cx - CTRL_W / 2, CTRL_Y, CTRL_W, CTRL_H, FILL_PURPLE, PURPLE, rx=4,
                    width=1.6))
    out.append(text(cx, CTRL_Y + CTRL_H / 2 + 5, "取指与译码", 14, INK, "bold"))
    gw = cols * PITCH - (PITCH - CELL) + 2 * FPAD
    gh = rows * PITCH - (PITCH - CELL) + 2 * FPAD
    gx = cx - gw / 2
    out.append(arrow(cx, CTRL_Y + CTRL_H, cx, GROUP_Y - 3, INK, 2))
    out.append(rect(gx, GROUP_Y, gw, gh, WHITE, LINE, rx=4, width=1.2))
    for r in range(rows):
        for c in range(cols):
            out.append(rect(gx + FPAD + c * PITCH, GROUP_Y + FPAD + r * PITCH, CELL, CELL,
                            FILL_ORANGE, ORANGE, rx=2, width=1.2))
    out.append(text(cx, GROUP_Y + gh + 24, f"{rows * cols} 个 ALU", 15, MUTED, "bold"))
    return out


def build():
    out = []
    x = (W - sum(p[3] for p in PANELS) - GAP * (len(PANELS) - 1)) / 2
    for k, (title, rows, cols, w) in enumerate(PANELS):
        if k:
            out.append(line(x - GAP / 2, 12, x - GAP / 2, H - 14, LINE, 1))
        out += panel(x, w, title, rows, cols)
        x += w + GAP
    return out


if __name__ == "__main__":
    save("alu-sharing", W, H, build())
