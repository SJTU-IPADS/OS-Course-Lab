#!/usr/bin/env python3
"""Llama 3.1 405B's 126 layers split across six H200 stages."""
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import BLUE, FILL_GREY, INK, MUTED, arrow, box, save, text
from llm_pipeline import GPU_COUNT, LAYERS_PER_GPU

W, H = 1120, 194
BW, BH, GAP = 132, 70, 34
X0 = (W - (GPU_COUNT * BW + (GPU_COUNT - 1) * GAP)) / 2
Y = 52


def build():
    out = [text(X0 - 42, Y + BH / 2 + 5, "词", 14, INK, anchor="end"),
           arrow(X0 - 38, Y + BH / 2, X0 - 2, Y + BH / 2, INK, 2)]
    for stage in range(GPU_COUNT):
        x = X0 + stage * (BW + GAP)
        first = stage * LAYERS_PER_GPU + 1
        last = (stage + 1) * LAYERS_PER_GPU
        out.append(text(x + BW / 2, Y - 14, f"GPU {stage + 1} · H200", 14, INK, "bold"))
        out += box(x, Y, BW, BH, f"第 {first}–{last} 层", FILL_GREY, INK, 14, "bold",
                   sub="本级 21 层的权重", sub_size=12)
        if stage < GPU_COUNT - 1:
            out.append(arrow(x + BW + 2, Y + BH / 2, x + BW + GAP - 2, Y + BH / 2, INK, 2))
            out.append(text(x + BW + GAP / 2, Y + BH + 22, "向量", 13, BLUE))
            out.append(text(x + BW + GAP / 2, Y + BH + 40, "16 384 维", 12, MUTED))
    right = X0 + GPU_COUNT * BW + (GPU_COUNT - 1) * GAP
    out.append(arrow(right + 2, Y + BH / 2, right + 36, Y + BH / 2, INK, 2))
    out.append(text(right + 40, Y + BH / 2 + 5, "取词", 14, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("llm-split", W, H, build())
