#!/usr/bin/env python3
"""Why a block far from zero needs a zero point.

Both rows draw the same 32 weights — the first block of Llama-3.2-1B's final
RMSNorm, which lie between 1.539 and 2.625 — against the levels each format
can represent, on one shared axis. Q4_0's levels are pinned symmetric about
zero, so only four of the sixteen land on the data; Q4_1 stores the block's
minimum as well and puts all sixteen inside it.
Run it to refresh ../assets/zero-point.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, INK, LINE, MONO, MUTED, ORANGE, line, rect, svg, text

W, H = 1000, 268
AX0, AX1 = 152, 962                        # the axis in page coordinates
LO, HI = -2.85, 2.85                       # the value range both rows share
ROW_A, ROW_B = 78, 186                     # the two axes
TICK = 13

DATA_LO, DATA_HI = 1.539, 2.625            # the block's own range
D0 = 2.625 / 8                             # Q4_0: |extreme| / 8, codes 0..15 mean 8..-7
D1 = (DATA_HI - DATA_LO) / 15              # Q4_1: (max - min) / 15


def px(v):
    return AX0 + (v - LO) / (HI - LO) * (AX1 - AX0)


def band(y):
    """The shaded span the 32 weights actually occupy."""
    x0, x1 = px(DATA_LO), px(DATA_HI)
    return [rect(x0, y - 26, x1 - x0, 52, "#fce9df", ORANGE, rx=3, width=1.2,
                 dash="5 4")]


def axis(y, label, note, levels, inside):
    out = [line(AX0, y, AX1, y, LINE, 1.4),
           text(AX0 - 12, y - 6, label, 15.5, INK, "bold", anchor="end"),
           text(AX0 - 12, y + 15, note, 13, MUTED, anchor="end")]
    for v in levels:
        if not LO <= v <= HI:
            continue
        c = ORANGE if DATA_LO <= v <= DATA_HI else BLUE
        w = 2.0 if DATA_LO <= v <= DATA_HI else 1.4
        out.append(line(px(v), y - TICK, px(v), y + TICK, c, w))
    out.append(text(px((DATA_LO + DATA_HI) / 2), y - 34, inside, 14, ORANGE, "bold"))
    return out


def build():
    body = [text(W / 2, 30, "同一个块的 32 个权重，两种格式给出的级", 17.5, INK, "bold"),
            text(W / 2, 50, "取自最后一个 RMSNorm 的第 0 块，取值在 1.539 与 2.625 之间",
                 14, MUTED)]

    body += band(ROW_A) + band(ROW_B)
    body += axis(ROW_A, "Q4_0", "级关于 0 对称",
                 [(8 - c) * D0 for c in range(16)], "16 个级里只有 4 个落在数据上")
    body += axis(ROW_B, "Q4_1", "级覆盖 [min, max]",
                 [DATA_LO + q * D1 for q in range(16)], "16 个级全部落在数据上")

    body.append(line(px(0), ROW_A - 30, px(0), ROW_A + 30, MUTED, 1.2, "3 3"))
    body.append(text(px(0), ROW_A + 44, "0", 13.5, MUTED, font=MONO))

    body.append(text(W / 2, 244,
                     "步长 0.328 与 0.0724：相差 4.5 倍，实测误差也相差 4.5 倍",
                     15, INK, "bold"))
    return svg(W, H, body)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "zero-point.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
