#!/usr/bin/env python3
"""Reading one supercell of a 16 x 8 DRAM chip, in three frames.

The chip holds 16 supercells of 8 bits, arranged as a 4 x 4 array, and an
internal row buffer of one row. The memory controller sends the address in two
halves over the same 2-bit address pins. Frame 1: the organisation, every
supercell labelled (row, column). Frame 2: the row address (RAS = 2) selects
row 2 and the whole row is copied to the row buffer. Frame 3: the column
address (CAS = 1) selects supercell (2, 1) in the row buffer, and its 8 bits go
to the controller on the data pins. Each frame carries a one-line caption and, at the top, a title that names the
supercell being read.
Drawn at 1100 px wide and shown at 820 px.
Run it to refresh ../assets/dram-read-1.svg .. dram-read-3.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK, MUTED, ORANGE, WHITE,
                    arrow, elbow, mono, rect, save, text)

W, H = 1100, 490
TITLE_H = 44                            # room for the title above the chip
CTRL = (10, 84, 130, 292)               # memory controller: x, y, w, h
CHIP = (300, 4, 790, 368)
GX, GY, CW, CH = 470, 74, 108, 46        # grid origin and cell size
BY = 290                                 # top of the row buffer
ADDR_Y, DATA_Y = 116, 352
ROW, COL = 2, 1
CAPTIONS = ["① 芯片有 4 行 4 列的超单元，以及保存一整行的行缓冲区",
            "② RAS（行访问选通）送出行地址 2：第 2 行整行复制到行缓冲区",
            "③ CAS（列访问选通）送出列地址 1：超单元 (2,1) 的 8 位送到数据线"]


def cell(x, y, fill, stroke, label, tone=INK, width=1.4):
    return [rect(x, y, CW, CH, fill, stroke, rx=3, width=width),
            text(x + CW / 2, y + CH / 2 + 7, label, 20, tone, font="monospace")]


def frame(k):
    out = []
    cx, cy, cw, ch = CHIP
    out.append(rect(cx, cy, cw, ch, "none", MUTED, rx=6, width=1.4, dash="6 4"))
    out.append(text(cx + cw - 14, cy + 28, "16 × 8 DRAM 芯片", 19, INK, "bold", "end"))
    x, y, w, h = CTRL
    out += [rect(x, y, w, h, FILL_GREY, MUTED, rx=6, width=1.6),
            text(x + w / 2, y + h / 2 - 4, "内存", 21, INK, "bold"),
            text(x + w / 2, y + h / 2 + 24, "控制器", 21, INK, "bold")]

    # the wires between controller and chip
    out.append(arrow(x + w, ADDR_Y, cx, ADDR_Y, MUTED, 2))
    out.append(text(220, ADDR_Y + 26, "addr（2 位）", 17, MUTED, "bold"))
    out.append(arrow(x + w, DATA_Y, cx, DATA_Y, MUTED, 2, both=True))
    out.append(text(220, DATA_Y + 26, "data（8 位）", 17, MUTED, "bold"))

    # the supercell array
    for j in range(4):
        hot = k == 3 and j == COL
        out.append(text(GX + j * CW + CW / 2, GY - 12, f"列 {j}", 19,
                        ORANGE if hot else INK, "bold"))
    for i in range(4):
        hot = k >= 2 and i == ROW
        out.append(text(GX - 12, GY + i * CH + CH / 2 + 7, f"行 {i}", 19,
                        ORANGE if hot else INK, "bold", "end"))
        for j in range(4):
            selected = k >= 2 and i == ROW
            out += cell(GX + j * CW, GY + i * CH, FILL_ORANGE if selected else FILL_GREEN,
                        ORANGE if selected else GREEN, f"({i},{j})")
    out.append(text(GX + 4 * CW + 16, GY + CH - 4, "每个方格是一个", 17, MUTED, "bold", "start"))
    out.append(text(GX + 4 * CW + 16, GY + CH + 18, "超单元，共 8 位", 17, MUTED, "bold", "start"))

    # the row buffer
    out.append(text(GX - 18, BY + CH / 2 + 7, "行缓冲区", 17, MUTED, "bold", "end"))
    for j in range(4):
        if k >= 2:
            hot = k == 3 and j == COL
            out += cell(GX + j * CW, BY, ORANGE if hot else FILL_ORANGE, ORANGE,
                        f"({ROW},{j})", WHITE if hot else INK, 2.4 if hot else 1.4)
        else:
            out += cell(GX + j * CW, BY, WHITE, GREEN, "")

    # the data moving this frame
    rowy = GY + ROW * CH + CH / 2
    if k == 2:
        out.append(text(220, ADDR_Y - 14, "RAS = 2", 19, ORANGE, "bold"))
        out.append(elbow([(cx, ADDR_Y), (cx + 60, ADDR_Y), (cx + 60, rowy), (GX - 66, rowy)],
                         ORANGE, 2.6))
        out.append(elbow([(GX + 4 * CW + 2, rowy), (GX + 4 * CW + 30, rowy),
                          (GX + 4 * CW + 30, BY + CH / 2), (GX + 4 * CW + 2, BY + CH / 2)],
                         ORANGE, 2.6))
    if k == 3:
        colx = GX + COL * CW + CW / 2
        out.append(text(220, ADDR_Y - 14, "CAS = 1", 19, ORANGE, "bold"))
        out.append(elbow([(cx, ADDR_Y), (cx + 60, ADDR_Y), (cx + 60, GY - 52), (colx, GY - 52),
                          (colx, GY - 34)], ORANGE, 2.6))
        out.append(elbow([(colx, BY + CH), (colx, DATA_Y), (x + w + 2, DATA_Y)], ORANGE, 3))
        out.append(text(220, DATA_Y - 14, "(2,1) 的 8 位", 17, ORANGE, "bold"))
    out.append(text(W / 2, H - TITLE_H - 10, CAPTIONS[k - 1], 20, INK, "bold"))
    return [text(W / 2, 28, "读取超单元 (2,1)：行地址 2，列地址 1", 24, INK, "bold"),
            f'<g transform="translate(0,{TITLE_H})">', *out, "</g>"]


if __name__ == "__main__":
    for k in (1, 2, 3):
        save(f"dram-read-{k}", W, H, frame(k))
