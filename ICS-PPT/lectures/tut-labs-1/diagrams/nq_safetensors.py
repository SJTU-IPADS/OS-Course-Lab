#!/usr/bin/env python3
"""Where the four functions of part A are called: the safetensors file.

Top, the file's three parts: the 8-byte little-endian header length N, the
N-byte JSON header, the tensor data. Below, two close-ups: the eight bytes of
N for the course's model file (N = 76240 = 0x129d0, README.md) and the
sixteen bits of one BF16 weight, in lecture 2's field colours (sign blue,
exponent orange, mantissa green), with the fp16 field d of a block beside
them. The bottom row names what reads each part: rd_u64le, the framework's
parser, bf16_to_f32, and the fp16 pair that writes and reads a block's d.
Orange is the student's code, blue the framework's, grey the file. Run it to
refresh ../assets/nq_safetensors.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE,
                    arrow, line, mono, rect, save, text)

W, H = 1120, 300
X0, Y0, RH = 30, 44, 40
SEGS = [("8 字节：头长度 N，小端", 150),
        ("N = 76240 字节：JSON 头，每个张量的 dtype、shape、data_offsets", 440),
        ("张量数据：625 个张量的 BF16 权重，4.25 GB", 470)]
ZY, ZH = 136, 32                               # the close-ups
BYTES = ["d0", "29", "01", "00", "00", "00", "00", "00"]
SIGN = ("#cfe3ec", "#156082")
EXP = ("#f8d5c0", "#e97132")
MAN = ("#cfe8d2", "#196b24")
BOX_Y, BOX_H = 232, 54
BOXES = [(30, 190, FILL_ORANGE, ORANGE, "rd_u64le(p)", "p[0] 最低，读出 N"),
         (240, 250, FILL_BLUE, BLUE, "框架解析 JSON 头", "src/st.cpp"),
         (510, 280, FILL_ORANGE, ORANGE, "bf16_to_f32(h)", "读入每个权重：一次移位"),
         (810, 280, FILL_ORANGE, ORANGE, "f32_to_fp16(f)  fp16_to_f32(h)",
          "写出与读回块中的 fp16 字段 d")]


def guide(x1, y1, x2, y2, color=LINE):
    return line(x1, y1, x2, y2, color, 1.1, dash="4 4")


def build():
    out = []
    x = X0
    edges = []
    for label, w in SEGS:
        out.append(rect(x, Y0, w, RH, FILL_GREY, MUTED, rx=3, width=1.4))
        out.append(text(x + w / 2, Y0 + RH / 2 + 5, label, 12.5, INK, "bold"))
        edges.append((x, x + w))
        x += w
    out.append(text(X0, Y0 - 12, "model.safetensors：4 255 140 312 字节", 13, INK, "bold",
                    anchor="start"))

    # the eight bytes of N
    bx, bw = 40, 32
    out.append(text(bx, ZY - 10, "N 的 8 个字节，p[0] … p[7]", 12, MUTED, anchor="start"))
    for i, b in enumerate(BYTES):
        out.append(rect(bx + i * bw, ZY, bw, ZH, FILL_GREY, MUTED, rx=2, width=1.2))
        out.append(mono(bx + i * bw + bw / 2, ZY + ZH / 2 + 5, b, 13, INK, "bold", "middle"))
    out.append(text(bx, ZY + ZH + 18, "小端：0x0001_29d0 = 76240", 12.5, INK, "bold",
                    anchor="start"))
    out.append(guide(edges[0][0], Y0 + RH, bx, ZY))
    out.append(guide(edges[0][1], Y0 + RH, bx + 8 * bw, ZY))

    # one BF16 weight
    fx, bit = 560, 20
    out.append(text(fx, ZY - 10, "一个 BF16 权重的 16 位 = FP32 的高 16 位", 12, MUTED,
                    anchor="start"))
    for bits, (fill, stroke), label in ((1, SIGN, "1"), (8, EXP, "阶码 8"), (7, MAN, "尾数 7")):
        w = bits * bit
        out.append(rect(fx, ZY, w, ZH, fill, stroke, rx=3, width=1.5))
        out.append(text(fx + w / 2, ZY + ZH / 2 + 5, label, 12.5, INK, "bold" if w > 40 else "normal"))
        fx += w
    out.append(guide(edges[2][0], Y0 + RH, 560, ZY))
    out.append(guide(edges[2][1], Y0 + RH, fx, ZY))

    # the fp16 field of a block, beside the weight
    dx = 930
    out.append(text(dx, ZY - 10, "块中的 d：fp16，2 字节", 12, ORANGE, anchor="start"))
    for i in range(2):
        out.append(rect(dx + i * 40, ZY, 40, ZH, FILL_ORANGE, ORANGE, rx=3, width=1.3))
        out.append(mono(dx + i * 40 + 20, ZY + ZH / 2 + 5, "d", 13, MUTED, "normal", "middle"))
    out.append(text(dx + 40, ZY + ZH + 18, "低字节在前", 12, ORANGE, "bold"))

    for x, w, fill, stroke, name, sub in BOXES:
        out.append(rect(x, BOX_Y, w, BOX_H, fill, stroke, rx=6, width=1.8))
        out.append(mono(x + w / 2, BOX_Y + 23, name, 13.5, INK, "bold", "middle"))
        out.append(text(x + w / 2, BOX_Y + 43, sub, 12, MUTED))
    out.append(arrow(bx + 4 * bw, ZY + ZH + 24, bx + 4 * bw, BOX_Y - 4, ORANGE, 1.8))
    out.append(arrow((edges[1][0] + edges[1][1]) / 2, Y0 + RH + 4, 365, BOX_Y - 4, BLUE, 1.8))
    out.append(arrow(720, ZY + ZH + 4, 650, BOX_Y - 4, ORANGE, 1.8))
    out.append(arrow(970, ZY + ZH + 24, 950, BOX_Y - 4, ORANGE, 1.8, both=True))
    return out


if __name__ == "__main__":
    save("nq_safetensors", W, H, build())
