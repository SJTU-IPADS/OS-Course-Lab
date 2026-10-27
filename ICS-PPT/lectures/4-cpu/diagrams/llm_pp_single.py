#!/usr/bin/env python3
"""Pipeline parallelism with one request: three of the four GPUs sit idle.

28 layers split across 4 GPUs. One request passes GPU 1..4 in four cycles;
every other cell is 停, the same bubble as the CPU pipeline. Run it to
refresh ../assets/llm-pp-single.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import RED, save, text
from timegrid import CH, CW, LX, TOP, col_frame, cx, cycle_numbers, row

W, H = 1120, 340
PITCH = CH + 8
GPUS = ["GPU 1（1–7 层）", "GPU 2（8–14 层）", "GPU 3（15–21 层）", "GPU 4（22–28 层）"]


def build():
    out = cycle_numbers(4)
    for r, label in enumerate(GPUS):
        cells = ["停"] * 4
        cells[r] = "请求 1"
        out += row(label, TOP + r * PITCH, 0, cells)
    bottom = TOP + 3 * PITCH + CH
    out.append(col_frame(0, TOP + PITCH - 4, bottom + 4, RED))
    out.append(text(cx(1.5), bottom + 28, "每一拍只有 1 块 GPU 在算，其余 3 块是气泡", 16, RED, "bold"))
    return out


if __name__ == "__main__":
    save("llm-pp-single", W, H, build(), left=-200)
