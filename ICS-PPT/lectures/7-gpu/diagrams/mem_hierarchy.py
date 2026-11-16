#!/usr/bin/env python3
"""The memory spaces of the CUDA thread hierarchy, and where each one lies.

Left: a thread (orange), a thread block (its threads blue, the thread on the
left orange), a grid of thread blocks. Middle: the memory space each of them
uses: a thread its registers and its local memory, a block its shared memory,
the grid the global memory. Right: where the space lies. On the chip (white,
dark outline): two of the SMs, each with its register file and its shared
memory / L1 cache (grey frames), and the L2 cache that all SMs share. Off the
chip: the device memory (green), which holds the global memory and the local
memory. The connection from local memory to device memory crosses the one from
shared memory to the SM: local memory belongs to one thread and lies off chip.
Redrawn after the memory hierarchy figure of Xingda Wei's GPU slides.
Run it to refresh ../assets/mem-hierarchy.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE,
                    MUTED, ORANGE, WHITE, arrow, box, elbow, rect, save, text)

W, H = 720, 420
EX = 82                            # left edge of the entity drawings
SX, SW, SH = 262, 92, 28           # the memory-space boxes
CHIP = (400, 26, 312, 258)         # x, y, w, h
SM_Y, SM_H, SM_W = 56, 170, 138
REG = (86, 36)                     # top and height of the register file in an SM
SHARED = (152, 64)                 # top and height of the shared memory / L1
DRAM = (316, 94)                   # top and height of the device memory
CHANNEL = 378                      # x of the line from local memory down to the DRAM

SPACES = {"寄存器": REG[0] + REG[1] / 2, "本地内存": 140, "共享内存": SHARED[0] + SHARED[1] / 2,
          "全局内存": 380}         # centre y of each memory-space box
LOCAL_IN = DRAM[0] + 20            # where the local-memory line enters the DRAM


def entities():
    out = []
    # a thread
    ty = (SPACES["寄存器"] + SPACES["本地内存"]) / 2
    out.append(text(EX - 12, ty + 6, "线程", 16, INK, "bold", "end"))
    out.append(rect(EX, ty - 14, 16, 28, FILL_ORANGE, ORANGE, rx=2, width=1.2))
    for space in ("寄存器", "本地内存"):
        out.append(arrow(EX + 18, ty, SX - 3, SPACES[space], INK, 1.6))
    # a thread block
    by = SPACES["共享内存"] + 14
    out.append(text(EX - 12, by + 6, "线程块", 16, INK, "bold", "end"))
    bw = 8 * 17 - 3 + 16
    out.append(rect(EX, by - 18, bw, 36, WHITE, BLUE, rx=4, width=1.6))
    for k in range(8):
        fill, stroke = (FILL_ORANGE, ORANGE) if k == 0 else (FILL_BLUE, BLUE)
        out.append(rect(EX + 8 + k * 17, by - 11, 14, 22, fill, stroke, rx=2, width=1.2))
    out.append(arrow(EX + bw + 2, by, SX - 3, SPACES["共享内存"], INK, 1.6))
    # the grid
    gy = SPACES["全局内存"]
    out.append(text(EX - 12, gy + 6, "Grid", 16, INK, "bold", "end"))
    for k in range(4):
        out += box(EX + (k % 2) * 74, gy - 30 + (k // 2) * 34, 66, 26, f"Block {k}",
                   FILL_BLUE, BLUE, size=13, weight="normal", rx=3)
    out.append(arrow(EX + 142, gy, SX - 3, gy, INK, 1.6))
    return out


def spaces():
    out = []
    for name, y in SPACES.items():
        out += box(SX, y - SH / 2, SW, SH, name, WHITE, LINE, size=15)
    return out


def hardware():
    out = []
    cx, cy, cw, ch = CHIP
    out.append(rect(cx, cy, cw, ch, WHITE, INK, rx=6, width=1.6))
    out.append(text(cx + 12, cy + 20, "GPU 芯片（片上）", 14, MUTED, "bold", "start"))
    for k in range(2):
        x = cx + 12 + k * (SM_W + 12)
        out.append(rect(x, SM_Y, SM_W, SM_H, FILL_GREY, MUTED, rx=5, width=1.4))
        out.append(text(x + SM_W / 2, SM_Y + 20, f"SM {k}", 15, INK, "bold"))
        out += box(x + 10, REG[0], SM_W - 20, REG[1], "寄存器堆", FILL_GREY, MUTED, size=14)
        out.append(rect(x + 10, SHARED[0], SM_W - 20, SHARED[1], FILL_GREY, MUTED, rx=6,
                        width=1.6))
        mid = SHARED[0] + SHARED[1] / 2
        out.append(text(x + SM_W / 2, mid - 4, "共享内存 /", 14, INK, "bold"))
        out.append(text(x + SM_W / 2, mid + 16, "L1 高速缓存", 14, INK, "bold"))
    l2y = SM_Y + SM_H + 12
    out += box(cx + 12, l2y, cw - 24, cy + ch - 12 - l2y, "L2 高速缓存（所有 SM 共用）",
               FILL_GREY, MUTED, size=15)
    out += box(cx, DRAM[0], cw, DRAM[1], "显存（DRAM，片外）", FILL_GREEN, GREEN, size=15)

    # the memory spaces -> where they lie
    sm0 = cx + 12 + 10
    out.append(arrow(SX + SW, SPACES["寄存器"], sm0 - 3, SPACES["寄存器"], INK, 1.6))
    out.append(arrow(SX + SW, SPACES["共享内存"], sm0 - 3, SPACES["共享内存"], INK, 1.6))
    out.append(elbow([(SX + SW, SPACES["本地内存"]), (CHANNEL, SPACES["本地内存"]),
                      (CHANNEL, LOCAL_IN), (cx - 3, LOCAL_IN)], INK, 1.6))
    out.append(arrow(SX + SW, SPACES["全局内存"], cx - 3, SPACES["全局内存"], INK, 1.6))
    return out


def build():
    out = [text(EX + 75, 20, "线程层次", 15, MUTED, "bold"),
           text(SX + SW / 2, 20, "存储空间", 15, MUTED, "bold")]
    return out + entities() + spaces() + hardware()


if __name__ == "__main__":
    save("mem-hierarchy", W, H, build(), left=8)
