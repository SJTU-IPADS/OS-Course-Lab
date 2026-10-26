#!/usr/bin/env python3
"""Four users' requests taking turns on four GPUs.

The grid of llm_pp_single.py over eight cycles: GPU g in cycle t works on
request ((t - g) mod 4) + 1. Requests 2-4 fill the cycles in which GPU 1 would
wait for request 1's word; request 1's word 1 leaves GPU 4 in cycle 4 and its
word 2 enters GPU 1 in cycle 5 (the arrow, as in llm_pp_decode.py). Only the
fill bubbles of cycles 1-3 remain.
Run it to refresh ../assets/llm-pp-batch.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from llm_pp_single import BOTTOM, K, W, gpu_grid, handoff
from svgkit import GREEN, INK, brace, save, text
from timegrid import CW, LX, VIEW_LEFT

STEPS = 2 * K


def work(g, t):
    return f"请求 {(t - g) % K + 1}" if t >= g else None


def build():
    out = gpu_grid(STEPS, work)
    out += handoff(K)
    out.append(text(LX + K * CW + 24, 22, "请求 1 的词 2 在周期 5 进入 GPU 1", 16, INK, "bold",
                    anchor="start"))
    out += brace(LX + (K - 1) * CW + 3, LX + STEPS * CW - 3, BOTTOM + 12, GREEN,
                 f"从周期 {K} 起没有 bubble：每个周期出一个词", 16)
    return out


if __name__ == "__main__":
    save("llm-pp-batch", W, BOTTOM + 50, build(), left=VIEW_LEFT)
