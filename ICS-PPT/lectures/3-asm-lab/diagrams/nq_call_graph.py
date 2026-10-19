#!/usr/bin/env python3
"""Who calls the functions of impl/nano_quant.cpp, from where, with what.

Left, the framework (blue): `nano-quant quant` as src/main_quant.cpp runs it,
its loops drawn as nested boxes: st_open once per file, then for every tensor
of the language model (310 of the 625; the vision part is skipped), for every chunk of whole rows, st_read_f32, nq_quantize over the blocks of the
chunk, nq_write_chunk. Right, the student's functions (orange) with their
prototypes from include/nano_quant.h; an arrow from each call site carries
the argument the framework passes. q4_0_dequantize has no call site in
nano-quant and no arrow.
The offsets 32·b and 18·b are Q4_0's block length and block bytes
(src/quant.cpp). Run it to refresh ../assets/nq_call_graph.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_ORANGE, FONT, INK, MONO, MUTED, ORANGE, WHITE, arrow,
                    label_line, mono, rect, save, text)

W, H = 1120, 440
PX, PY, PW, PH = 16, 16, 556, 408         # the framework panel
AX = 560                                  # arrows leave the panel here
RX, RW, RH = 710, 400, 50                 # the student's boxes


def loop(x, y, w, h, label):
    return [rect(x, y, w, h, WHITE, BLUE, rx=6, width=1.4, dash="6 4"),
            text(x + 12, y + 20, label, 14, BLUE, "bold", anchor="start")]


def call(x, y, name, what):
    return label_line(x, y, [(name, MONO, INK, "bold"), (what, FONT, INK, "normal")], 14)


def student(y, sig, sub=None):
    out = [rect(RX, y, RW, RH, FILL_ORANGE, ORANGE, rx=6, width=1.8)]
    if sub:
        out.append(mono(RX + 14, y + 22, sig, 12.5, INK, "bold"))
        out.append(text(RX + 14, y + 41, sub, 12, MUTED, anchor="start"))
    else:
        out.append(mono(RX + 14, y + RH / 2 + 5, sig, 12.5, INK, "bold"))
    return out


def call_arrow(y, label, label2=None):
    out = [arrow(AX, y, RX - 4, y, ORANGE, 2),
           text((AX + RX) / 2, y - 8, label, 12.5, INK)]
    if label2:
        out.append(text((AX + RX) / 2, y + 18, label2, 12, MUTED))
    return out


def build():
    out = [rect(PX, PY, PW, PH, WHITE, BLUE, rx=8, width=1.8),
           text(PX + 14, PY + 24, "nano-quant quant（src/main_quant.cpp）", 15, BLUE,
                "bold", anchor="start")]
    out += call(PX + 24, 74, "st_open", "：打开文件，读 8 字节的头长度，解析 JSON 头")
    out += loop(PX + 14, 96, PW - 28, 308, "对每个张量：文件中 625 个张量里属于语言模型的 310 个，视觉部分跳过")
    out += loop(PX + 30, 128, PW - 60, 264, "对每段整行（每次至多 4 Mi 个权重）")
    out += call(PX + 46, 178, "st_read_f32", "：读入一段 BF16 权重，逐个转为 float")
    out += call(PX + 46, 218, "nq_quantize(type, src, n, dst)", "：按格式分块")
    out += loop(PX + 46, 236, PW - 92, 76, "对每个块 b = 0 .. n/32 − 1")
    out.append(mono(PX + 62, 292, "x = src + 32·b,   blk = dst + 18·b", 14, INK, "bold"))
    out += call(PX + 46, 350, "nq_write_chunk", "：把 dst 的字节写入 .nq 的数据区")

    out.append(text(RX, 30, "impl/nano_quant.cpp（你写的函数）", 15, ORANGE, "bold",
                    anchor="start"))
    out += student(45, "uint64_t rd_u64le(const uint8_t *p)")
    out += call_arrow(70, "p：头的前 8 个字节")
    out += student(149, "float bf16_to_f32(uint16_t h)")
    out += call_arrow(174, "h：一个权重的 16 位")
    out += student(263, "void q4_0_quantize(const float *x, uint8_t *blk)")
    out += call_arrow(288, "x, blk", "32 个 float → 18 字节")
    out += student(360, "void q4_0_dequantize(const uint8_t *blk, float *x)")
    return out


if __name__ == "__main__":
    save("nq_call_graph", W, H, build())
