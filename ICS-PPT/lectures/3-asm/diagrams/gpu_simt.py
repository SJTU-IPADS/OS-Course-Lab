#!/usr/bin/env python3
"""The threads a CUDA program starts, and the GPU hardware that runs them.

Left: a grid of thread blocks, and one block opened into its warps of 32
threads. Right: the GPU as 170 SMs (one opened into its shared memory and its
128 cores, which execute 4 warps at a time), the device memory all SMs share, and the host memory behind the
PCIe bus. Two arrows carry the two rules: a block is placed on one SM as a
whole, and the 32 threads of a warp execute the same instruction at the same
time. The numbers are those of the RTX 5090 in NVIDIA's specification: 21760
CUDA cores (170 SMs of 128) and 32 GB of device memory.
Run it to refresh ../assets/gpu-simt.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, MUTED, ORANGE, WHITE, arrow, box, brace, line, mono, rect,
                    save, text)

W, H = 1120, 440
LX, LW = 20, 480            # left half: what the program starts
RX, RW = 640, 460           # right half: the hardware
MID = (LX + LW + RX) / 2    # the gap between them, where the arrow labels sit


def threads(x, y, n, fill, stroke, pitch=12, size=10):
    return [rect(x + k * pitch, y, size, size, fill, stroke, rx=1.5, width=1)
            for k in range(n)]


def build():
    out = []

    # --- left: Grid -> Thread Block -> Warp -> Thread -----------------------
    out.append(rect(LX, 40, LW, 80, WHITE, INK, rx=6, width=1.8))
    out.append(text(LX + 14, 62, "Grid：一次启动的全部线程块", 15, INK, "bold", "start"))
    bx = LX + 20
    for k in range(4):
        out += box(bx + k * 96, 74, 86, 34, f"Block {k}",
                   FILL_ORANGE if k == 0 else FILL_BLUE,
                   ORANGE if k == 0 else BLUE, size=14)
    out.append(text(bx + 4 * 96 + 26, 97, "…", 18, MUTED, "bold"))

    top = 162
    out.append(rect(LX, top, LW, 196, WHITE, ORANGE, rx=6, width=1.8))
    out.append(text(LX + 14, top + 24, "Thread Block：256 个线程，分为 8 个 Warp",
                    15, INK, "bold", "start"))
    # the opened block hangs from Block 0
    out.append(line(bx, 108, LX, top, ORANGE, 1.2, dash="4 3"))
    out.append(line(bx + 86, 108, LX + LW, top, ORANGE, 1.2, dash="4 3"))
    tx = LX + 84
    rows = [("Warp 0", top + 40, True), ("Warp 1", top + 70, False),
            ("Warp 7", top + 130, False)]
    for name, y, first in rows:
        out.append(mono(LX + 16, y + 10, name, 14, INK, "bold"))
        out += threads(tx, y, 32, FILL_ORANGE if first else FILL_BLUE,
                       ORANGE if first else BLUE)
    out.append(text(tx + 32 * 12 / 2, top + 116, "…", 18, MUTED, "bold"))
    out += brace(tx, tx + 32 * 12 - 2, top + 146, MUTED, "每个 Warp 32 个线程", 14)

    out += threads(LX + 16, 392, 1, FILL_BLUE, BLUE)
    out.append(text(LX + 34, 402, "1 个线程（Thread）：执行一遍 kernel 函数", 14, MUTED,
                    anchor="start"))

    # --- right: GPU -> SM -> cores and shared memory; device memory ---------
    out.append(rect(RX, 40, RW, 300, WHITE, INK, rx=6, width=1.8))
    out.append(text(RX + 14, 62, "GPU：RTX 5090", 15, INK, "bold", "start"))
    sx, sw = RX + 16, 272
    out.append(rect(sx, 74, sw, 200, WHITE, ORANGE, rx=6, width=1.8))
    out.append(text(sx + 12, 96, "SM 0", 15, INK, "bold", "start"))
    out += box(sx + 12, 106, sw - 24, 34, "共享内存", FILL_GREY, MUTED, size=14)
    cx, cy = sx + 12, 164
    for r in range(4):
        out += [rect(cx + k * 7.75, cy + r * 13, 6, 10,
                     FILL_ORANGE if r == 0 else FILL_GREEN,
                     ORANGE if r == 0 else GREEN, rx=1, width=1) for k in range(32)]
    out.append(text(sx + sw / 2, 240, "128 个运算核心", 14, INK, "bold"))
    out.append(text(sx + sw / 2, 260, "每个时刻执行 4 个 Warp，其余 Warp 等待", 13, MUTED))
    ox, ow = sx + sw + 14, RW - 16 - sw - 14 - 16
    for name, y in (("SM 1", 74), ("SM 2", 120), ("SM 169", 212)):
        out += box(ox, y, ow, 38, name, FILL_BLUE, BLUE, size=14)
    out.append(text(ox + ow / 2, 192, "…", 18, MUTED, "bold"))
    out.append(text(ox + ow / 2, 270, "共 170 个 SM", 13, MUTED))
    out += box(sx, 288, RW - 32, 38, "显存 32 GB：所有 SM 共用", FILL_GREEN, GREEN, size=14)
    out += box(sx, 388, RW - 32, 38, "主机内存：CPU 一侧", FILL_GREY, MUTED, size=14)
    mx = RX + RW / 2
    out.append(arrow(mx, 342, mx, 386, INK, both=True))
    out.append(text(mx + 12, 369, "PCIe 总线：拷贝数据", 14, INK, anchor="start"))

    # --- the two rules, in the gap ------------------------------------------
    out.append(arrow(LX + LW + 4, 92, sx - 4, 92, ORANGE))
    out.append(text(MID, 66, "一个线程块", 14, INK, "bold"))
    out.append(text(MID, 84, "整块分配给 1 个 SM", 14, INK, "bold"))
    wy = top + 45
    out.append(arrow(LX + LW + 4, wy, sx + 8, wy - 24, ORANGE))
    out.append(text(MID, wy + 30, "同一个 Warp 的线程", 14, INK, "bold"))
    out.append(text(MID, wy + 48, "同时执行同一条指令", 14, INK, "bold"))
    return out


if __name__ == "__main__":
    save("gpu-simt", W, H, build())
