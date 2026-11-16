#!/usr/bin/env python3
"""The threads a CUDA program starts, and the Fermi GPU that runs them.

Left: a grid of thread blocks, and one block of 256 threads opened into its 8
warps of 32 threads. Right: the Fermi GPU with its thread-block scheduler
(GigaThread, purple) and 16 SMs, one of them opened into its two warp
schedulers (purple), its 32 CUDA cores in two groups of 16 (orange, one group
per scheduler) and its 64 KB of shared memory / L1 cache (grey). Below the
chip: the device memory all SMs share (GDDR5, six 64-bit partitions, up to 6
GB) and the host memory behind the PCIe bus. Two arrows carry the two rules:
the scheduler places a block on one SM as a whole, and the threads of a warp
execute the same instruction. The numbers and the structure follow NVIDIA's
Fermi Compute Architecture whitepaper. Run it to refresh ../assets/gpu-simt.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_PURPLE,
                    GREEN, INK, MUTED, ORANGE, PURPLE, WHITE, arrow, box, brace, line, mono, rect,
                    save, text)

W, H = 1158, 450            # the drawing is built on 1180 x 500 and cropped to its ink
CROP_X, CROP_Y = 11, 31
LX, LW = 20, 480            # left half: what the program starts
RX, RW = 680, 480           # right half: the hardware
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
    out.append(text(LX + 14, top + 24, "Thread Block（本例 256 个线程）：分为 8 个 Warp",
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

    # --- right: GPU -> GigaThread, SMs -> one SM opened; memories off the chip ---
    gpu_h = 330
    out.append(rect(RX, 40, RW, gpu_h, WHITE, INK, rx=6, width=1.8))
    out.append(text(RX + 14, 62, "Fermi GPU", 15, INK, "bold", "start"))
    sx, sw = RX + 16, 300
    gx1 = RX + RW - 16
    gt_y, gt_h = 72, 32
    out.append(rect(sx + 110, gt_y, gx1 - sx - 110, gt_h, FILL_PURPLE, PURPLE, rx=4, width=1.6))
    out.append(text((sx + 110 + gx1) / 2, gt_y + 21, "线程块调度器（GigaThread）", 15, INK, "bold"))

    sm_y, sm_h = 124, 234
    out.append(rect(sx, sm_y, sw, sm_h, WHITE, ORANGE, rx=6, width=1.8))
    out.append(text(sx + 12, sm_y + 22, "SM 0", 15, INK, "bold", "start"))
    half = (sw - 24 - 12) / 2
    cell_w, cell_pitch = 12, 15
    group_w = 8 * cell_pitch - (cell_pitch - cell_w)
    for k in range(2):
        hx = sx + 12 + k * (half + 12)
        out.append(rect(hx, sm_y + 32, half, 30, FILL_PURPLE, PURPLE, rx=4, width=1.6))
        out.append(text(hx + half / 2, sm_y + 52, "Warp 调度器", 15, INK, "bold"))
        out.append(arrow(hx + half / 2, sm_y + 62, hx + half / 2, sm_y + 80, INK, 1.6))
        gx = hx + (half - group_w) / 2
        for r in range(2):
            out += [rect(gx + c * cell_pitch, sm_y + 82 + r * 18, cell_w, 14, FILL_ORANGE,
                         ORANGE, rx=2, width=1.2) for c in range(8)]
        out.append(text(hx + half / 2, sm_y + 140, "16 个核心", 14, INK, "bold"))
    out.append(text(sx + sw / 2, sm_y + 160, "两个调度器各发射一个 Warp 的一条指令", 14, MUTED))
    out.append(text(sx + sw / 2, sm_y + 178, "32 个线程在 16 个核心上分两批执行", 14, MUTED))
    out += box(sx + 12, sm_y + 190, sw - 24, 36, "共享内存 / L1 高速缓存（64 KB）",
               FILL_GREY, MUTED, size=14)

    ox = sx + sw + 12
    ow = gx1 - ox
    for name, y in (("SM 1", sm_y), ("SM 2", sm_y + 48), ("SM 15", sm_y + 144)):
        out += box(ox, y, ow, 38, name, FILL_BLUE, BLUE, size=14)
    out.append(text(ox + ow / 2, sm_y + 118, "…", 18, MUTED, "bold"))
    out.append(text(ox + ow / 2, sm_y + 214, "共 16 个 SM", 13, MUTED))
    out.append(arrow(sx + sw / 2, gt_y + gt_h, sx + sw / 2, sm_y, ORANGE))
    out.append(arrow(ox + ow / 2, gt_y + gt_h, ox + ow / 2, sm_y, MUTED))

    # --- memories, off the chip ---------------------------------------------
    gpu_bottom = 40 + gpu_h
    my, mh = gpu_bottom + 52, 50
    out += box(sx, my, 230, mh, "显存（GDDR5）", FILL_GREEN, GREEN, size=14,
               sub="所有 SM 共用，最大 6 GB", sub_size=13)
    out += box(gx1 - 210, my, 210, mh, "主机内存", FILL_GREY, MUTED, size=14,
               sub="CPU 一侧", sub_size=13)
    dx = sx + 64
    out.append(arrow(dx, gpu_bottom, dx, my, GREEN, 2.4, both=True))
    out.append(text(dx + 10, gpu_bottom + 30, "6 个 64 位分区", 13, INK, anchor="start"))
    hx = gx1 - 105
    out.append(arrow(hx, gpu_bottom, hx, my, INK, both=True))
    out.append(text(hx + 10, gpu_bottom + 30, "PCIe 总线", 13, INK, anchor="start"))

    # --- the two rules, in the gap ------------------------------------------
    out.append(arrow(LX + LW + 4, 88, sx + 110 - 4, 88, ORANGE))
    out.append(text(MID, 62, "一个线程块", 14, INK, "bold"))
    out.append(text(MID, 80, "整块分配给 1 个 SM", 14, INK, "bold"))
    wy = top + 45
    out.append(arrow(LX + LW + 4, wy, sx + 18, sm_y + 90, ORANGE))
    out.append(text(MID, wy + 52, "同一个 Warp 的线程", 14, INK, "bold"))
    out.append(text(MID, wy + 70, "执行同一条指令", 14, INK, "bold"))
    return out


if __name__ == "__main__":
    save("gpu-simt", W, H, build(), left=CROP_X, top=CROP_Y)
