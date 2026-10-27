#!/usr/bin/env python3
"""Four users' requests take turns: from cycle 4 on no GPU is idle.

Request 1's word 1 leaves GPU 4 in cycle 4; its word 2 enters GPU 1 in
cycle 5, exactly when GPU 1 is free again. Each request still gets one word
every four cycles. Run it to refresh ../assets/llm-pp-batch.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import GREEN, INK, ORANGE, brace, elbow, save, text
from timegrid import CH, CW, LX, TOP, col_frame, cx, cycle_numbers, row

W, H = 1120, 360
PITCH = CH + 8
GPUS = ["GPU 1（1–7 层）", "GPU 2（8–14 层）", "GPU 3（15–21 层）", "GPU 4（22–28 层）"]
REQS = ["请求 1", "请求 2", "请求 3", "请求 4"]
NCOL = 8


def build():
    out = [text(LX + 4 * CW, 22, "请求 1 的词 1 第 4 拍取出，词 2 第 5 拍进入 GPU 1：第 4 拍起没有气泡",
                16, ORANGE, "bold")]
    out += cycle_numbers(NCOL)
    for r, label in enumerate(GPUS):
        cells = ["停"] * r + [REQS[(c - r) % 4] for c in range(r, NCOL)]
        out += row(label, TOP + r * PITCH, 0, cells)
    y4 = TOP + 3 * PITCH + CH / 2
    y1 = TOP + CH / 2
    xg = LX + 4 * CW
    out.append(elbow([(LX + 4 * CW - 4, y4), (xg, y4), (xg, y1), (LX + 4 * CW + 4, y1)], INK, 2.4))
    bottom = TOP + 3 * PITCH + CH
    out += brace(LX + 3 * CW + 3, LX + 8 * CW - 3, bottom + 10, GREEN,
                 "每一拍 4 块 GPU 都在算；每个请求每 4 拍得到一个词", 15, depth=7)
    return out


if __name__ == "__main__":
    save("llm-pp-batch", W, H, build(), left=-64)
