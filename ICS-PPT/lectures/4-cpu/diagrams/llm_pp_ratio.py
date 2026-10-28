#!/usr/bin/env python3
"""Six stages and eight requests: 30 bubbles among 78 cells."""
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import RED, save, text
from llm_pipeline import BOTTOM, GPU_COUNT, PITCH, TOP, W, canvas_left, cycle_numbers, row

K, M = GPU_COUNT, 8
NCOL = M + K - 1


def build():
    out = cycle_numbers(NCOL)
    for stage in range(K):
        cells = ["bubble"] * stage + [f"请求 {request + 1}" for request in range(M)] + ["bubble"] * (K - stage - 1)
        out += row(f"GPU {stage + 1}", TOP + stage * PITCH, cells)
    out.append(text(204, BOTTOM + 27, "bubble 30 格 ÷ 总共 6 × 13 = 78 格 ≈ 38%", 15, RED, "bold", anchor="start"))
    return out


if __name__ == "__main__":
    save("llm-pp-ratio", W, BOTTOM + 42, build(), left=canvas_left(NCOL))
