#!/usr/bin/env python3
"""The one path that bounds the whole machine: weights from memory to the CPU.

Memory holds the 0.968 GB of Q4_0 weights; every token moves all of them
across a 51.2 GB/s link once, so the machine makes at most 53 tokens a
second, however many cores compute. Numbers as in bandwidth_limit() in
pages.py. Run it to refresh ../assets/bandwidth-pipe.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, FILL_BLUE, FILL_GREY, INK, MUTED, RED, arrow, box, save, text

W, H = 1120, 150
BY, BH = 30, 90


def build():
    out = []
    out += box(120, BY, 300, BH, "内存", FILL_GREY, MUTED, 20, sub="全部权重 0.968 GB")
    out += box(700, BY, 300, BH, "CPU", FILL_BLUE, BLUE, 20, sub="8 个核心")
    mid = BY + BH / 2
    out.append(arrow(424, mid, 694, mid, RED, 3))
    out.append(text(560, mid - 14, "内存带宽 51.2 GB/s", 17, RED, "bold"))
    out.append(text(560, mid + 26, "每个 Token 送一遍 0.968 GB", 15, INK))
    out.append(text(560, mid + 46, "每秒至多 51.2 ÷ 0.968 ≈ 53 遍", 15, INK))
    return out


if __name__ == "__main__":
    save("bandwidth-pipe", W, H, build())
