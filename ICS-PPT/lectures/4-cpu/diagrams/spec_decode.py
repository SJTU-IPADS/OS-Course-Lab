#!/usr/bin/env python3
"""Speculative decoding on the lecture's own sentence.

The text so far is 流水线 提高 的 是. A small model guesses four words in a
row; the large model reads its weights once and, for each of the four
positions, produces the word it would have taken itself; the two rows are
compared left to right. spec-guess.svg stops after the guess row;
spec-decode.svg is the whole figure. Run it to refresh both.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_RED, GREEN,
                    INK, MUTED, ORANGE, RED, arrow, box, rect, save, text)

W = 1120
LX, BX = 20, 330
BW, BH, GAP = 104, 40, 14
K = 4
PREFIX = ["流水线", "提高", "的", "是"]
GUESS = ["吞", "吐", "量", "，"]
TARGET = ["吞", "吐", "率", "。"]
VERDICT = [("接受", GREEN, FILL_GREEN), ("接受", GREEN, FILL_GREEN),
           ("改为「率」", ORANGE, FILL_ORANGE), ("作废", RED, FILL_RED)]
YP, YA, YB, YC, YR = 16, 96, 176, 256, 326


def col_x(i):
    return BX + i * (BW + GAP)


def label(y, s, color=INK):
    return text(LX, y + BH / 2 + 6, s, 16, color, "bold", anchor="start")


def build(full):
    out = [label(YP, "前文")]
    for i, w in enumerate(PREFIX):
        out += box(col_x(i), YP, BW, BH, w, FILL_BLUE, BLUE, 15, "normal")
    out.append(text(col_x(K) + 20, YP + BH / 2 + 6, "下一个词是什么？", 15, MUTED, anchor="start"))

    out.append(label(YA, f"小模型连猜 {K} 个", ORANGE))
    for i, w in enumerate(GUESS):
        out.append(text(col_x(i) + BW / 2, YA - 8, f"猜 {i + 1}", 13, MUTED))
        out += box(col_x(i), YA, BW, BH, w, FILL_ORANGE, ORANGE, 16, "bold")
    out.append(text(col_x(K) + 20, YA + BH / 2 + 6, "权重少几十倍，猜得快", 14, MUTED, anchor="start"))
    if not full:
        return out

    out.append(label(YB, "大模型一遍算出", INK))
    for i, w in enumerate(TARGET):
        out += box(col_x(i), YB, BW, BH, w, FILL_GREY, INK, 16, "bold")
    out.append(text(col_x(K) + 20, YB + BH / 2 + 6, "连同前文一起送入，读一遍权重", 14, MUTED, anchor="start"))
    for i in range(K):
        out.append(arrow(col_x(i) + BW / 2, YA + BH + 2, col_x(i) + BW / 2, YB - 2, MUTED, 1.4))

    out.append(label(YC, "从前往后比对"))
    for i, (v, stroke, fill) in enumerate(VERDICT):
        dash = "5 4" if v == "作废" else None
        out.append(rect(col_x(i), YC, BW, BH, fill, stroke, rx=6, width=1.6, dash=dash))
        out.append(text(col_x(i) + BW / 2, YC + BH / 2 + 6, v, 15, stroke, "bold"))
    out.append(text(col_x(K) + 20, YC + BH / 2 + 6, "猜错处换成大模型的词，后面的作废", 14, MUTED, anchor="start"))

    out.append(label(YR, "这一轮得到", GREEN))
    for i, w in enumerate(["吞", "吐", "率"]):
        out += box(col_x(i), YR, BW, BH, w, FILL_GREEN, GREEN, 16, "bold")
    out.append(text(col_x(3) + 10, YR + BH / 2 + 6, "读一遍权重得到 3 个词；逐词生成要读 3 遍",
                    15, GREEN, "bold", anchor="start"))
    return out


if __name__ == "__main__":
    save("spec-guess", W, YA + BH + 20, build(False))
    save("spec-decode", W, YR + BH + 16, build(True))
