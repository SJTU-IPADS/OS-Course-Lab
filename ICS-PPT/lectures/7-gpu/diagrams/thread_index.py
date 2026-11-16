#!/usr/bin/env python3
"""The index of a thread in a one-dimensional grid, and the element it computes.

The grid has 4 blocks of blockDim.x = 8 threads; n = 29. Each cell inside a
block is a thread, labelled with its threadIdx.x; under it is its index in the
grid, i = blockIdx.x * blockDim.x + threadIdx.x, and the element c[i] it
writes. Thread 3 of block 2 is the thread being followed (orange). The last 3
threads of block 3 have i >= n and write nothing (grey, dashed).
Run it to refresh ../assets/thread-index.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE,
                    WHITE, MONO, mono, rect, save, text)

W, H = 1110, 262
BLOCKS, DIM, N = 4, 8, 29
CW, PAD, GAP = 26, 10, 18             # thread cell, padding inside a block, gap between blocks
BW = DIM * CW + 2 * PAD
X0 = 120                              # left edge of block 0
BOX_TOP, CELL_TOP, CH = 12, 46, 32
I_Y, C_TOP = 112, 124
FOLLOW = (2, 3)                       # (blockIdx.x, threadIdx.x) of the orange thread


def cell_x(b, t):
    return X0 + b * (BW + GAP) + PAD + t * CW


def build():
    out = []
    out.append(text(X0 - 16, CELL_TOP + 21, "threadIdx.x", 15, MUTED, anchor="end", font=MONO))
    out.append(text(X0 - 16, I_Y + 4, "i", 15, MUTED, anchor="end", font=MONO))
    out.append(text(X0 - 16, C_TOP + 21, "c[i]", 15, MUTED, anchor="end", font=MONO))
    for b in range(BLOCKS):
        bx = X0 + b * (BW + GAP)
        out.append(rect(bx, BOX_TOP, BW, CELL_TOP + CH + 10 - BOX_TOP, WHITE, BLUE, rx=6,
                        width=1.6))
        out.append(mono(bx + BW / 2, BOX_TOP + 22, f"blockIdx.x = {b}", 15, INK, "bold",
                        "middle"))
        for t in range(DIM):
            i = b * DIM + t
            x = cell_x(b, t)
            past = i >= N
            follow = (b, t) == FOLLOW
            fill = WHITE if past else FILL_ORANGE if follow else FILL_BLUE
            stroke = LINE if past else ORANGE if follow else BLUE
            out.append(rect(x, CELL_TOP, CW, CH, fill, stroke, rx=2, width=1.2,
                            dash="4 3" if past else None))
            out.append(mono(x + CW / 2, CELL_TOP + 21, str(t), 14,
                            MUTED if past else INK, anchor="middle"))
            out.append(mono(x + CW / 2, I_Y + 4, str(i), 13,
                            MUTED if past else ORANGE if follow else INK,
                            "bold" if follow else "normal", "middle"))
            if not past:
                out.append(rect(x, C_TOP, CW, CH, FILL_ORANGE if follow else WHITE,
                                ORANGE if follow else INK, rx=0, width=1.2))
    # the threads past n
    px = cell_x(3, 5)
    out.append(text(px + 1.5 * CW, C_TOP + 22, "i ≥ n", 14, MUTED, "bold"))

    y = 190
    out.append(text(X0, y, "线程 (blockIdx.x = 2, threadIdx.x = 3)：i = 2 × 8 + 3 = 19，"
                    "计算 c[19]", 17, ORANGE, "bold", "start"))
    out.append(text(X0, y + 34, "blockDim.x = 8，n = 29：4 个线程块共 32 个线程，"
                    "最后 3 个线程的 i ≥ n，不访问数组", 16, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("thread-index", W, H, build())
