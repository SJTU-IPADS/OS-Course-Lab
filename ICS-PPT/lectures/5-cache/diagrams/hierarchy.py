#!/usr/bin/env python3
"""The memory hierarchy as a stack of six levels, smallest and fastest on top.

Levels L0 registers, L1-L3 caches (SRAM, blue), L4 main memory (DRAM, green),
L5 local disk (grey). Each row says what the level holds: the data fetched from
the level below it. The two arrows on the left give the direction of speed,
size and cost per byte.
Run it to refresh ../assets/hierarchy.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, GREEN, INK, MUTED, arrow, rect, save,
                    text)

W, H = 980, 384
CX = 400                                  # centre line of the stack
ROW_H, GAP, TOP = 52, 8, 14
LEVELS = [
    ("L0  寄存器", FILL_BLUE, INK, "保存从 L1 缓存取出的字"),
    ("L1  缓存（SRAM）", FILL_BLUE, INK, "保存从 L2 缓存取出的块"),
    ("L2  缓存（SRAM）", FILL_BLUE, INK, "保存从 L3 缓存取出的块"),
    ("L3  缓存（SRAM）", FILL_BLUE, INK, "保存从内存取出的块"),
    ("L4  内存（DRAM）", FILL_GREEN, GREEN, "保存从磁盘取出的数据"),
    ("L5  本地磁盘", FILL_GREY, MUTED, "保存程序与文件的全部数据"),
]


def build():
    out = []
    n = len(LEVELS)
    for k, (label, fill, stroke, note) in enumerate(LEVELS):
        w = 150 + 50 * k
        y = TOP + k * (ROW_H + GAP)
        out.append(rect(CX - w / 2, y, w, ROW_H, fill, stroke, rx=6, width=1.8))
        out.append(text(CX, y + ROW_H / 2 + 6, label, 18, INK, "bold"))
        out.append(text(CX + 200 + 22, y + ROW_H / 2 + 6, note, 17, INK, anchor="start"))
    bottom = TOP + n * (ROW_H + GAP) - GAP
    # direction of speed, size and cost
    x = 62
    out.append(arrow(x, bottom - 6, x, TOP + 6, MUTED, 2.2))
    out.append(arrow(x, TOP + 6, x, bottom - 6, MUTED, 2.2))
    out.append(text(x + 18, TOP + 40, "更快", 16, MUTED, "bold", "start"))
    out.append(text(x + 18, TOP + 64, "更小", 16, MUTED, "bold", "start"))
    out.append(text(x + 18, TOP + 88, "每字节更贵", 16, MUTED, "bold", "start"))
    out.append(text(x + 18, bottom - 80, "更慢", 16, MUTED, "bold", "start"))
    out.append(text(x + 18, bottom - 56, "更大", 16, MUTED, "bold", "start"))
    out.append(text(x + 18, bottom - 32, "每字节更便宜", 16, MUTED, "bold", "start"))
    return out


if __name__ == "__main__":
    save("hierarchy", W, H, build(), left=-50)
