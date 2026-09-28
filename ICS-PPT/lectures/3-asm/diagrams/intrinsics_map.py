#!/usr/bin/env python3
"""AVX2 intrinsics on the left, the instructions they become on the right.

The load of x has no instruction of its own: gcc folds it into the memory
operand of vpmulld, so its line joins the vpmulld row (dashed).
Run it to refresh ../assets/intrinsics-map.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, elbow, line, mono, rect, save,
                    text)

W, H = 1120, 440
LX, LW, RX, RW = 20, 520, 700, 400
TOP, RH = 70, 58

ROWS = [("__m256i", "%ymm 寄存器（256 位）", FILL_GREY, MUTED),
        ("_mm256_setzero_si256()", "vpxor %xmm1, %xmm1, %xmm1", FILL_GREY, MUTED),
        ("_mm256_loadu_si256(&w[i])", "vmovdqu", FILL_BLUE, BLUE),
        ("_mm256_loadu_si256(&x[i])", None, WHITE, BLUE),
        ("_mm256_mullo_epi32(va, vb)", "vpmulld", FILL_ORANGE, ORANGE),
        ("_mm256_add_epi32(vsum, vprod)", "vpaddd", FILL_GREEN, GREEN)]


def build():
    out = [text(LX + LW / 2, 36, "C 语言 Intrinsics（<immintrin.h>）", 17, INK, "bold"),
           text(RX + RW / 2, 36, "机器指令", 17, INK, "bold")]
    for k, (c, asm, fill, stroke) in enumerate(ROWS):
        y = TOP + k * RH
        out.append(rect(LX, y, LW, RH - 12, fill, stroke, rx=4, width=1.6,
                        dash="5 4" if asm is None else None))
        out.append(mono(LX + 16, y + 29, c, 16, INK, "bold" if k else "normal"))
        if asm is None:
            continue
        out.append(rect(RX, y, RW, RH - 12, fill, stroke, rx=4, width=1.6))
        if asm.startswith("%"):
            out.append(text(RX + 16, y + 29, asm, 16, INK, anchor="start"))
        else:
            out.append(mono(RX + 16, y + 29, asm, 16, INK, "bold"))
        out.append(line(LX + LW + 4, y + 23, RX - 4, y + 23, stroke, 2))
    # x's load joins vpmulld
    y3, y4 = TOP + 3 * RH + 23, TOP + 4 * RH + 23
    out.append(elbow([(LX + LW + 4, y3), (620, y3), (620, y4 - 8), (RX - 4, y4 - 8)],
                     BLUE, 2, dash="6 4", head=False))
    out.append(text(RX + 16, TOP + 3 * RH + 29, "没有单独的指令：并入 vpmulld 的内存操作数",
                    14, BLUE, anchor="start"))
    return out


if __name__ == "__main__":
    save("intrinsics-map", W, H, build())
