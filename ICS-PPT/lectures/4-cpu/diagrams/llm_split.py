#!/usr/bin/env python3
"""Pipeline parallelism: 28 layers cut into four pieces, one GPU each.

The vector handed from one GPU to the next is what an inter-stage register
holds in the CPU pipeline. Run it to refresh ../assets/llm-split.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK,
                    MUTED, ORANGE, arrow, box, save, text)

W, H = 1120, 180
BW, BH, GAP = 184, 74, 68
X0 = (W - (4 * BW + 3 * GAP)) / 2
Y = 54
TONES = [(FILL_BLUE, BLUE), (FILL_ORANGE, ORANGE), (FILL_GREEN, GREEN), (FILL_GREY, INK)]
LAYERS = ["第 1–7 层", "第 8–14 层", "第 15–21 层", "第 22–28 层"]


def build():
    out = [text(X0 - 44, Y + BH / 2 + 5, "词", 15, INK, anchor="end"),
           arrow(X0 - 40, Y + BH / 2, X0 - 2, Y + BH / 2, INK, 2)]
    for i, (name, (fill, stroke)) in enumerate(zip(LAYERS, TONES)):
        x = X0 + i * (BW + GAP)
        out.append(text(x + BW / 2, Y - 14, f"GPU {i + 1}", 17, stroke, "bold"))
        out += box(x, Y, BW, BH, name, fill, stroke, 17, "bold",
                   sub="显存里只放这 7 层的权重", sub_size=12)
        if i < 3:
            out.append(arrow(x + BW + 2, Y + BH / 2, x + BW + GAP - 2, Y + BH / 2, INK, 2.2))
            out.append(text(x + BW + GAP / 2, Y + BH + 22, "向量", 14, MUTED))
            out.append(text(x + BW + GAP / 2, Y + BH + 40, "2048 个数", 12, MUTED))
    xr = X0 + 4 * BW + 3 * GAP
    out.append(arrow(xr + 2, Y + BH / 2, xr + 40, Y + BH / 2, INK, 2))
    out.append(text(xr + 46, Y + BH / 2 + 5, "取词", 15, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("llm-split", W, H, build())
