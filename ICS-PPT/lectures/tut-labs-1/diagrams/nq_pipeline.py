#!/usr/bin/env python3
"""The four programs of nano-quant and the files that pass between them.

The main line, left to right: the safetensors file, `nano-quant quant`, the
.nq file, `nq2gguf`, the GGUF, ollama. Under it, `tools/mkmeta.py` turns
config.json and tokenizer.json into meta.kv, the key-value section nq2gguf
merges in. Files are grey, the framework's programs blue, ollama green; the
orange strip inside nano-quant is where the functions of impl/nano_quant.cpp
run. Sizes are those of the real model under the q4_k_m recipe
(README.md, tests/expected-qwen3vl.txt). Run it to refresh
../assets/nq_pipeline.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK,
                    MUTED, ORANGE, arrow, elbow, mono, rect, save, text)

W, H = 1120, 292
BH = 64                                   # box height
YA, YB = 60, 190                          # the tensor row, the metadata row
COLS = [(10, 170), (222, 150), (414, 150), (606, 130), (778, 150), (970, 140)]


def file_box(col, y, name, sub=None, name2=None):
    x, w = COLS[col]
    out = [rect(x, y, w, BH, FILL_GREY, MUTED, rx=6, width=1.6)]
    if name2:
        out.append(mono(x + w / 2, y + 27, name, 14, INK, "bold", "middle"))
        out.append(mono(x + w / 2, y + 48, name2, 14, INK, "bold", "middle"))
    elif sub:
        out.append(mono(x + w / 2, y + 27, name, 14, INK, "bold", "middle"))
        out.append(text(x + w / 2, y + 48, sub, 12, MUTED))
    else:
        out.append(mono(x + w / 2, y + BH / 2 + 5, name, 14, INK, "bold", "middle"))
    return out


def program_box(col, y, name, fill=FILL_BLUE, stroke=BLUE, sub=None, strip=None):
    x, w = COLS[col]
    out = [rect(x, y, w, BH, fill, stroke, rx=6, width=1.8)]
    ty = y + 25 if (sub or strip) else y + BH / 2 + 5
    out.append(mono(x + w / 2, ty, name, 14, INK, "bold", "middle"))
    if sub:
        out.append(text(x + w / 2, y + 48, sub, 12, MUTED))
    if strip:
        out.append(rect(x + 6, y + 36, w - 12, 22, FILL_ORANGE, ORANGE, rx=3, width=1.4))
        out.append(mono(x + w / 2, y + 51, strip, 11.5, ORANGE, "bold", "middle"))
    return out


def link(col_a, col_b, y):
    xa = COLS[col_a][0] + COLS[col_a][1] + 4
    xb = COLS[col_b][0] - 4
    return arrow(xa, y, xb, y, INK, 2)


def legend(x, y, fill, stroke, label):
    return [rect(x, y - 11, 14, 14, fill, stroke, rx=2, width=1.4),
            text(x + 20, y, label, 12.5, MUTED, anchor="start")]


def build():
    ym = YA + BH / 2
    out = [text(COLS[0][0], YA - 16, "张量的数据：读 BF16 权重，按块量化，写出数据区", 13.5,
                INK, "bold", anchor="start")]
    out += file_box(0, YA, "model.safetensors", "4.25 GB · 625 个 BF16 张量")
    out += program_box(1, YA, "nano-quant quant", strip="impl/nano_quant.cpp")
    out += file_box(2, YA, "q4_k_m.nq", "数据区 1.10 GB")
    out += program_box(3, YA, "nq2gguf")
    out += file_box(4, YA, "q4_k_m.gguf", "1.11 GB")
    out += program_box(5, YA, "ollama", FILL_GREEN, GREEN, sub="create · run")
    for a in range(5):
        out.append(link(a, a + 1, ym))

    yb = YB + BH / 2
    out.append(text(COLS[0][0], YB - 16, "模型的元数据：架构参数、词表、特殊词元，成为 GGUF 的键值段",
                    13.5, INK, "bold", anchor="start"))
    out += file_box(0, YB, "config.json", name2="tokenizer.json")
    out += program_box(1, YB, "tools/mkmeta.py")
    out += file_box(2, YB, "meta.kv", "24 个键值对")
    out.append(link(0, 1, yb))
    out.append(link(1, 2, yb))
    x2 = COLS[2][0] + COLS[2][1] + 4
    x3 = COLS[3][0] + COLS[3][1] / 2
    out.append(elbow([(x2, yb), (x3, yb), (x3, YA + BH + 5)], INK, 2))

    y = H - 10
    out += legend(COLS[3][0], y, FILL_GREY, MUTED, "文件")
    out += legend(COLS[3][0] + 70, y, FILL_BLUE, BLUE, "框架的程序")
    out += legend(COLS[3][0] + 190, y, FILL_ORANGE, ORANGE, "你写的代码")
    out += legend(COLS[3][0] + 310, y, FILL_GREEN, GREEN, "ollama")
    return out


if __name__ == "__main__":
    save("nq_pipeline", W, H, build())
