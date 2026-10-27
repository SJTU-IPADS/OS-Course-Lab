#!/usr/bin/env python3
"""How much of the grid the start-up and drain bubbles take.

K = 4 GPUs, M = 8 requests in a row: 4 x 11 cells, 12 of them bubbles, so
12 / 44 = 27 %, which is (K - 1) / (M + K - 1). Smaller cells than
timegrid so eleven cycles fit. Run it to refresh ../assets/llm-pp-ratio.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, FILL_BLUE, FILL_RED, INK, MUTED, RED, mono, rect, save, text

W = 1120
K, M = 4, 8
NCOL = M + K - 1
GX, GY = 150, 40
CELL_W, CELL_H, PITCH = 66, 40, 46


def build():
    out = [text(GX - 10, GY - 20, "周期", 14, MUTED, anchor="end")]
    for c in range(NCOL):
        out.append(text(GX + c * CELL_W + CELL_W / 2, GY - 20, str(c + 1), 14, MUTED))
    bubbles = 0
    for r in range(K):
        y = GY + r * PITCH
        out.append(mono(24, y + CELL_H / 2 + 6, f"GPU {r + 1}", 16, INK))
        for c in range(NCOL):
            x = GX + c * CELL_W
            if r <= c < r + M:
                out.append(rect(x + 2, y + 2, CELL_W - 4, CELL_H - 4, FILL_BLUE, BLUE, rx=3, width=1.2))
                out.append(text(x + CELL_W / 2, y + CELL_H / 2 + 5, f"请求 {c - r + 1}", 13, BLUE, "bold"))
            else:
                bubbles += 1
                out.append(rect(x + 2, y + 2, CELL_W - 4, CELL_H - 4, FILL_RED, RED, rx=3, width=1.2))
                out.append(text(x + CELL_W / 2, y + CELL_H / 2 + 5, "停", 14, RED, "bold"))
    total = K * NCOL
    y = GY + K * PITCH + 24
    out.append(text(GX, y, f"气泡 {bubbles} 格 ÷ 总共 {K} × {NCOL} = {total} 格 = {bubbles / total:.0%}",
                    16, RED, "bold", anchor="start"))
    return out


if __name__ == "__main__":
    save("llm-pp-ratio", W, GY + K * PITCH + 36, build(), left=-(W - (GX + NCOL * CELL_W + 24)) / 2)
