#!/usr/bin/env python3
"""The grid of matvec_q4_0_kernel<<<rows, 256>>>: one thread block per row.

Left: the matrix W with its rows and the vector y beside it. Middle: the
grid, one block per row, each block reading its row and writing its y[r].
Right: one block opened into its 256 threads, 8 warps of 32, drawn as in
lecture 3's GPU figure. Threads are orange, as the followed block and warp
are in that figure.
Run it to refresh ../assets/pd-rest-grid.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, LINE,
                    MUTED, ORANGE, WHITE, arrow, box, brace, line, mono, rect,
                    save, text)

W, H = 1120, 270
MX, MY, MW, RH = 70, 72, 220, 28                # W and its rows, one row per RH
GAP = 56                                        # between row 2 and the last row
MH = 4 * RH + GAP
YX, YW = 318, 24                                # y
GX, GY, GW = 430, 36, 320                       # the grid
GH = MY + MH + 12 - GY
BH = RH - 6                                     # a block, 6 px apart from the next
OX, OW, OH, WP = 790, 310, 194, 16              # the opened block, the warp pitch
OY = GY + (GH - OH) / 2
ROWS = [0, 1, 2, None, "rows−1"]                # the rows drawn


def row_y(i):
    return MY + i * RH if i < 3 else MY + MH - RH


def build():
    out = []
    dots = MY + 3 * RH + GAP / 2 + 5             # the baseline of the three "…"
    # W: rows of Q4_0, and y
    out.append(text(MX + MW / 2, MY - 14, "W：rows 行", 14, INK, "bold"))
    out.append(rect(MX, MY, MW, MH, FILL_BLUE, BLUE, rx=2, width=1.4))
    out.append(text(YX + YW / 2, MY - 14, "y", 14, INK, "bold"))
    out.append(rect(YX, MY, YW, MH, FILL_GREEN, GREEN, rx=2, width=1.4))
    out.append(text(MX + MW / 2, dots, "…", 18, BLUE, "bold"))
    out.append(text(YX + YW / 2, dots, "…", 18, GREEN, "bold"))
    for i, r in enumerate(ROWS):
        if r is None:
            continue
        y = row_y(i)
        if i < 3:
            out.append(line(MX, y + RH, MX + MW, y + RH, BLUE, 0.8))
            out.append(line(YX, y + RH, YX + YW, y + RH, GREEN, 0.8))
        else:
            out.append(line(MX, y, MX + MW, y, BLUE, 0.8))
            out.append(line(YX, y, YX + YW, y, GREEN, 0.8))
        out.append(text(MX + MW / 2, y + RH / 2 + 4, f"第 {r} 行", 11.5, INK))
    # the grid
    out.append(rect(GX, GY, GW, GH, WHITE, INK, rx=6, width=1.8))
    out.append(text(GX + GW / 2, GY + 22, "Grid：<<<rows, 256>>>，rows 个线程块", 14, INK, "bold"))
    out.append(text(GX + GW / 2, dots, "…", 18, MUTED, "bold"))
    bx, bw = GX + 20, GW - 40
    for i, r in enumerate(ROWS):
        if r is None:
            continue
        y = row_y(i)
        out += box(bx, y + 3, bw, BH, f"Block {r}：读第 {r} 行，写 y[{r}]",
                   FILL_ORANGE if r == 1 else WHITE, ORANGE, 12.5, weight="normal")
        out.append(arrow(bx - 4, y + RH / 2, YX + YW + 4, y + RH / 2, ORANGE, 1.4))
    # one block opened: 256 threads = 8 warps
    y1 = row_y(1) + 3
    out.append(line(bx + bw, y1, OX, OY, ORANGE, 1.2, dash="4 3"))
    out.append(line(bx + bw, y1 + BH, OX, OY + OH, ORANGE, 1.2, dash="4 3"))
    out.append(rect(OX, OY, OW, OH, WHITE, ORANGE, rx=6, width=1.8))
    out.append(text(OX + OW / 2, OY + 24, "Block 1：256 个线程 = 8 个 Warp × 32", 14, INK, "bold"))
    tx, pitch, size = OX + 70, 7.2, 6
    for w in range(8):
        y = OY + 42 + w * WP
        out.append(mono(OX + 12, y + 7, f"Warp {w}", 12, INK))
        for k in range(32):
            out.append(rect(tx + k * pitch, y, size, size, FILL_ORANGE, ORANGE, rx=1, width=0.9))
    out.append(text(OX + OW / 2, OY + OH - 12, "线程 t = threadIdx.x，0 .. 255；行号 = blockIdx.x", 12, MUTED))
    return out


if __name__ == "__main__":
    save("pd-rest-grid", W, H, build(), left=25)
