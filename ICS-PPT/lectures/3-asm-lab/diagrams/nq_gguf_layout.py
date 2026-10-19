#!/usr/bin/env python3
"""The GGUF that nq2gguf writes for the real model, drawn to scale.

Four strips, each a close-up of a part of the one above or below it, joined
by dashed guides. Row B is the whole file: the header is a sliver (5.9 MB of
974 MB) and the data area the rest. Row A blows the header up: magic,
version, counts, the key-value pairs mkmeta.py made, the tensor directory,
the padding. Row C blows the data area up: 310 tensors end to end in the
order of the safetensors header, token_embd first, then 28 layers, then
output_norm. Row D blows one layer up: its 11 tensors, the Q4_0 ones in
proportion and the four F32 norms as labelled hairlines, since 8 KB of
28 MB is less than a pixel.

Blue is what the framework writes, orange what the student's functions
write. The numbers are `nano-quant plan model/model.safetensors --recipe
q4_0` and tests/expected-qwen3vl.txt. Run it to refresh
../assets/nq_gguf_layout.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, LINE, MUTED, ORANGE, line, mono,
                    rect, save, text)

W, H = 1120, 432
X0, XW, RH = 30, 1060, 44
YA, YB, YC, YD = 40, 150, 250, 350

HEAD, DATA = 5946976, 968249344
FILE = HEAD + DATA
TOKEN, LAYER, N_LAYERS = 175030272, 28328960, 28
HEADER = [("GGUF", 70, "魔数"), ("3", 50, "版本"), ("310", 70, "张量数"),
          ("26", 80, "键值对数"),
          ("26 个键值对", 330, "mkmeta.py 的 24 个 + nq2gguf 的 2 个"),
          ("310 个张量目录项", 380, "名字、维数、各维长度、格式、数据区内偏移"),
          ("填充", 80, "至 32 的倍数")]
BLK0 = [("attn_norm", 8192), ("ffn_down", 7077888), ("ffn_gate", 7077888),
        ("ffn_up", 7077888), ("ffn_norm", 8192), ("attn_k_norm", 512),
        ("attn_k", 1179648), ("attn_output", 2359296), ("attn_q_norm", 512),
        ("attn_q", 2359296), ("attn_v", 1179648)]


def guides(ax0, ax1, ay, bx0, bx1, by, color):
    return [line(ax0, ay, bx0, by, color, 1.2, dash="4 4"),
            line(ax1, ay, bx1, by, color, 1.2, dash="4 4")]


def header_row():
    out = [text(X0, YA - 12, "头：nq2gguf 按 meta.kv 与 .nq 的目录写出", 13, BLUE, "bold",
                anchor="start")]
    x = X0
    for label, w, sub in HEADER:
        out.append(rect(x, YA, w, RH, FILL_BLUE, BLUE, rx=3, width=1.4))
        out.append(text(x + w / 2, YA + RH / 2 + 5, label, 12.5, INK, "bold"))
        out.append(text(x + w / 2, YA + RH + 16, sub, 11, MUTED))
        x += w
    return out


def file_row():
    sliver = max(8, XW * HEAD / FILE)
    out = [rect(X0, YB, sliver, RH, FILL_BLUE, BLUE, rx=0, width=1.4),
           rect(X0 + sliver, YB, XW - sliver, RH, FILL_ORANGE, ORANGE, rx=0, width=1.4),
           text(X0, YB - 12, "头 5 946 976 字节", 13, BLUE, "bold", anchor="start"),
           text(X0 + XW, YB - 12, "整个文件 974 196 320 字节 = 头 + 数据区", 13, INK, "bold",
                anchor="end"),
           text(X0 + sliver + (XW - sliver) / 2, YB + RH / 2 + 5,
                "张量数据区 968 249 344 字节", 13.5, INK, "bold")]
    return out, sliver


def data_row():
    out = [text(X0, YC - 12, "数据区：310 个张量首尾相接，顺序同 safetensors 的头；"
                "blk.0 … blk.27 每层 11 个张量，28 328 960 字节", 13, ORANGE, "bold",
                anchor="start")]
    tw = XW * TOKEN / DATA
    lw = XW * LAYER / DATA
    out.append(rect(X0, YC, tw, RH, FILL_ORANGE, ORANGE, rx=0, width=1.2))
    out.append(mono(X0 + tw / 2, YC + RH / 2 + 4, "token_embd.weight", 11.5, INK, "bold",
                    "middle"))
    x = X0 + tw
    for i in range(N_LAYERS):
        out.append(rect(x, YC, lw, RH, FILL_ORANGE, ORANGE, rx=0, width=1.0))
        label = {0: "blk.0", 1: "blk.1", N_LAYERS - 1: "blk.27"}.get(i, "·")
        out.append(mono(x + lw / 2, YC + RH / 2 + 4, label, 9.5, INK, "bold", "middle"))
        x += lw
    out.append(rect(x, YC, 2, RH, MUTED, MUTED, rx=0, width=0))
    out.append(text(X0 + XW, YC + RH + 14, "output_norm.weight，8 192 字节", 10.5, MUTED,
                    anchor="end"))
    return out, X0 + tw, X0 + tw + lw


def layer_row():
    out = [text(X0, YD - 12, "blk.0 的 11 个张量：7 个矩阵是 Q4_0 的块，由你的函数写出，"
                "灰线标出的 4 个一维的 F32 归一化系数原样保留", 13, INK, "bold",
                anchor="start")]
    x = X0
    hair = []                                   # (x, name) of the F32 norms
    for name, nbytes in BLK0:
        w = XW * nbytes / LAYER
        if w < 3:
            out.append(rect(x, YD, 2, RH, MUTED, MUTED, rx=0, width=0))
            hair.append((x + 1, name))
            x += 2
            continue
        out.append(rect(x, YD, w, RH, FILL_ORANGE, ORANGE, rx=0, width=1.2))
        out.append(mono(x + w / 2, YD + RH / 2 + 4, name, 11, INK, "bold", "middle"))
        x += w
    # label the hairlines below the row; of two adjacent ones the first is
    # labelled to the left of its tick, the second to the right
    for i, (hx, name) in enumerate(hair):
        out.append(line(hx, YD + RH, hx, YD + RH + 8, MUTED, 1.2))
        before = i > 0 and hx - hair[i - 1][0] < 8
        after = i + 1 < len(hair) and hair[i + 1][0] - hx < 8
        if hx < X0 + 40:
            lx, anchor = hx, "start"
        elif after:
            lx, anchor = hx - 3, "end"
        elif before:
            lx, anchor = hx + 3, "start"
        else:
            lx, anchor = hx, "middle"
        out.append(mono(lx, YD + RH + 22, name, 11, MUTED, "bold", anchor))
    return out


def build():
    out = header_row()
    strip, sliver = file_row()
    out += strip
    out += guides(X0, X0 + XW, YA + RH + 24, X0, X0 + sliver, YB, BLUE)
    data, l0, l1 = data_row()
    out += data
    out += guides(X0 + sliver, X0 + XW, YB + RH, X0, X0 + XW, YC, ORANGE)
    out += layer_row()
    out += guides(l0, l1, YC + RH, X0, X0 + XW, YD, ORANGE)
    return out


if __name__ == "__main__":
    save("nq_gguf_layout", W, H, build())
