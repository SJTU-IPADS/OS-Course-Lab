#!/usr/bin/env python3
"""One user generating word by word on four GPUs.

The grid of llm_pp_single.py over eight cycles: word 1 walks down the
diagonal in cycles 1-4, GPU 4 takes it in cycle 4, and only then can word 2
enter GPU 1 in cycle 5; the arrow runs from word 1 on GPU 4 to word 2 on
GPU 1. GPU 1 has bubbles in cycles 2-4 (red frame). Eight cycles give two
words.
Run it to refresh ../assets/llm-pp-decode.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from llm_pp_single import BOTTOM, K, W, gpu_grid, gy, handoff
from svgkit import INK, RED, brace, rect, save, text
from timegrid import CH, CW, LX, VIEW_LEFT

STEPS = 2 * K


def work(g, t):
    for n in (1, 2):
        if t == g + (n - 1) * K:
            return f"词 {n}"
    return None


def build():
    out = gpu_grid(STEPS, work)
    out.append(rect(LX + CW + 1, gy(1) - 3, (K - 1) * CW - 2, CH + 6, "none", RED, rx=6,
                    width=2, dash="6 4"))
    out.append(text(LX + 2.5 * CW, 22, "GPU 1 等词 1：3 个周期是 bubble", 16, RED, "bold"))
    out += handoff(K)
    out.append(text(LX + K * CW + 24, 22, "词 1 是词 2 的输入", 16, INK, "bold",
                    anchor="start"))
    out += brace(LX + 3, LX + STEPS * CW - 3, BOTTOM + 12, INK,
                 f"{STEPS} 个周期只出 2 个词", 16)
    return out


if __name__ == "__main__":
    save("llm-pp-decode", W, BOTTOM + 50, build(), left=VIEW_LEFT)
