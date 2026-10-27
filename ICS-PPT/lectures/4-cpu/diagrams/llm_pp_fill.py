#!/usr/bin/env python3
"""Pipeline parallelism with four requests entering one cycle apart.

The grid of llm_pp_single.py: request m is on GPU g in cycle m + g - 1. From
cycle 4 on all four GPUs are busy, each on a different request; cycles 1-3
and 5-7 keep their bubbles, the fill and drain bubbles.

An animation of four frames: cycles 1; 1-2; 1-4 with the orange frame around
cycle 4; 1-7 with the red brackets under the two groups of bubbles.
Run it to refresh ../assets/llm-pp-fill-1.svg .. llm-pp-fill-4.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from llm_pp_single import BOTTOM, K, W, gpu_grid, gy
from svgkit import GREEN, ORANGE, RED, brace, save, text
from timegrid import CH, CW, LX, TOP, VIEW_LEFT, col_frame, cx

M = 4                                # requests
STEPS = M + K - 1
FRAMES = [1, 2, 4, STEPS]            # cycles shown in each frame


def work(g, t):
    m = t - g + 1
    return f"请求 {m}" if 1 <= m <= M else None


def build(steps):
    out = gpu_grid(steps, work, emph=K if steps >= K else None, color=ORANGE)
    if steps >= K:
        out.append(text(cx(K - 1), 22, f"周期 {K}：{K} 块 GPU 同时在算", 16, ORANGE, "bold"))
        out.append(col_frame(K - 1, TOP - 5, BOTTOM + 5, ORANGE))
    if steps < STEPS:
        return out
    out.append(text(LX + STEPS * CW + 14, gy(K) + CH / 2 + 6, "每个周期完成一个请求", 16, GREEN,
                    "bold", anchor="start"))
    out += brace(LX + 3, LX + (K - 1) * CW - 3, BOTTOM + 12, RED, "开头的 bubble（进管）", 16)
    out += brace(LX + K * CW + 3, LX + STEPS * CW - 3, BOTTOM + 12, RED,
                 "结尾的 bubble（排空）", 16)
    return out


if __name__ == "__main__":
    for k, steps in enumerate(FRAMES, 1):
        save(f"llm-pp-fill-{k}", W, BOTTOM + 50, build(steps), left=VIEW_LEFT)
