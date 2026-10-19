#!/usr/bin/env python3
"""A vector instruction: the VEX prefix names a third register, the lanes are
computed one by one, and the result is written with vset. The instruction
is c5 ed fe d0  vpaddd %ymm0,%ymm2,%ymm2 at 401243 of guest/simd.

Run it to refresh ../assets/cpu_vector.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, GREEN, INK, LINE, MUTED, ORANGE,
                    WHITE, arrow, cells, line, rect, save, text)
from cpu_kit import MODRM, OPCODE, PREFIX, byte_row, code, note

W, H = 1120, 330
LW, LH = 54, 30


def lanes(x, y, name, values, fill, stroke):
    out = [code(x, y + 20, name, 14, INK, "bold")]
    out += cells(x + 70, y, values, LW, LH, [fill] * 8, stroke, 13)[0]
    return out


def build():
    out = [code(20, 30, "c5 ed fe d0   vpaddd %ymm0,%ymm2,%ymm2", 16, INK, "bold"),
           note(430, 30, "guest/simd，linear_simd 的循环体，地址 401243")]
    shapes, right = byte_row(20, 44, [(["c5", "ed"], PREFIX), (["fe"], OPCODE), (["d0"], MODRM)])
    out += shapes
    X = 200
    out.append(note(X, 66, "VEX 前缀：x86_begin 读出 x.v = 2（vvvv），x.vlen = 32（L = 1，ymm），"
                    "R、X、B 并入 x.rex；op = 0x11fe（map 1）", 13, INK))
    out.append(note(X, 88, "ModRM d0 = 11 010 000：reg = 2，rm = 0；三个操作数在 objdump 中的顺序是 rm、v、reg", 13, INK))
    # the lanes
    y = 112
    out += lanes(20, y, "Y[x.v]", [f"v{i}" for i in range(7, -1, -1)], FILL_BLUE, BLUE)
    out.append(note(20 + 70 + 8 * LW + 10, y + 20, "ymm2：第一个源", 13))
    out.append(text(72, y + LH + 34, "+", 18, INK, "bold"))
    out += lanes(20, y + LH + 14, "rm", [f"m{i}" for i in range(7, -1, -1)], FILL_BLUE, BLUE)
    out.append(note(20 + 70 + 8 * LW + 10, y + LH + 34, "ymm0：rm_vget(&x, &b, x.vlen)", 13))
    out.append(line(90, y + 2 * LH + 22, 90 + 8 * LW, y + 2 * LH + 22, INK, 1.4))
    out += lanes(20, y + 2 * LH + 30, "r", [f"v{i}+m{i}" for i in range(7, -1, -1)], FILL_GREEN, GREEN)
    out.append(note(20 + 70 + 8 * LW + 10, y + 2 * LH + 50, "局部的 u256，逐通道 r.d[i] = Y[x.v].d[i] + b.d[i]", 13))
    out.append(arrow(90 + 4 * LW, y + 3 * LH + 34, 90 + 4 * LW, y + 3 * LH + 58, GREEN, 2))
    out.append(code(90 + 4 * LW + 10, y + 3 * LH + 52, "vset(&x, x.reg, &r, x.vlen)", 14, INK, "bold"))
    out.append(note(90 + 4 * LW + 10, y + 3 * LH + 72, "写 ymm2；x.vlen 为 16（xmm）时只写 4 个通道，其余字节清零", 13, INK))
    out.append(note(20, y + 3 * LH + 72, "通道 d[0] 在最右", 12))
    out.append(note(20, H - 12, "目标 ymm2 同时是源：先算到局部的 r，最后一步才写出。vmovdqu 的 rm 可以是内存；vmovd 的目标是 rm（rm_set）", 13, INK))
    return out


if __name__ == "__main__":
    save("cpu_vector", W, H, build())
