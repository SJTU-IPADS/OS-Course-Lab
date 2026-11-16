#!/usr/bin/env python3
"""A whole GPU of the Fermi architecture.

On the chip (white, dark outline): the host interface and the thread-block
scheduler (GigaThread) on top; 16 SMs in two rows of 8 with the L2 cache that
all of them share between the rows; three DRAM interfaces on each side. Off
the chip, on both sides: the device memory (DRAM). Each SM is a small copy of
sm-structure.svg: warp schedulers (purple), register file and shared memory
(grey frames), 32 CUDA cores, 16 LD/ST units and 4 SFUs (orange).
Redrawn after the GPU figure of the NVIDIA Fermi Compute Architecture
whitepaper. Run it to refresh ../assets/gpu-overview.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_PURPLE, GREEN, INK, MUTED,
                    ORANGE, PURPLE, WHITE, arrow, line, rect, save, text)

W, H = 900, 380
CHIP = (64, 8, 772, 364)          # x, y, w, h of the chip
SM_X0, SM_W = 140, 72
ROW_Y, SM_H = (66, 242), 120      # tops of the two rows of SMs
L2 = (ROW_Y[0] + SM_H + 10, 36)   # top and height of the L2 cache
IF_W = 54                         # a DRAM interface
SM_GAP = (CHIP[0] + CHIP[2] - 8 - IF_W - 12 - SM_X0 - 8 * SM_W) / 7
DRAM_W = 44


def sm(x, y, n):
    """One SM, drawn small: the parts of sm-structure.svg at the same places."""
    out = [rect(x, y, SM_W, SM_H, FILL_GREY, MUTED, rx=4, width=1.4),
           text(x + SM_W / 2, y + 16, f"SM {n}", 13, INK, "bold")]
    ix, iw = x + 5, SM_W - 10
    for k in range(2):                                   # warp schedulers
        out.append(rect(ix + k * (iw / 2 + 1), y + 23, iw / 2 - 1, 7, FILL_PURPLE, PURPLE,
                        rx=1, width=0.8))
    out.append(rect(ix, y + 34, iw, 12, FILL_GREY, MUTED, rx=1, width=0.9))   # registers
    uy, uh = y + 50, 54
    pitch = (uh + 2) / 8
    for c in range(4):                                   # 32 cores
        for r in range(8):
            out.append(rect(ix + c * 9.5, uy + r * pitch, 8, pitch - 2, FILL_ORANGE, ORANGE,
                            rx=0, width=0.6))
    pitch = (uh + 1) / 16
    for r in range(16):                                  # 16 LD/ST
        out.append(rect(ix + 41, uy + r * pitch, 7, pitch - 1, FILL_ORANGE, ORANGE, rx=0,
                        width=0.5))
    pitch = (uh + 2) / 4
    for r in range(4):                                   # 4 SFU
        out.append(rect(ix + 51, uy + r * pitch, iw - 51, pitch - 2, FILL_ORANGE, ORANGE,
                        rx=0, width=0.6))
    out.append(rect(ix, y + SM_H - 12, iw, 6, FILL_GREY, MUTED, rx=1, width=0.9))  # shared
    return out


def build():
    out = []
    cx, cy, cw, ch = CHIP
    out.append(rect(cx, cy, cw, ch, WHITE, INK, rx=6, width=1.6))
    out.append(text(cx + 8 + IF_W / 2, cy + 34, "GPU 芯片", 13, MUTED, "bold"))

    # --- host interface and thread-block scheduler ---------------------------
    sm_x1 = SM_X0 + 8 * SM_W + 7 * SM_GAP
    mid = (SM_X0 + sm_x1) / 2
    out.append(rect(SM_X0, cy + 10, mid - 20 - SM_X0, 36, WHITE, INK, rx=4, width=1.4))
    out.append(text((SM_X0 + mid - 20) / 2, cy + 34, "主机接口（PCIe）", 15, INK, "bold"))
    out.append(rect(mid + 20, cy + 10, sm_x1 - mid - 20, 36, FILL_PURPLE, PURPLE, rx=4,
                    width=1.6))
    out.append(text((mid + 20 + sm_x1) / 2, cy + 34, "线程块调度器（GigaThread）", 15, INK,
                    "bold"))
    out.append(arrow(mid - 18, cy + 28, mid + 17, cy + 28, INK, 2))

    # --- 16 SMs and the L2 cache ------------------------------------------------
    for row, y in enumerate(ROW_Y):
        for k in range(8):
            out += sm(SM_X0 + k * (SM_W + SM_GAP), y, row * 8 + k)
    out.append(rect(SM_X0, L2[0], sm_x1 - SM_X0, L2[1], FILL_GREY, MUTED, rx=4, width=1.6))
    out.append(text(mid, L2[0] + L2[1] / 2 + 6, "L2 高速缓存（所有 SM 共用）", 16, INK, "bold"))

    # --- DRAM interfaces on the chip, device memory off it ----------------------
    top, bottom = ROW_Y[0], ROW_Y[1] + SM_H
    ih = (bottom - top - 2 * 10) / 3
    for side in (0, 1):
        ix = cx + 8 if side == 0 else cx + cw - 8 - IF_W
        dx = cx - 12 - DRAM_W if side == 0 else cx + cw + 12
        out.append(rect(dx, top, DRAM_W, bottom - top, FILL_GREEN, GREEN, rx=4, width=1.6))
        out.append(text(dx + DRAM_W / 2, (top + bottom) / 2 + 5, "显存", 15, INK, "bold"))
        for k in range(3):
            iy = top + k * (ih + 10)
            out.append(rect(ix, iy, IF_W, ih, WHITE, GREEN, rx=4, width=1.6))
            out.append(text(ix + IF_W / 2, iy + ih / 2 - 3, "DRAM", 13, INK, "bold"))
            out.append(text(ix + IF_W / 2, iy + ih / 2 + 15, "接口", 13, INK, "bold"))
            ly = iy + ih / 2
            x0, x1 = (dx + DRAM_W, ix) if side == 0 else (ix + IF_W, dx)
            out.append(line(x0, ly, x1, ly, GREEN, 3))
    return out


if __name__ == "__main__":
    save("gpu-overview", W, H, build())
