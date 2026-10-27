#!/usr/bin/env python3
"""Pipeline parallelism with one request: one GPU busy per cycle.

The time-space grid of timegrid.py with GPUs for rows and cycles for
columns: a row is one GPU, a cell is what it does in one cycle (seven layers
of one request). Request 1 walks down the diagonal; every other cell is a
bubble (the dashed "bubble" cell of timegrid.TONE), 12 of the 16. The helpers below are shared by llm_pp_fill.py,
llm_pp_decode.py and llm_pp_batch.py, so the cells of the four figures never
move between pages.
Run it to refresh ../assets/llm-pp-single.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, MUTED, WHITE, elbow, line, mono, save, text
from timegrid import CH, CW, LX, TOP, VIEW_LEFT, cell, cycle_numbers

W = 1120
K = 4                                # GPUs
PITCH = CH + 6
BOTTOM = TOP + (K - 1) * PITCH + CH


def gy(g):
    """Top of GPU g's row, g = 1..K."""
    return TOP + (g - 1) * PITCH


def gpu_grid(steps, work, emph=None, color=None):
    """Row labels, cycle numbers and one cell per (GPU, cycle) up to `steps`.

    `work(g, t)` names the cell of GPU g in cycle t (1-based); None is a bubble."""
    out = cycle_numbers(steps, emph=emph, color=color or INK)
    for g in range(1, K + 1):
        out.append(mono(24, gy(g) + CH / 2 + 6, f"GPU {g}", 16, INK))
        for t in range(1, steps + 1):
            out += cell(t - 1, gy(g), work(g, t) or "bubble")
    return out


def handoff(t):
    """GPU K's cell in cycle t hands its word to GPU 1's cell in cycle t + 1:
    an arrow up the boundary between the two columns."""
    x = LX + t * CW
    y0, y1 = gy(K) + CH / 2, gy(1) + CH / 2
    return [line(x, y0, x, y1, WHITE, 5),
            elbow([(x - 14, y0), (x, y0), (x, y1), (x + 16, y1)], INK, 2)]


def build():
    out = gpu_grid(K, lambda g, t: "请求 1" if g == t else None)
    tx = LX + K * CW + 28
    out.append(text(tx, TOP + 1.5 * PITCH + 4, "一个请求依次经过 4 块 GPU", 17, INK,
                    anchor="start"))
    out.append(text(tx, TOP + 2.5 * PITCH + 4, "bubble：这个周期 GPU 没有可算的输入", 17, MUTED, "bold",
                    anchor="start"))
    return out


if __name__ == "__main__":
    save("llm-pp-single", W, BOTTOM + 14, build(), left=VIEW_LEFT)
