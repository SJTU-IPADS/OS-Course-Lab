#!/usr/bin/env python3
"""Attention as a table of queries against keys, in three frames.

One row per token (its query q), one column per token (its key k). The weights are
schematic: the last row is the softmax of the four inner products shown in frame 1.
Frame 1: the query of the last token against every key, as inner products.
Frame 2: every row normalised into weights that sum to 1 (the dot grows with the weight);
         a token uses itself and the tokens before it, the cells after it are masked.
Frame 3: the output o of a token is the sum of the values v under its row of weights.
Run it to refresh ../assets/kv-qkv-1..3.svg.
"""

import math

from kv_kit import *
from kv_matrix import WORDS
from svgkit import circle

W, H = 760, 332
N = 4
GX, GY, CW, CH = 250, 84, 76, 44          # the table: left, top, cell width and height
SW, SH = 56, 20                           # a small vector cell (q, k, v, o)
KY, VY = 30, 54                           # tops of the k and v header rows
QX = 178                                  # left edge of the q column
OX = 604                                  # left edge of the o column
SCORES = [1.2, 2.4, 1.7, 0.8]             # q4 . k1..k4
EARLIER = [[1.0], [0.35, 0.65], [0.10, 0.50, 0.40]]


def softmax(values):
    total = sum(math.exp(v) for v in values)
    return [math.exp(v) / total for v in values]


WEIGHTS = EARLIER + [softmax(SCORES)]


def small(x, y, label, kind="mid", width=1.3):
    fill, stroke = KINDS[kind]
    return [rect(x, y, SW, SH, fill, stroke, rx=2, width=width),
            mono(x + SW / 2, y + SH / 2 + 4.3, label, 12, INK, "bold", "middle")]


def row_mid(i):
    return GY + i * CH + CH / 2


def col_left(j):
    return GX + j * CW


def headers(stage):
    out = [text(GX - 10, KY + 15, "键", 14, MUTED, "bold", "end"),
           text(QX + SW / 2, GY + 5, "查询", 14, MUTED, "bold")]
    if stage == 3:
        out.append(text(GX - 10, VY + 15, "值", 14, MUTED, "bold", "end"))
    for j in range(N):
        x = col_left(j) + (CW - SW) / 2
        out.append(text(col_left(j) + CW / 2, 22, WORDS[j], 15, INK, "bold"))
        out += small(x, KY, f"k{j + 1}")
        if stage == 3:
            out += small(x, VY, f"v{j + 1}")
    for i in range(N):
        active = stage > 1 or i == N - 1
        out.append(text(QX - 10, row_mid(i) + 5, WORDS[i], 15, INK if active else MUTED,
                        "bold" if active else "normal", "end"))
        if active:
            out += small(QX, row_mid(i) - SH / 2, f"q{i + 1}")
    return out


def table(stage):
    out = []
    for i in range(N):
        y = GY + i * CH
        for j in range(N):
            x = col_left(j)
            if stage == 1 and i < N - 1:           # not computed in this frame
                out.append(rect(x, y, CW, CH, WHITE, LINE, rx=2, width=1.1))
            elif j > i:                            # a later token: masked
                out.append(rect(x, y, CW, CH, *KINDS["empty"], rx=2, width=1.1, dash="4 3"))
            elif stage == 1:
                out.append(rect(x, y, CW, CH, *KINDS["mid"], rx=2, width=1.3))
                out.append(mono(x + CW / 2, y + CH / 2 + 5.5, f"{SCORES[j]:.1f}", 16, INK,
                                "bold", "middle"))
            else:
                weight = WEIGHTS[i][j]
                out.append(rect(x, y, CW, CH, *KINDS["mid"], rx=2, width=1.3))
                out.append(circle(x + 21, y + CH / 2, 13 * math.sqrt(weight), MUTED, MUTED, 0))
                out.append(mono(x + 55, y + CH / 2 + 4.8, f"{weight:.2f}", 13, INK, "bold",
                                "middle"))
    if stage > 1:
        out.append(text(col_left(1) + 1.5 * CW, GY + CH / 2 + 5, "之后的 token 不参与", 13,
                        MUTED, "bold"))
    return out


def outputs():
    """Frame 3: one output per row, the last row framed and written out as a sum."""
    out = [text(OX + SW / 2, GY + 5, "输出", 14, MUTED, "bold")]
    for i in range(N):
        y = row_mid(i)
        last = i == N - 1
        out.append(arrow(GX + N * CW + 5, y, OX - 6, y, INK if last else MUTED, 1.6))
        out += small(OX, y - SH / 2, f"o{i + 1}", width=2.2 if last else 1.3)
    out.append(rect(GX - 3, GY + (N - 1) * CH - 3, N * CW + 6, CH + 6, "none", INK, rx=4,
                    width=2.2))
    terms = " + ".join(f"{w:.2f} v{j + 1}" for j, w in enumerate(WEIGHTS[-1]))
    out.append(mono(GX + N * CW / 2, GY + N * CH + 26, f"o{N} = {terms}", 15, INK, "bold",
                    "middle"))
    return out


CAPTIONS = [
    f"① 「{WORDS[N - 1]}」的查询 q{N} 与每个 token 的键作内积，内积越大，键与查询越匹配",
    "② 内积按行归一化为权重，每行的和为 1，圆点的大小表示权重",
    f"③ 输出是各 token 的值按权重相加，o{N} 含有前面各 token 的信息",
]


def figure(stage):
    out = headers(stage) + table(stage)
    if stage == 1:
        out.append(text(GX + N * CW + 12, row_mid(N - 1) + 5, f"q{N} 与各个键的内积", 14, MUTED,
                        "bold", "start"))
    if stage == 3:
        out += outputs()
    out.append(text(W / 2, H - 10, CAPTIONS[stage - 1], 17, INK, "bold"))
    return out


if __name__ == "__main__":
    for stage in (1, 2, 3):
        save(f"kv-qkv-{stage}", W, H, figure(stage))
