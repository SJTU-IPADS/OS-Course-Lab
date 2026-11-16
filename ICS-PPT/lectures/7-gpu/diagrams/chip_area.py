#!/usr/bin/env python3
"""How a CPU and a GPU divide their chip area.

Two chips of the same size, drawn to the same scale. The CPU (left) has four
cores, each with its ALUs, its control logic and an L1 cache; an L2 cache per
pair of cores, a shared L3 cache and the DRAM interface. The GPU (right) is
rows of small ALUs; each row shares one small control unit and one small L1
cache; under them are an L2 cache and the same DRAM interface. The areas are
the content of the figure: the ALUs take about 17 % of the CPU and 74 % of the
GPU, control logic 22 % and 3 %, caches 46 % and 15 %, the DRAM interface 15 %
of both. Redrawn after the figure "The GPU Devotes More Transistors to Data
Processing" of the NVIDIA CUDA C++ Programming Guide.
Run it to refresh ../assets/chip-area.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREY, FILL_ORANGE, FILL_PURPLE, GREEN, INK, MUTED, ORANGE, PURPLE,
                    WHITE, _advance, FONT, rect, save, text)

W, H = 540, 268
PW, PH = 240, 200                 # one chip
PX, PY = (20, 280), 30            # left edges of the two chips, their top
PAD, GAP = 6, 4                   # inset of the regions, gap between bands
IW = PW - 2 * PAD                 # inner width
IFACE_H, L2_H = 24, 20

ALU = (FILL_ORANGE, ORANGE)
CTRL = (FILL_PURPLE, PURPLE)
CACHE = (FILL_GREY, MUTED)
IFACE = (WHITE, GREEN)


def region(x, y, w, h, kind, label=None, size=13):
    fill, stroke = kind
    out = [rect(x, y, w, h, fill, stroke, rx=2, width=1.2)]
    if label:
        out.append(text(x + w / 2, y + h / 2 + size * 0.36, label, size, INK))
    return out


def cpu(x0):
    out = []
    x, y = x0 + PAD, PY + PAD
    core_w = (IW - GAP) / 2
    alu_w, core_h, l1_h = 48, 34, 14
    for row in range(2):
        for col in range(2):
            cx = x + col * (core_w + GAP)
            cy = y + row * (core_h + 2 + l1_h + GAP)
            out += region(cx, cy, alu_w, core_h, ALU, "ALU")
            out += region(cx + alu_w + 2, cy, core_w - alu_w - 2, core_h, CTRL, "控制")
            out += region(cx, cy + core_h + 2, core_w, l1_h, CACHE, "L1", 12)
    y += 2 * (core_h + 2 + l1_h + GAP)
    for col in range(2):
        out += region(x + col * (core_w + GAP), y, core_w, L2_H, CACHE, "L2")
    y += L2_H + GAP
    l3_h = PY + PH - PAD - IFACE_H - GAP - y
    out += region(x, y, IW, l3_h, CACHE, "L3")
    out += region(x, y + l3_h + GAP, IW, IFACE_H, IFACE, "DRAM 接口")
    return out


def gpu(x0):
    out = []
    x, y = x0 + PAD, PY + PAD
    rows, cols, side = 10, 16, 8        # rows of ALUs, ALUs per row, width of control and L1
    array_h = PH - 2 * PAD - IFACE_H - L2_H - 2 * GAP
    rh = array_h / rows
    aw = (IW - 2 * side - 2) / cols
    for r in range(rows):
        ry = y + r * rh
        out.append(rect(x, ry, side, rh, *CTRL, rx=0, width=0.8))
        out.append(rect(x + side, ry, side, rh, *CACHE, rx=0, width=0.8))
        for c in range(cols):
            out.append(rect(x + 2 * side + 2 + c * aw, ry, aw, rh, *ALU, rx=0, width=0.8))
    y += array_h + GAP
    out += region(x, y, IW, L2_H, CACHE, "L2")
    out += region(x, y + L2_H + GAP, IW, IFACE_H, IFACE, "DRAM 接口")
    return out


def legend(y):
    items = [(ALU, "运算单元（ALU）"), (CTRL, "控制逻辑"), (CACHE, "高速缓存"),
             (IFACE, "DRAM 接口")]
    size, sw, gap = 14, 22, 18
    total = sum(sw + 6 + _advance(s, size, FONT) for _, s in items) + gap * (len(items) - 1)
    x = (W - total) / 2
    out = []
    for (fill, stroke), s in items:
        out.append(rect(x, y - 12, sw, 14, fill, stroke, rx=2, width=1.2))
        out.append(text(x + sw + 6, y, s, size, MUTED, anchor="start"))
        x += sw + 6 + _advance(s, size, FONT) + gap
    return out


def build():
    out = []
    for x0, name, part in ((PX[0], "CPU", cpu), (PX[1], "GPU", gpu)):
        out.append(text(x0 + PW / 2, PY - 10, name, 17, INK, "bold"))
        out.append(rect(x0, PY, PW, PH, WHITE, INK, rx=6, width=1.6))
        out += part(x0)
    out += legend(PY + PH + 28)
    return out


if __name__ == "__main__":
    save("chip-area", W, H, build())
