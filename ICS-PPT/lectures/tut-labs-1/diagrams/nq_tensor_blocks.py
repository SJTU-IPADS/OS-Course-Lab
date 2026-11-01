#!/usr/bin/env python3
"""How one tensor turns into blocks: the framework loops, one call per block.

Left, the tensor blk.0.attn_q.weight as a grid of 2048 rows (a row drawn
every 128). Its first row is blown up to the right: 2048 weights, cut from
the front into 64 blocks of 32. The first block is blown up again: the 32
floats the framework hands to q4_0_quantize as x, and the 18 bytes the
function writes at blk. The byte count on the right is the one
`nano-quant plan --recipe q4_0` prints for this tensor. Grey is data the
framework reads, orange the bytes the student's function writes. Run it to
refresh ../assets/nq_tensor_blocks.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE,
                    arrow, line, mono, rect, save, text)

W, H = 1120, 330
TX, TY, TW, TH = 20, 44, 300, 200        # the tensor
ROW_H = TH / 16                          # one drawn row stands for 128
SX, SY, SW, SH = 380, 46, 720, 24        # the row strip: 64 blocks
XS, XY, XC, XH = 380, 140, 14, 28        # x: 32 floats
BX, BY, BC, BH = 380, 232, 24, 28        # blk: 18 bytes
NOTE_X = 860


def guide(x1, y1, x2, y2):
    return line(x1, y1, x2, y2, BLUE, 1.2, dash="4 4")


def build():
    out = [text(TX, TY - 14, "blk.0.attn_q.weight：2048 行 × 2048 个 BF16 权重", 13.5, INK,
                "bold", anchor="start"),
           rect(TX, TY, TW, TH, FILL_GREY, MUTED, rx=2, width=1.4)]
    for i in range(1, 16):
        out.append(line(TX, TY + i * ROW_H, TX + TW, TY + i * ROW_H, LINE, 0.8))
    out.append(rect(TX, TY, TW, ROW_H, FILL_BLUE, BLUE, rx=0, width=1.6))
    out.append(text(TX + TW / 2, TY + TH / 2 + 12, "行优先存放：一行接一行", 13, MUTED))
    out.append(text(TX, TY + TH + 24, "每行独立切块，行长 2048 是 32 的倍数", 13, INK,
                    anchor="start"))

    # the first row, cut into 64 blocks
    out.append(text(SX, SY - 10, "第 0 行：2048 个权重 = 64 个块 × 32 个", 13.5, INK, "bold",
                    anchor="start"))
    cw = SW / 64
    for i in range(64):
        out.append(rect(SX + i * cw, SY, cw, SH, FILL_BLUE if i == 0 else FILL_GREY,
                        BLUE if i == 0 else LINE, rx=0, width=1.4 if i == 0 else 0.8))
    out.append(guide(TX + TW, TY, SX, SY))
    out.append(guide(TX + TW, TY + ROW_H, SX, SY + SH))

    # block 0: the 32 floats in, the 18 bytes out
    out.append(text(XS, XY - 10, "块 0 的输入 x：32 个 float，x[0] … x[31]", 13.5, INK, "bold",
                    anchor="start"))
    for i in range(32):
        out.append(rect(XS + i * XC, XY, XC, XH, FILL_GREY, LINE, rx=0, width=0.8))
    out.append(guide(SX, SY + SH, XS, XY))
    out.append(guide(SX + cw, SY + SH, XS + 32 * XC, XY))
    mid = XS + 16 * XC
    out.append(arrow(mid, XY + XH + 4, mid, BY - 6, ORANGE, 2.2))
    out.append(mono(mid + 14, (XY + XH + BY) / 2 + 5, "q4_0_quantize(x, blk)", 14, ORANGE,
                    "bold"))
    for i in range(18):
        out.append(rect(BX + i * BC, BY, BC, BH, FILL_ORANGE, ORANGE, rx=0, width=1.0))
    out.append(text(BX, BY + BH + 20, "块 0 的输出 blk：18 个字节，写在 dst 的开头；块 b 写在 dst + 18·b",
                    13.5, INK, "bold", anchor="start"))

    # the count the plan prints
    out.append(text(NOTE_X, XY + 4, "框架的循环", 13.5, INK, "bold", anchor="start"))
    out.append(text(NOTE_X, XY + 26, "每行 64 次调用", 13, INK, anchor="start"))
    out.append(text(NOTE_X, XY + 48, "2048 行 × 64 块 × 18 字节", 13, INK, anchor="start"))
    out.append(text(NOTE_X, XY + 70, "= 2 359 296 字节", 13, INK, "bold", anchor="start"))
    return out


if __name__ == "__main__":
    save("nq_tensor_blocks", W, H, build())
