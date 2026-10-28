#!/usr/bin/env python3
"""Six users keep all six H200 stages occupied after the pipeline fills."""
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import BLUE, GREEN, ORANGE, brace, save
from llm_pipeline import BOTTOM, CW, GPU_COUNT, GPUS, LX, PITCH, RETURN_GAP, TOP, W, canvas_left, cycle_numbers, return_arrow, row

NCOL = 2 * GPU_COUNT
RETURN_AFTER = (GPU_COUNT, GPU_COUNT + 1)


def build():
    out = cycle_numbers(NCOL, return_gap=RETURN_GAP, return_after=RETURN_AFTER)
    for stage, label in enumerate(GPUS):
        cells = ["bubble"] * stage + [f"请求 {(column - stage) % GPU_COUNT + 1}" for column in range(stage, NCOL)]
        out += row(label, TOP + stage * PITCH, cells, return_gap=RETURN_GAP,
                   return_after=RETURN_AFTER)
    out += return_arrow(GPU_COUNT, BLUE, "请求1回传", return_after=RETURN_AFTER)
    out += return_arrow(GPU_COUNT + 1, ORANGE, "请求2回传", return_after=RETURN_AFTER)
    out += brace(LX + (GPU_COUNT - 1) * CW + 3,
                 LX + NCOL * CW + len(RETURN_AFTER) * RETURN_GAP - 3, BOTTOM + 10,
                 GREEN, "填满后每周期输出一个词；每个请求每 6 个周期得到一个词", 14, depth=7)
    return out


if __name__ == "__main__":
    save("llm-pp-batch", W, BOTTOM + 48, build(),
         left=canvas_left(NCOL, RETURN_GAP, RETURN_AFTER))
