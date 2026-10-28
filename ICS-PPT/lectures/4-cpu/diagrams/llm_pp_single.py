#!/usr/bin/env python3
"""One request visits six H200 stages; five are idle in every cycle."""
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import RED, save, text
from llm_pipeline import BOTTOM, GPU_COUNT, GPUS, PITCH, TOP, W, canvas_left, col_frame, cx, cycle_numbers, row


def build():
    out = cycle_numbers(GPU_COUNT)
    for stage, label in enumerate(GPUS):
        cells = ["bubble"] * GPU_COUNT
        cells[stage] = "请求 1"
        out += row(label, TOP + stage * PITCH, cells)
    out.append(col_frame(0, TOP + PITCH - 3, BOTTOM + 3, RED))
    out.append(text(cx(2.5), BOTTOM + 27, "每个周期只有 1 块 H200 在计算，其余 5 块为 bubble", 15, RED, "bold"))
    return out


if __name__ == "__main__":
    save("llm-pp-single", W, BOTTOM + 42, build(), left=canvas_left(GPU_COUNT))
