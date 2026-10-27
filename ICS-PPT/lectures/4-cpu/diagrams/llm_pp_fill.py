#!/usr/bin/env python3
"""Pipeline parallelism with four requests: the middle cycle is full.

Requests 1..4 enter one cycle apart. Cycles 1..3 and 5..7 still hold the
start-up and drain bubbles; cycle 4 has all four GPUs busy. An animation:
frame k shows the cycles that have happened. Run it to refresh
../assets/llm-pp-fill-1.svg .. llm-pp-fill-4.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import ORANGE, RED, brace, save, text
from timegrid import CH, CW, LX, TOP, col_frame, cx, cycle_numbers, row

W, H = 1120, 360
PITCH = CH + 8
GPUS = ["GPU 1（1–7 层）", "GPU 2（8–14 层）", "GPU 3（15–21 层）", "GPU 4（22–28 层）"]
REQS = ["请求 1", "请求 2", "请求 3", "请求 4"]
NCOL = 7
UPTO = [1, 2, 4, 7]                 # cycles shown in each frame


def build(upto):
    out = []
    if upto >= 4:
        out.append(text(cx(3), 22, "周期 4：4 块 GPU 同时在算，各算一个请求", 16, ORANGE, "bold"))
    out += cycle_numbers(NCOL, emph=4 if upto >= 4 else None)
    for r, label in enumerate(GPUS):
        cells = ["停"] * r + REQS + ["停"] * (3 - r)
        out += row(label, TOP + r * PITCH, 0, cells, upto)
    bottom = TOP + 3 * PITCH + CH
    if upto >= 4:
        out.append(col_frame(3, TOP - 5, bottom + 5, ORANGE))
    if upto >= 7:
        out += brace(LX + 3, LX + 3 * CW - 3, bottom + 10, RED, "进管：后面的 GPU 还没拿到请求", 15, depth=7)
        out += brace(LX + 4 * CW + 3, LX + 7 * CW - 3, bottom + 10, RED, "排空：前面的 GPU 没有新请求", 15, depth=7)
    return out


if __name__ == "__main__":
    for k, upto in enumerate(UPTO, 1):
        save(f"llm-pp-fill-{k}", W, H, build(upto), left=-80)
