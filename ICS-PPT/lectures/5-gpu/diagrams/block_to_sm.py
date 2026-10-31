#!/usr/bin/env python3
"""The thread blocks of one grid, assigned to the SMs of two GPUs.

The grid has 8 blocks. On a GPU with 2 SMs each SM runs 4 of them, on a GPU
with 4 SMs each runs 2; the program is the same, and the GPU with more SMs
finishes in fewer rounds. A column of blocks in an SM is the order in which
the SM takes them. Redrawn after the figure "Automatic Scalability" of the
NVIDIA CUDA C++ Programming Guide. Run it to refresh ../assets/block-to-sm.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, INK, MUTED, WHITE, arrow, box, rect, save,
                    text)

W, H = 1020, 424
BH, PITCH = 32, 41           # height of a block, vertical pitch of the blocks in an SM
GPU_TOP, SM_TOP = 150, 190


def block(x, y, w, n):
    return box(x, y, w, BH, f"Block {n}", FILL_BLUE, BLUE, size=17, weight="normal", rx=3)


def gpu(x, w, sms, sm_w, gap, block_w, label):
    """A GPU of len(sms) SMs; sms[k] lists the blocks SM k runs, top first."""
    rows = max(len(s) for s in sms)
    sm_h = 34 + rows * PITCH + 2
    out = [rect(x, GPU_TOP, w, SM_TOP - GPU_TOP + sm_h + 14, WHITE, INK, rx=6, width=1.6),
           text(x + w / 2, GPU_TOP + 27, label, 18, INK, "bold")]
    inner = len(sms) * sm_w + (len(sms) - 1) * gap
    sx = x + (w - inner) / 2
    for k, blocks in enumerate(sms):
        out.append(rect(sx, SM_TOP, sm_w, sm_h, FILL_GREY, MUTED, rx=5, width=1.4))
        out.append(text(sx + sm_w / 2, SM_TOP + 23, f"SM {k}", 17, INK, "bold"))
        for r, n in enumerate(blocks):
            out += block(sx + (sm_w - block_w) / 2, SM_TOP + 34 + r * PITCH, block_w, n)
        sx += sm_w + gap
    return out


def build():
    out = []
    gx, gw, gap = 146, 84, 8
    out.append(text(W / 2, 28, "Grid：一次核函数启动的 8 个线程块", 18, INK, "bold"))
    for n in range(8):
        out += block(gx + n * (gw + gap), 46, gw, n)
    out.append(arrow(470, 84, 260, GPU_TOP - 6, BLUE, 2))
    out.append(arrow(550, 84, 740, GPU_TOP - 6, BLUE, 2))
    out += gpu(40, 420, [[0, 2, 4, 6], [1, 3, 5, 7]], 170, 24, 130, "2 个 SM 的 GPU")
    out += gpu(500, 480, [[0, 4], [1, 5], [2, 6], [3, 7]], 106, 10, 90, "4 个 SM 的 GPU")
    return out


if __name__ == "__main__":
    save("block-to-sm", W, H, build())
