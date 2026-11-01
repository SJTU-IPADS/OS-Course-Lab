#!/usr/bin/env python3
"""The structure of mini-ollama: the framework, the operator table, the three
sets of operators and the programs each set is linked into.

Left: the model file, model.c, main.c and selftest.c. Middle: struct ops of
ops.h, the only way the framework reaches an operator. Right: the three
files that implement the table, each linked into its own programs; the two
functions the lab asks for are marked in red.
Run it to refresh ../assets/pd-frame-ops.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, INK, LINE, MONO, MUTED, RED,
                    WHITE, arrow, box, elbow, line, mono, rect, save, text)

W, H = 1120, 400

LX, LW = 20, 160                      # the framework files
TX, TW, TY, TH = 240, 300, 92, 224    # the table
IX, IW = 610, 250                     # the three implementations
PX, PW = 900, 210                     # the programs

OPERATORS = ["matvec", "embed", "rmsnorm", "rope", "attn_scores", "softmax",
             "attn_values", "swiglu", "add", "argmax"]
MEMORY = ["init", "alloc", "upload", "download", "finish"]


def framework():
    out = []
    out += box(LX, 24, LW, 40, "q4_0.gguf", FILL_GREY, MUTED, 15, font=MONO)
    out += box(LX, 92, LW, 56, "model.c", WHITE, INK, 16, font=MONO,
               sub="载入 GGUF，找到张量")
    out += box(LX, 176, LW, 56, "main.c", WHITE, INK, 16, font=MONO,
               sub="逐个 Token 生成")
    out += box(LX, 260, LW, 56, "selftest.c", WHITE, INK, 16, font=MONO,
               sub="检查算子，计时")
    out.append(arrow(LX + LW / 2, 64, LX + LW / 2, 90, INK, 1.6))
    out.append(arrow(LX + LW / 2, 148, LX + LW / 2, 174, INK, 1.6))
    out.append(arrow(LX + LW, 204, TX - 4, 204, INK))
    out.append(arrow(LX + LW, 288, TX - 4, 250, INK))
    return out


def table():
    out = [rect(TX, TY, TW, TH, WHITE, INK, rx=6, width=1.8),
           text(TX + TW / 2, TY + 22, "struct ops（ops.h）：算子表", 15, INK, "bold")]
    out.append(line(TX, TY + 32, TX + TW, TY + 32, LINE, 1))
    for i, name in enumerate(OPERATORS):
        y = TY + 52 + i * 17.5
        first = i == 0
        out.append(mono(TX + 18, y, name, 13.5, RED if first else INK,
                        "bold" if first else "normal"))
    out.append(text(TX + 92, TY + 52, "两部分写的算子", 12, RED, "bold", anchor="start"))
    out.append(text(TX + 196, TY + 52, "内存操作", 12, MUTED, anchor="start"))
    for i, name in enumerate(MEMORY):
        out.append(mono(TX + 196, TY + 72 + i * 17.5, name, 13.5, MUTED))
    return out


def implementations():
    out = [text(IX + IW / 2, 20, "编译时 -DOPS= 选一套，一个程序链接一套", 13, MUTED)]
    rows = [(40, "ops/scalar.c", "ops_scalar · CPU，每次 1 个元素", "两部分的参照与计时基准", MUTED),
            (150, "ops/simd.c", "ops_simd · CPU，AVX2，每次 8 个 float", "matvec_q4_0：第一部分", RED),
            (260, "ops/cuda.cu", "ops_cuda · GPU，每个算子一个 kernel", "matvec_q4_0_kernel：第二部分", RED)]
    for y, name, what, note, tone in rows:
        out.append(rect(IX, y, IW, 90, FILL_BLUE if tone is RED else FILL_GREY,
                        BLUE if tone is RED else MUTED, rx=6, width=1.6))
        out.append(mono(IX + IW / 2, y + 26, name, 15, INK, "bold", anchor="middle"))
        out.append(text(IX + IW / 2, y + 48, what, 12, INK))
        out.append(text(IX + IW / 2, y + 72, note, 12.5, tone, "bold"))
        out.append(arrow(TX + TW, TY + TH / 2, IX - 4, y + 45, INK, 1.6))
    return out


def programs():
    out = []
    progs = [(60, 40, "mini-ollama-scalar", None, 85),
             (150, 36, "mini-ollama-simd", None, 195),
             (196, 36, "ops-selftest-simd", None, 195),
             (260, 36, "mini-ollama-cuda", None, 305),
             (306, 36, "ops-selftest-cuda", None, 305)]
    for y, h, name, sub, src_y in progs:
        out.append(rect(PX, y, PW, h, WHITE, INK, rx=5, width=1.4))
        out.append(mono(PX + 12, y + h / 2 + 5, name, 13.5, INK, "bold"))
        if sub:
            out.append(text(PX + PW - 8, y + h / 2 + 5, sub, 11, MUTED, anchor="end"))
        out.append(arrow(IX + IW, src_y, PX - 4, y + h / 2, INK, 1.4))
    return out


def build():
    return framework() + table() + implementations() + programs()


if __name__ == "__main__":
    save("pd-frame-ops", W, H, build())
