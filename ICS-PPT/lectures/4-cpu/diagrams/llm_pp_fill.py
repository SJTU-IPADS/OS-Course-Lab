#!/usr/bin/env python3
"""Six independent requests fill six H200 stages; retain four reveal frames."""
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import ORANGE, RED, brace, save, text
from llm_pipeline import BOTTOM, CW, GPU_COUNT, GPUS, LX, PITCH, TOP, W, canvas_left, col_frame, cx, cycle_numbers, row

NCOL = 2 * GPU_COUNT - 1
UPTO = [1, 2, GPU_COUNT, NCOL]
REQS = [f"请求 {index + 1}" for index in range(GPU_COUNT)]


def build(upto):
    out = []
    if upto >= GPU_COUNT:
        out.append(text(cx(GPU_COUNT - 1), 18, "周期 6：6 块 H200 同时计算，各处理一个请求", 15, ORANGE, "bold"))
    out += cycle_numbers(NCOL, emph=GPU_COUNT if upto >= GPU_COUNT else None)
    for stage, label in enumerate(GPUS):
        cells = ["bubble"] * stage + REQS + ["bubble"] * (GPU_COUNT - 1 - stage)
        out += row(label, TOP + stage * PITCH, cells, upto)
    if upto >= GPU_COUNT:
        out.append(col_frame(GPU_COUNT - 1, TOP - 4, BOTTOM + 4, ORANGE))
    if upto >= NCOL:
        out += brace(LX + 3, LX + (GPU_COUNT - 1) * CW - 3, BOTTOM + 10,
                     RED, "进管：后面的 GPU 等待请求", 14, depth=7)
        out += brace(LX + GPU_COUNT * CW + 3, LX + NCOL * CW - 3, BOTTOM + 10,
                     RED, "排空：前面的 GPU 没有新请求", 14, depth=7)
    return out


if __name__ == "__main__":
    for frame, upto in enumerate(UPTO, 1):
        save(f"llm-pp-fill-{frame}", W, BOTTOM + 48, build(upto), left=canvas_left(NCOL))
