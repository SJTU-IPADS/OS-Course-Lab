#!/usr/bin/env python3
"""The machine of quiz question 5 (2): GPUs in a chain, each with its own memory.

Each GPU reads weights from its own 80 GB of GPU memory at 3350 GB/s (the
orange links); the GPUs pass data to the next one along the chain, and the
time of that transfer is ignored. The number of GPUs is left open, because
the question asks how many are needed.
Uses the deck's svgkit, so the style is the deck's.
Run it to refresh ../assets/q-gpu-memory.svg.
"""

import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from svgkit import BLUE, FILL_BLUE, FILL_GREY, INK, MUTED, ORANGE, arrow, box, save, text

W, H = 820, 172
BW, GAP = 170, 150
X0 = 40
MEM_Y, MEM_H = 8, 56
GPU_Y, GPU_H = 112, 52


def build():
    out = []
    for k in range(2):
        x = X0 + k * (BW + GAP)
        out += box(x, MEM_Y, BW, MEM_H, "显存", FILL_GREY, INK, size=17,
                   sub="容量 80 GB", sub_size=14)
        out.append(arrow(x + BW / 2, MEM_Y + MEM_H + 2, x + BW / 2, GPU_Y - 2, ORANGE, 3))
        out.append(text(x + BW / 2 + 12, (MEM_Y + MEM_H + GPU_Y) / 2 + 6,
                        "带宽 3350 GB/s", 16, ORANGE, "bold", anchor="start"))
        out += box(x, GPU_Y, BW, GPU_H, f"GPU {k + 1}", FILL_BLUE, BLUE, size=18)
        # each GPU passes its result to the next one
        nx = x + BW + GAP
        out.append(arrow(x + BW + 2, GPU_Y + GPU_H / 2, nx - 2, GPU_Y + GPU_H / 2, INK, 2))
        if k == 0:
            out.append(text(x + BW + GAP / 2, GPU_Y + GPU_H / 2 - 10, "传递数据的时间忽略", 14,
                            MUTED))
    right = X0 + 2 * (BW + GAP)
    out.append(text(right + 20, GPU_Y + GPU_H / 2 + 8, "⋯", 24, BLUE, "bold", anchor="start"))
    out.append(text(right + 40, MEM_Y + MEM_H / 2 + 6, "各块 GPU", 15, BLUE, anchor="middle"))
    out.append(text(right + 40, MEM_Y + MEM_H / 2 + 28, "参数相同", 15, BLUE, anchor="middle"))
    return out


if __name__ == "__main__":
    save("q-gpu-memory", W, H, build())
