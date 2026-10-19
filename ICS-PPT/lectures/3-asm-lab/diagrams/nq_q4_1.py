#!/usr/bin/env python3
"""Q4_1: the block strip of lecture 2 over the same six weights on its grid.

Top, the 20 bytes of a Q4_1 block as lecture 2's gguf_q4_blocks.py draws
them: d orange, m gold, qs blue. Below, the six weights of nq_q4_grid.py
quantized by the Q4_1 rule of include/nano_quant.h: lo = -0.40, hi = 1.40,
d = (hi - lo) / 15 = 0.12, m = lo, codes min(15, (int)((x - lo) * id + 0.5)),
grid points d * q + m, so q = 0 sits at m and all sixteen points lie inside
the data range (no grey point). The grid is drawn by nq_q4_grid.py's
helpers, offset below the strip. Run it to refresh ../assets/nq_q4_1.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import nq_q4_grid as g
from svgkit import BLUE, FILL_BLUE, FILL_ORANGE, INK, MONO, MUTED, ORANGE, rect, save, text

GOLD, FILL_GOLD = "#b8860b", "#fbf2dc"

W = g.W
STRIP = 118                       # height of the strip band above the grid
H = STRIP + g.H
X0, CELL, CELL_H, SY = 160, 40, 40, 56


def strip():
    fields = [(2, FILL_ORANGE, ORANGE, "d"), (2, FILL_GOLD, GOLD, "m"),
              (16, FILL_BLUE, BLUE, "qs")]
    out = [text(X0 - 8, SY + CELL_H / 2 + 5, "Q4_1", 13.5, INK, "bold", anchor="end")]
    x = X0
    for count, fill, stroke, label in fields:
        for _ in range(count):
            out.append(rect(x, SY, CELL, CELL_H, fill, stroke, rx=3, width=1.3))
            out.append(text(x + CELL / 2, SY + CELL_H / 2 + 4.5, label, 11.5, MUTED, font=MONO))
            x += CELL
    out.append(text(X0 + CELL, SY - 12, "2 字节 d", 12, ORANGE, "bold"))
    out.append(text(X0 + 3 * CELL, SY - 12, "2 字节 m = lo", 12, GOLD, "bold"))
    out.append(text(X0 + 12 * CELL, SY - 12, "16 字节 qs：第 j 个字节存 q[j] 与 q[j + 16]", 12,
                    BLUE, "bold"))
    out.append(text(x, SY + CELL_H + 18, "合计 20 字节 · 5.00 位/权重", 12.5, INK, "bold",
                    anchor="end"))
    return out


def q4_1():
    lo, hi = min(g.WEIGHTS), max(g.WEIGHTS)
    d = (hi - lo) / 15.0
    idv = 1.0 / d
    grid = [(q, d * q + lo) for q in range(16)]
    codes = {w: min(15, int((w - lo) * idv + 0.5)) for w in g.WEIGHTS}
    return g.figure(f"Q4_1：lo = {g.minus(lo, '.2f')}，hi = {hi:.2f}，d = (hi − lo) / 15 = {d:.2f}",
                    grid, codes, 12, "d = 0.12", 0, "q = 0 对应 m = lo", dy=STRIP)


if __name__ == "__main__":
    save("nq_q4_1", W, H, strip() + q4_1())
