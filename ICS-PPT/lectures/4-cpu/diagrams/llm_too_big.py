#!/usr/bin/env python3
"""The weights of a large model against the memory of one GPU.

Two bars on one scale: Llama 3.1 405B in 16-bit (810 GB) and one H100
(80 GB). The long bar is ticked every 80 GB, so the number of GPUs needed
can be counted off it. Run it to refresh ../assets/llm-too-big.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, MUTED, ORANGE, RED, WHITE, line,
                    rect, save, text)

W, H = 1120, 200
LX = 24                    # labels
BX = 300                   # bars start here
SCALE = 700 / 810          # px per GB
BH = 44
Y1, Y2 = 36, 118
GPU_GB, MODEL_GB = 80, 810


def bar(y, gb, fill, stroke, label, value):
    w = gb * SCALE
    out = [text(LX, y + BH / 2 + 6, label, 16, INK, "bold", anchor="start"),
           rect(BX, y, w, BH, fill, stroke, rx=4, width=1.6),
           text(BX + w + 12, y + BH / 2 + 6, value, 17, stroke, "bold", anchor="start")]
    return out


def build():
    out = bar(Y1, MODEL_GB, FILL_ORANGE, ORANGE, "Llama 3.1 405B 的权重", "810 GB")
    n_full = MODEL_GB // GPU_GB
    for k in range(1, n_full + 1):
        x = BX + k * GPU_GB * SCALE
        out.append(line(x, Y1 + 2, x, Y1 + BH - 2, WHITE, 2))
    for k in range(n_full):
        x = BX + (k + 0.5) * GPU_GB * SCALE
        out.append(text(x, Y1 + BH / 2 + 5, str(k + 1), 13, ORANGE))
    out.append(text(BX + MODEL_GB * SCALE - 6, Y1 + BH + 18, "第 11 块", 13, RED, anchor="end"))
    out += bar(Y2, GPU_GB, FILL_BLUE, BLUE, "一块 H100 的显存", "80 GB")
    out.append(text(BX, Y2 + BH + 30, "4050 亿个权重 × 2 字节 = 810 GB；810 ÷ 80 ≈ 10.1，至少 11 块 GPU 才放得下",
                    15, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("llm-too-big", W, H, build())
