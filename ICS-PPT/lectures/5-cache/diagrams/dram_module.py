#!/usr/bin/env python3
"""A 64 MB memory module: eight 8M x 8 DRAM chips, one byte of each 64-bit word per chip, in four frames.

Each chip is drawn as its 4 x 4 supercell array in miniature with its row
buffer under it; the eight chips together are framed by a dashed box labelled
as main memory. The controller sits under the chips and holds the 64-bit word
being assembled. Chip 0 supplies bits 0-7, chip 7 bits 56-63.
Frame 1: the controller converts address A into (row i, column j).
Frame 2: the row address i goes to all eight chips, and each chip reads its
whole row i into its row buffer (light orange).
Frame 3: the column address j goes to all eight chips, and each chip selects
supercell j of its row buffer (solid orange).
Frame 4: the eight selected bytes travel to the controller, which puts them
together into the 64-bit word at A.
The address bus leaves the address box on its left side and runs round the
chips. Drawn at 1100 px wide and shown at 1040 px.
Run it to refresh ../assets/dram-module-1.svg .. dram-module-4.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK, MUTED, ORANGE, WHITE,
                    arrow, elbow, rect, save, text)

W, H = 1100, 404
X0, CW, GAP = 146, 104, 12               # first chip, chip width, gap between chips
CHIP_Y, CHIP_H = 64, 130
BUS_Y = 20
ARRAY_Y, BUF_Y = 44, 106                 # offsets inside a chip
PAD = 40                                 # space between the controller's left edge and its address box
CELL = 13                                # a supercell in the miniature array
CTRL = (356, 250, 468, 112)              # controller box: x, y, w, h
WORD_X, WORD_Y, WORD_W, WORD_H = 504, 266, 36, 36   # the 64-bit word, one cell per chip
SEL = (2, 1)                             # the supercell (i, j) every chip selects
CAPTIONS = ["① 内存控制器把地址 A 转换为（行 i，列 j）",
            "② 行地址 i 同时送给 8 个芯片，每个芯片把第 i 行整行读入自己的行缓冲",
            "③ 列地址 j 同时送给 8 个芯片，每个芯片选出行缓冲中的第 j 个超单元",
            "④ 8 个芯片各送出 8 位，控制器把这 8 个字节拼成地址 A 处的 64 位字"]
BUS_LABELS = ["addr（行 i，列 j）", "addr：行 i", "addr：列 j", "addr"]


def mid(k):
    """Centre of the k-th chip from the left; k = 0 is DRAM 7."""
    return X0 + k * (CW + GAP) + CW / 2


def chip(k, n):
    """One chip: name, supercell array, row buffer."""
    x = X0 + k * (CW + GAP)
    out = [rect(x, CHIP_Y, CW, CHIP_H, FILL_GREEN, GREEN, rx=4, width=1.6),
           text(mid(k), CHIP_Y + 20, f"DRAM {7 - k}", 16, INK, "bold"),
           text(mid(k), CHIP_Y + 35, "8M × 8", 12, MUTED)]
    left = mid(k) - 2 * CELL
    for i in range(4):
        for j in range(4):
            row = n >= 2 and i == SEL[0]
            out.append(rect(left + j * CELL, CHIP_Y + ARRAY_Y + i * CELL, CELL, CELL,
                            FILL_ORANGE if row else WHITE, ORANGE if row else GREEN,
                            rx=0, width=1))
    for j in range(4):
        hot = n >= 3 and j == SEL[1]
        out.append(rect(left + j * CELL, CHIP_Y + BUF_Y, CELL, CELL,
                        ORANGE if hot else FILL_ORANGE if n >= 2 else WHITE,
                        ORANGE if n >= 2 else GREEN, rx=0, width=1.6 if hot else 1))
    return out


def frame(n):
    out = []
    right = X0 + 8 * CW + 7 * GAP
    cx, cy, cw, ch = CTRL
    addr_y = cy + 16 + 32                # the address box's centre line

    # main memory: the eight chips together, with what each row of the figure is
    out.append(rect(X0 - 12, CHIP_Y - 10, right - X0 + 24, CHIP_H + 20, "none", MUTED, rx=8,
                    width=1.6, dash="7 5"))
    out.append(text(X0 - 12, CHIP_Y - 18, "内存", 20, INK, "bold", "start"))
    out.append(text(X0 - 22, CHIP_Y + ARRAY_Y + 2 * CELL + 6, "超单元阵列", 15, MUTED, "bold", "end"))
    out.append(text(X0 - 22, CHIP_Y + BUF_Y + CELL - 2, "行缓冲", 15, MUTED, "bold", "end"))

    # the memory controller, with the address and the 64-bit word under assembly
    out.append(rect(cx, cy, cw, ch, FILL_GREY, MUTED, rx=6, width=1.6))
    out.append(text(cx + PAD, cy + ch - 10, "内存控制器", 18, MUTED, "bold", "start"))
    hot = n == 1
    out.append(rect(cx + PAD, cy + 16, 100, 64, FILL_ORANGE if hot else WHITE,
                    ORANGE if hot else MUTED, rx=4, width=2 if hot else 1.4))
    out.append(text(cx + PAD + 50, cy + 42, "地址 A", 17, INK, "bold"))
    out.append(text(cx + PAD + 50, cy + 68, "(i, j)", 17, ORANGE if n >= 1 else INK, "bold"))
    for k in range(8):
        wx = WORD_X + k * WORD_W
        full = n == 4
        out.append(rect(wx, WORD_Y, WORD_W, WORD_H, FILL_ORANGE if full else WHITE,
                        ORANGE if full else MUTED, rx=2, width=1.6 if full else 1.2,
                        dash=None if full else "4 3"))
        if full:
            out.append(text(wx + WORD_W / 2, WORD_Y + 23, f"B{7 - k}", 14, INK))
    out.append(text(WORD_X + 4 * WORD_W, WORD_Y + WORD_H + 26, "地址 A 处的 64 位字", 16,
                    INK if n == 4 else MUTED, "bold"))

    # the address bus: leaves the address box on its left, runs round the chips
    bus = ORANGE if n in (2, 3) else MUTED
    out.append(elbow([(cx + PAD, addr_y), (30, addr_y), (30, BUS_Y), (mid(7), BUS_Y)], bus,
                     2.6 if n in (2, 3) else 2, head=False))
    out.append(text(mid(7) + 36, BUS_Y - 6, BUS_LABELS[n - 1],
                    16, bus, "bold", "end"))
    for k in range(8):
        out.append(arrow(mid(k), BUS_Y, mid(k), CHIP_Y - 12, bus, 2.6 if n in (2, 3) else 2))

    for k in range(8):
        out += chip(k, n)

    # frame 4: the byte in each row buffer goes to its cell of the word
    if n == 4:
        for k in range(8):
            hx = mid(k) - 2 * CELL + SEL[1] * CELL + CELL / 2
            wx = WORD_X + k * WORD_W + WORD_W / 2
            out.append(elbow([(hx, CHIP_Y + BUF_Y + CELL + 2), (hx, CHIP_Y + CHIP_H + 24),
                              (wx, WORD_Y - 24), (wx, WORD_Y - 2)], ORANGE, 2.2))
    out.append(text(W / 2, H - 12, CAPTIONS[n - 1], 20, INK, "bold"))
    return out


if __name__ == "__main__":
    for n in (1, 2, 3, 4):
        save(f"dram-module-{n}", W, H, frame(n))
