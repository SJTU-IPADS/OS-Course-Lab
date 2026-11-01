#!/usr/bin/env python3
"""How a Q4_K super-block spends its 144 bytes, in the order of its bytes.

Lecture 2's q4k_superblock.py redrawn with the fields in byte order: d and
dmin (orange, 4 bytes), the 12 scale bytes (green), the 128 code bytes
(blue), then the eight sub-blocks of 32 weights that the scale bytes steer.
The super-block bar is grey: the page also shows nq_scale_bytes.svg, where
gold is m.
The code field says which sub-blocks share a byte, from the bytes rule of
include/nano_quant.h; the last line is the restore formula. Run it to refresh
../assets/nq_q4_k.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE, line,
                    rect, save, text)

W, H = 1120, 220
GREEN, FILL_GREEN = "#196b24", "#e7f2e4"

TOP_X, TOP_Y, TOP_W, TOP_H = 80, 4, 960, 40
MID_Y, MID_H = 60, 58
SUB_Y, SUB_H = 140, 44
SUB_N, SUB_W, SUB_GAP = 8, 106, 14


def field(x, w, fill, stroke, head, body):
    return [rect(x, MID_Y, w, MID_H, fill, stroke, rx=4, width=1.6),
            text(x + w / 2, MID_Y + 24, head, 13.5, stroke, "bold"),
            text(x + w / 2, MID_Y + 43, body, 12, MUTED)]


def build():
    out = [rect(TOP_X, TOP_Y, TOP_W, TOP_H, FILL_GREY, INK, rx=5, width=2.0),
           text(TOP_X + TOP_W / 2, TOP_Y + TOP_H / 2 + 6,
                "Q4_K 超块：256 个权重 · 144 字节 · 4.50 位/权重", 16, INK, "bold")]
    fields = [(220, FILL_ORANGE, ORANGE, "d, dmin · 字节 0..3", "两个 fp16：sc 与 m 的步长"),
              (270, FILL_GREEN, GREEN, "scales · 字节 4..15", "8 对 (sc[j], m[j])，各 6 位"),
              (450, FILL_BLUE, BLUE, "qs · 字节 16..143",
               "256 个 4 位编码：第 g 组 32 字节存子块 2g（低 4 位）与 2g+1（高 4 位）")]
    x = TOP_X
    for w, fill, stroke, head, body in fields:
        out += field(x, w, fill, stroke, head, body)
        out.append(line(TOP_X + TOP_W / 2, TOP_Y + TOP_H, x + w / 2, MID_Y, LINE, 1.2,
                        dash="4 4"))
        if fill is FILL_GREEN:
            scales_mid = x + w / 2
        x += w + 10

    span = SUB_N * SUB_W + (SUB_N - 1) * SUB_GAP
    sx = (W - span) / 2
    for i in range(SUB_N):
        x = sx + i * (SUB_W + SUB_GAP)
        out.append(rect(x, SUB_Y, SUB_W, SUB_H, FILL_GREEN, GREEN, rx=4, width=1.4))
        out.append(text(x + SUB_W / 2, SUB_Y + 19, f"子块 {i}", 12.5, GREEN, "bold"))
        out.append(text(x + SUB_W / 2, SUB_Y + 36, "32 个权重", 11.5, MUTED))
        out.append(line(scales_mid, MID_Y + MID_H, x + SUB_W / 2, SUB_Y, LINE, 1.0,
                        dash="3 4"))
    out.append(text(W / 2, SUB_Y + SUB_H + 25,
                    "子块 j 的步长 D[j] = d · sc[j]，偏移 M[j] = dmin · m[j]；还原 x = D[j] · q − M[j]，"
                    "q 取 0..15", 13.5, INK, "bold"))
    return out


if __name__ == "__main__":
    save("nq_q4_k", W, H, build())
