#!/usr/bin/env python3
"""One streaming multiprocessor (SM) of the Fermi architecture.

Top down: the instruction cache; two warp schedulers, each with its dispatch
unit; the register file of 32768 32-bit registers; the execution units, 32 CUDA
cores, 16 load/store units (LD/ST) and 4 special function units (SFU), each
drawn; the interconnect network; 64 KB of shared memory / L1 cache. The register
file (128 KB) and the shared memory (64 KB) have the same width and heights in
the ratio 2 : 1, so their areas are in the ratio of their capacities. Fills:
the SM grey as in block-to-sm.svg, storage inside it grey frames, control logic
purple, execution units orange, as in chip-area.svg.
Redrawn after the SM figure of the NVIDIA Fermi Compute Architecture whitepaper.
Run it to refresh ../assets/sm-structure.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREY, FILL_ORANGE, FILL_PURPLE, INK, LINE, MUTED, ORANGE, PURPLE,
                    WHITE, rect, save, text)

W, H = 520, 584
X0, X1 = 20, 500                  # inner left and right edge of the SM
UNITS_Y, UNIT_ROWS, ROW_PITCH, CORE_H = 234, 8, 32, 26
UNITS_H = UNIT_ROWS * ROW_PITCH - (ROW_PITCH - CORE_H)
CORE_COLS = (20, 80, 154, 214)    # two groups of two columns of cores
CORE_W = 54
LDST = (294, 90)                  # x and width of the load/store column
SFU = (410, 90)


def store(x, y, w, h, label, sub=None, size=15):
    """Storage inside the SM: a grey frame."""
    out = [rect(x, y, w, h, FILL_GREY, MUTED, rx=4, width=1.6)]
    if sub is None:
        out.append(text(x + w / 2, y + h / 2 + size * 0.36, label, size, INK, "bold"))
    else:
        out.append(text(x + w / 2, y + h / 2 - 4, label, size, INK, "bold"))
        out.append(text(x + w / 2, y + h / 2 + 18, sub, 14, MUTED))
    return out


def control(x, y, w, h, label):
    return [rect(x, y, w, h, FILL_PURPLE, PURPLE, rx=4, width=1.6),
            text(x + w / 2, y + h / 2 + 5.5, label, 15, INK, "bold")]


def unit(x, y, w, h):
    return rect(x, y, w, h, FILL_ORANGE, ORANGE, rx=2, width=1.2)


def build():
    out = [rect(6, 6, W - 12, H - 12, FILL_GREY, MUTED, rx=6, width=1.6)]
    out.append(text(X0, 37, "SM", 18, INK, "bold", "start"))
    out += store(64, 16, X1 - 64, 30, "指令缓存")
    half = (X1 - X0 - 12) / 2
    for k in range(2):
        x = X0 + k * (half + 12)
        out += control(x, 56, half, 30, "Warp 调度器")
        out += control(x, 92, half, 30, "分派单元")
    out += store(X0, 134, X1 - X0, 64, "寄存器堆", "32768 × 32 位（128 KB）")

    # --- the execution units ------------------------------------------------
    hy = UNITS_Y - 10
    cores_mid = (CORE_COLS[0] + CORE_COLS[-1] + CORE_W) / 2
    out.append(text(cores_mid, hy, "32 个 CUDA 核心", 15, INK, "bold"))
    out.append(text(LDST[0] + LDST[1] / 2, hy, "16 个 LD/ST", 15, INK, "bold"))
    out.append(text(SFU[0] + SFU[1] / 2, hy, "4 个 SFU", 15, INK, "bold"))
    for cx in CORE_COLS:
        for r in range(UNIT_ROWS):
            out.append(unit(cx, UNITS_Y + r * ROW_PITCH, CORE_W, CORE_H))
    pitch = (UNITS_H + 4) / 16
    for r in range(16):
        out.append(unit(LDST[0], UNITS_Y + r * pitch, LDST[1], pitch - 4))
    pitch = (UNITS_H + 8) / 4
    for r in range(4):
        out.append(unit(SFU[0], UNITS_Y + r * pitch, SFU[1], pitch - 8))

    y = UNITS_Y + UNITS_H + 10
    out.append(rect(X0, y, X1 - X0, 26, WHITE, LINE, rx=4, width=1.4))
    out.append(text((X0 + X1) / 2, y + 18, "互连网络", 14, INK, "bold"))
    out += store(X0, y + 36, X1 - X0, 32, "共享内存 / L1 高速缓存（64 KB）")
    return out


if __name__ == "__main__":
    save("sm-structure", W, H, build())
