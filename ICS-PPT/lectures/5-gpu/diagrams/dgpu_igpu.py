#!/usr/bin/env python3
"""The system structure of a discrete GPU and of an integrated GPU.

Left: the CPU with its memory, the GPU with its own device memory (VRAM), the
two chips connected by PCIe. Right: CPU cores and GPU cores in one chip, which
reaches one system memory. The width of a connection is proportional to its
bandwidth, for a PC with the graphics card of the photo beside the figure:
PCIe 3.0 x16 16 GB/s (2 px), dual-channel DDR4-3200 51.2 GB/s (6.4 px), the
GDDR6 of an RTX 2080 448 GB/s (56 px). The integrated GPU is drawn with the
same dual-channel DDR4-3200 as its system memory.
Run it to refresh ../assets/dgpu-igpu.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, GREEN, INK, LINE, MUTED, WHITE, box, line, rect, save, text)

W, H = 572, 180
PX_PER_GBS = 2 / 16               # width of a connection per GB/s
PCIE, DDR, GDDR = 16, 51.2, 448   # bandwidths in GB/s
MEM_Y, MEM_H = 30, 40             # the memories
CHIP_Y, CHIP_H = 124, 40          # the processors
BAND = LINE


def band(xc, y0, y1, gbs):
    w = gbs * PX_PER_GBS
    return rect(xc - w / 2, y0, w, y1 - y0, BAND, "none", rx=0, width=0)


def memory(x, w, label):
    return box(x, MEM_Y, w, MEM_H, label, FILL_GREEN, GREEN, size=14)


def chip(x, y, w, h, label, stroke=INK):
    return box(x, y, w, h, label, WHITE, stroke, size=15)


def discrete(x0):
    out = [text(x0 + 138, 18, "独立 GPU", 16, INK, "bold")]
    cpu, gpu, bw = x0 + 6, x0 + 156, 114
    out += memory(cpu, bw, "内存（DRAM）") + memory(gpu, bw, "显存（VRAM）")
    out.append(band(cpu + bw / 2, MEM_Y + MEM_H, CHIP_Y, DDR))
    out.append(band(gpu + bw / 2, MEM_Y + MEM_H, CHIP_Y, GDDR))
    out += chip(cpu, CHIP_Y, bw, CHIP_H, "CPU") + chip(gpu, CHIP_Y, bw, CHIP_H, "GPU")
    py = CHIP_Y + CHIP_H / 2
    w = PCIE * PX_PER_GBS
    out.append(rect(cpu + bw, py - w / 2, gpu - cpu - bw, w, BAND, "none", rx=0, width=0))
    out.append(text((cpu + bw + gpu) / 2, py - 8, "PCIe", 13, MUTED, "bold"))
    return out


def integrated(x0):
    out = [text(x0 + 138, 18, "集成 GPU", 16, INK, "bold")]
    x, w = x0 + 14, 248
    out += memory(x, w, "系统内存（DRAM）")
    pkg_y = 106
    out.append(band(x + w / 2, MEM_Y + MEM_H, pkg_y, DDR))
    out.append(rect(x, pkg_y, w, H - 6 - pkg_y, WHITE, INK, rx=6, width=1.6))
    out.append(text(x + w / 2, pkg_y + 16, "同一颗芯片", 13, MUTED))
    cw = (w - 36) / 2
    out += chip(x + 12, pkg_y + 22, cw, 36, "CPU 核心", LINE)
    out += chip(x + 24 + cw, pkg_y + 22, cw, 36, "GPU 核心", LINE)
    return out


def build():
    out = discrete(4) + integrated(292)
    out.append(line(286, 8, 286, H - 8, LINE, 1))
    return out


if __name__ == "__main__":
    save("dgpu-igpu", W, H, build(), left=-4)
