#!/usr/bin/env python3
"""A second word waits until its first word leaves the sixth H200 stage."""
import pathlib
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import RED, rect, save, text
from llm_pipeline import BOTTOM, CH, CW, GPU_COUNT, GPUS, LX, PITCH, RETURN_GAP, TOP, W, canvas_left, cx, cycle_numbers, return_arrow, row

NCOL = 2 * GPU_COUNT


def build():
    out = cycle_numbers(NCOL, return_gap=RETURN_GAP)
    for stage, label in enumerate(GPUS):
        cells = ["bubble"] * NCOL
        cells[stage] = "词 1"
        cells[stage + GPU_COUNT] = "词 2"
        out += row(label, TOP + stage * PITCH, cells, return_gap=RETURN_GAP)
    out += return_arrow()
    out.append(rect(LX + CW + 1, TOP - 3, (GPU_COUNT - 1) * CW - 2, CH + 6,
                    "none", RED, rx=5, width=2, dash="6 4"))
    out.append(text(cx(3), BOTTOM + 27, "GPU 1 在周期 2–6 等待本请求的下一个词", 15, RED, "bold"))
    return out


if __name__ == "__main__":
    save("llm-pp-decode", W, BOTTOM + 42, build(), left=canvas_left(NCOL, RETURN_GAP))
