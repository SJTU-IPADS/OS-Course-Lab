#!/usr/bin/env python3
"""Pipeline parallelism: the 28 layers cut into four parts on four GPUs.

The input word (blue) enters GPU 1; each GPU holds seven layers, drawn as
seven small weight matrices in the grey of llm_layers.py; between two GPUs
only the vector (blue, 2048 numbers) is passed on; GPU 4 takes the next word
(orange). The four GPUs are the four stages, the vector between them is what
a pipeline register holds.
Run it to refresh ../assets/llm-split.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE,
                    WHITE, arrow, box, line, rect, save, text)

W, H = 1120, 160
GW, GH, GAP = 168, 128, 62           # a GPU box and the gap between two
GY = 14
MY = GY + GH / 2
IN_W, OUT_W, END_GAP = 60, 76, 46
X0 = (W - (IN_W + END_GAP + 4 * GW + 3 * GAP + END_GAP + OUT_W)) / 2
PARTS = ["第 1–7 层", "第 8–14 层", "第 15–21 层", "第 22–28 层，取词"]


def mini_matrix(x, y, w=14, h=30):
    return [rect(x, y, w, h, FILL_GREY, INK, rx=2, width=1.1),
            line(x + w / 2, y + 3, x + w / 2, y + h - 3, LINE, 0.8)]


def vector(x, y, n=4, cw=10, ch=18):
    return [rect(x + k * cw, y, cw, ch, FILL_BLUE, BLUE, rx=1, width=1.1) for k in range(n)]


def build():
    out = []
    x = X0
    out.append(text(x + IN_W / 2, MY - 32, "词", 14, MUTED))
    out += box(x, MY - 22, IN_W, 44, "是", FILL_BLUE, BLUE, 17, "normal")
    x += IN_W
    out.append(arrow(x + 4, MY, x + END_GAP - 4, MY, INK, 2))
    x += END_GAP
    for k, part in enumerate(PARTS):
        out.append(rect(x, GY, GW, GH, WHITE, INK, rx=8, width=1.8))
        out.append(text(x + GW / 2, GY + 28, f"GPU {k + 1}", 18, INK, "bold"))
        out.append(text(x + GW / 2, GY + 52, part, 14, MUTED))
        mx = x + (GW - (7 * 14 + 6 * 5)) / 2
        for j in range(7):
            out += mini_matrix(mx + j * 19, GY + 66)
        out.append(text(x + GW / 2, GY + 116, "7 层的权重", 13, MUTED))
        x += GW
        if k < 3:
            out.append(arrow(x + 4, MY, x + GAP - 4, MY, INK, 2))
            out += vector(x + GAP / 2 - 20, MY - 26)
            out.append(text(x + GAP / 2, MY + 22, "向量", 13, BLUE))
            x += GAP
    out.append(arrow(x + 4, MY, x + END_GAP - 4, MY, INK, 2))
    x += END_GAP
    out.append(text(x + OUT_W / 2, MY - 32, "下一个词", 14, MUTED))
    out += box(x, MY - 22, OUT_W, 44, "吞吐", FILL_ORANGE, ORANGE, 16, "bold")
    return out


if __name__ == "__main__":
    save("llm-split", W, H, build())
