#!/usr/bin/env python3
"""A 4 x 4 int matrix W stored by rows, coloured by the order its elements are read.

Two figures, no cache in either. traverse-row.svg: the loop with j innermost, which reads
addresses 0, 4, 8, ... (stride 1). traverse-col.svg: the loop with i innermost, which reads
addresses 0, 16, 32, 48, 4, ... (stride 4). Top: the matrix as the program sees it, every
element filled with the colour of its read number (pale to dark, first to last) and carrying
its own number 1..16 in row-major order, so the matrix is the same in both figures and only the loop differs. Bottom: the same 16 ints in memory, row after row, with the same colours. When
the colour changes evenly along the memory strip, the reads of neighbouring times lie at
neighbouring addresses (spatial locality); when it jumps, they do not.
Drawn at the width it is shown on the slide (1000 px).
Run it to refresh ../assets/traverse-row.svg and ../assets/traverse-col.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_ORANGE, INK, MUTED, WHITE, arrow, brace, listing, rect, save,
                    text)

W, H = 1000, 400
M = N = 4
LETTERS = [str(k + 1) for k in range(16)]    # the element in row-major order: the same in both figures
MX, MY, CELL = 130, 46, 52               # matrix origin and cell size
FX, FY, FW, FH = 36, 292, 58, 48         # memory strip origin and cell size
STOPS = [(0.0, (247, 224, 138)), (0.4, (233, 113, 50)), (0.75, (21, 96, 130)),
         (1.0, (14, 40, 65))]


def colour(t, count):
    """Colour of the t-th read (0-based) out of `count`: pale yellow, orange, blue, ink."""
    x = t / (count - 1)
    for (x0, c0), (x1, c1) in zip(STOPS, STOPS[1:]):
        if x <= x1:
            f = (x - x0) / (x1 - x0)
            return "#%02x%02x%02x" % tuple(round(a + (b - a) * f) for a, b in zip(c0, c1))
    return "#%02x%02x%02x" % STOPS[-1][1]


def read_number(i, j, by_column):
    return (j * M + i) if by_column else (i * N + j)


def build(by_column):
    count = M * N
    out = []

    out.append(text(MX - 80, MY + M * CELL / 2 + 6, "W[4][4]", 20, INK, "bold", "middle"))
    for j in range(N):
        out.append(text(MX + j * CELL + CELL / 2, MY - 10, f"j = {j}", 15, MUTED))
    for i in range(M):
        out.append(text(MX - 12, MY + i * CELL + CELL / 2 + 5, f"i = {i}", 15, MUTED, anchor="end"))
        for j in range(N):
            t = read_number(i, j, by_column)
            fill = colour(t, count)
            out.append(rect(MX + j * CELL, MY + i * CELL, CELL, CELL, fill, WHITE, rx=3, width=2))
            out.append(text(MX + j * CELL + CELL / 2, MY + i * CELL + CELL / 2 + 7, LETTERS[i * N + j], 20,
                            INK if t < 6 else WHITE, "bold"))

    lx, ly = 560, 82                     # colour key
    out.append(text(lx, ly - 22, "颜色表示引用的先后", 18, INK, "bold", "start"))
    for t in range(count):
        out.append(rect(lx + t * 20, ly, 20, 22, colour(t, count), WHITE, rx=0, width=1))
    out.append(text(lx, ly + 46, "先", 17, MUTED, "bold", "start"))
    out.append(text(lx + count * 20, ly + 46, "后", 17, MUTED, "bold", "end"))
    lines = (["for (j = 0; j < N; j++)", "    for (i = 0; i < M; i++)", "        sum += W[i][j];"]
             if by_column else
             ["for (i = 0; i < M; i++)", "    for (j = 0; j < N; j++)", "        sum += W[i][j];"])
    out += listing(lx, ly + 84, lines, 19, 28, marks={1: FILL_ORANGE})

    ax = MX + N * CELL / 2
    out.append(arrow(ax, MY + M * CELL + 8, ax, FY - 8, MUTED, 2.4))
    out.append(text(ax + 14, MY + M * CELL + 34, "按行存放，地址由低到高：W[i][j] 的地址 = (i × N + j) × 4",
                    17, MUTED, "bold", "start"))
    for a in range(count):
        i, j = divmod(a, N)
        t = read_number(i, j, by_column)
        x = FX + a * FW
        out.append(rect(x, FY, FW, FH, colour(t, count), WHITE, rx=3, width=2))
        out.append(text(x + FW / 2, FY + FH / 2 + 7, LETTERS[a], 20, INK if t < 6 else WHITE,
                        "bold"))
    for i in range(M):
        out += brace(FX + i * N * FW + 4, FX + (i + 1) * N * FW - 4, FY + FH + 30, MUTED, f"W[{i}]",
                     16, 7)
    return out


if __name__ == "__main__":
    save("traverse-row", W, H, build(False))
    save("traverse-col", W, H, build(True))
