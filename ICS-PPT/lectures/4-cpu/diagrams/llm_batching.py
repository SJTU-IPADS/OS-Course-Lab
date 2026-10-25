#!/usr/bin/env python3
"""Continuous batching drawn as requests against time steps.

One row per request, one column per step, one cell per stage the request
is in: tokenize, prefill, decode (one token per step), output.  Requests
arrive at different steps, so at any one step the batch holds tokens of
several requests in different stages, exactly as a pipeline holds several
instructions in different stages.  The chart of pipeline.py draws the
five processor stages only, so this one is drawn by hand.  Run it to
refresh ../assets/llm-batching.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED,
                    RED, line, rect, save, text)

W, H = 1120, 236
X0, Y0 = 20, 8
LABEL_W, CELL, ROW_H = 96, 46, 32
NCOL = 16
STEP = 6                       # the step whose batch is boxed

FILL = {"T": FILL_GREY, "P": FILL_ORANGE, "D": FILL_BLUE, "O": FILL_GREEN}
NAME = {"T": "分词", "P": "prefill", "D": "decode（1 个 token）", "O": "输出（detokenize）"}

ROWS = [("请求 1", 1, "TPPDDDDDO"),
        ("请求 2", 3, "TPDDDDDDO"),
        ("请求 3", 4, "TPPPDDDDO"),
        ("请求 4", 7, "TPDDDDDDDO")]


def chart():
    out = []
    x0 = X0 + LABEL_W
    out.append(text(x0 - 8, Y0 + ROW_H * 0.7, "时间步", 14, MUTED, anchor="end"))
    for c in range(NCOL):
        out.append(text(x0 + c * CELL + CELL / 2, Y0 + ROW_H * 0.7, str(c + 1), 14, MUTED))
    out.append(line(X0, Y0 + ROW_H, x0 + NCOL * CELL, Y0 + ROW_H, LINE, 1))
    for r, (label, start, pat) in enumerate(ROWS):
        ry = Y0 + ROW_H * (r + 1)
        out.append(text(X0, ry + ROW_H * 0.7, label, 15, INK, anchor="start"))
        for i, ch in enumerate(pat):
            cx = x0 + (start - 1 + i) * CELL
            out.append(rect(cx + 1, ry + 2, CELL - 2, ROW_H - 4, FILL[ch], LINE, rx=3, width=1))
            out.append(text(cx + CELL / 2, ry + ROW_H * 0.7, ch, 15, INK, "bold"))
    bx = x0 + (STEP - 1) * CELL
    out.append(rect(bx - 2, Y0 + 2, CELL + 4, ROW_H * (len(ROWS) + 1), "none", RED,
                    rx=4, width=1.8, dash="5 4"))
    return out, x0 + NCOL * CELL


def legend(x):
    out = []
    for i, k in enumerate("TPDO"):
        y = Y0 + 44 + i * 28
        out.append(rect(x, y, 16, 16, FILL[k], LINE, rx=3, width=1))
        out.append(text(x + 24, y + 13, f"{k}  {NAME[k]}", 14, MUTED, anchor="start"))
    return out


def build():
    out, right = chart()
    out += legend(right + 30)
    y = Y0 + ROW_H * (len(ROWS) + 1) + 28
    out.append(text(X0, y, f"第 {STEP} 步的一批：请求 1、请求 2 各 1 个 decode token，"
                    "请求 3 的 1 个 prefill token，合成一次矩阵乘", 15, RED, anchor="start"))
    out.append(text(X0, y + 26, "每一行的 D 格一个接一个：一个请求的 token 逐个生成，"
                    "批处理没有缩短它", 15, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("llm-batching", W, H, build())
