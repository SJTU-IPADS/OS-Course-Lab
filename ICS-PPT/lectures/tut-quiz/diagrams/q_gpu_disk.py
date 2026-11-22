#!/usr/bin/env python3
"""The machine of quiz question 5 (3): one GPU whose weights stay on a disk.

The disk holds all the weights and sends them into the GPU memory at
13.5 GB/s; the GPU reads them from its 80 GB of GPU memory at 3350 GB/s.
The links are orange, as in q-gpu-memory.svg of question 5 (2).
Uses the deck's svgkit, so the style is the deck's.
Run it to refresh ../assets/q-gpu-disk.svg.
"""

import pathlib
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from svgkit import BLUE, FILL_BLUE, FILL_GREY, INK, ORANGE, arrow, box, save, text

W, H = 620, 172
BW = 170
DISK_X, MEM_X = 40, 360
MEM_Y, MEM_H = 8, 56
GPU_Y, GPU_H = 112, 52


def build():
    out = box(DISK_X, MEM_Y, BW, MEM_H, "磁盘", FILL_GREY, INK, size=17,
              sub="存放全部权重", sub_size=14)
    mid = MEM_Y + MEM_H / 2
    out.append(arrow(DISK_X + BW + 2, mid, MEM_X - 2, mid, ORANGE, 3))
    out.append(text((DISK_X + BW + MEM_X) / 2, mid - 12, "带宽 13.5 GB/s", 16, ORANGE,
                    "bold"))
    out += box(MEM_X, MEM_Y, BW, MEM_H, "显存", FILL_GREY, INK, size=17,
               sub="容量 80 GB", sub_size=14)
    out.append(arrow(MEM_X + BW / 2, MEM_Y + MEM_H + 2, MEM_X + BW / 2, GPU_Y - 2, ORANGE, 3))
    out.append(text(MEM_X + BW / 2 + 12, (MEM_Y + MEM_H + GPU_Y) / 2 + 6,
                    "带宽 3350 GB/s", 16, ORANGE, "bold", anchor="start"))
    out += box(MEM_X, GPU_Y, BW, GPU_H, "GPU", FILL_BLUE, BLUE, size=18)
    return out


if __name__ == "__main__":
    save("q-gpu-disk", W, H, build())
