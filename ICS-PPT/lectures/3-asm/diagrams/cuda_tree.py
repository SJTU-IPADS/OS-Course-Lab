#!/usr/bin/env python3
"""The shared-memory reduction of dot_kernel in one 256-thread block.

Row 0 is cache[0..255] after every thread stores its product. At each step
s halves; threads with tid < s add cache[tid + s] (orange half) into
cache[tid] (blue half). After 8 steps cache[0] holds the block's sum. Bar
length follows the element count down to a minimum width.
Run it to refresh ../assets/cuda-tree.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, MONO,
                    MUTED, ORANGE, arrow, mono, rect, save, text)

W, H = 1120, 470
X0, FULL, MINW = 170, 760, 30
RH, STEP, TOP = 26, 44, 56


def width(c):
    return max(FULL * c / 256, MINW)


def build():
    out = [text(X0, 26, "cache[tid] += cache[tid + s]，每步之后 __syncthreads()", 16, INK,
                "bold", anchor="start", font=MONO + ", PingFang SC, Noto Sans CJK SC")]
    for k in range(9):
        c = 256 >> k
        y = TOP + k * STEP
        w = width(c)
        label = "写入乘积" if k == 0 else f"s = {c}"
        out.append(text(X0 - 16, y + 19, label, 15, INK if k else MUTED,
                        "bold" if k else "normal", anchor="end",
                        font=MONO if k else "PingFang SC, Noto Sans CJK SC, sans-serif"))
        if c > 1:
            half = w / 2
            out.append(rect(X0, y, half, RH, FILL_BLUE, BLUE, rx=2, width=1.2))
            out.append(rect(X0 + half, y, half, RH, FILL_ORANGE, ORANGE, rx=2, width=1.2))
        else:
            out.append(rect(X0, y, w, RH, FILL_GREEN, GREEN, rx=2, width=1.6))
        out.append(mono(X0 + w + 14, y + 19, f"cache[0..{c - 1}]" if c > 1 else "cache[0]",
                        14, MUTED if c > 1 else GREEN, "bold" if c == 1 else "normal"))
        if k:
            px = X0 + width(c * 2) * 3 / 4
            out.append(arrow(px, y - STEP + RH + 2, X0 + w / 2, y - 2, ORANGE, 1.4))
    out.append(text(X0 + 250, TOP + 8 * STEP + 19, "block_sum[blockIdx.x] = cache[0]", 15,
                    GREEN, "bold", anchor="start",
                    font=MONO))
    out.append(text(1100, 120, "256 → 1：共 8 步", 18, INK, "bold", anchor="end"))
    out.append(text(1100, 148, "每步活跃线程减半", 15, MUTED, anchor="end"))
    return out


if __name__ == "__main__":
    save("cuda-tree", W, H, build())
