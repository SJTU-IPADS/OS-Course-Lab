#!/usr/bin/env python3
"""Speculative decoding on the lecture's own sentence.

The text so far is 流水线 提高 的 是. A small model guesses four words in a
row; the large model reuses its weights in one forward pass and, for each of the four
positions, produces the word it would have taken itself; the two rows are
compared left to right. spec-guess.svg stops after the guess row.
spec-verify-1.svg … spec-verify-4.svg walk the comparison; the canvas is the
whole figure. Colours follow the other part-4
figures: the text so far blue, a word the large model takes orange (its row
and the words this round yields); the small model's guesses are dashed
orange, words not yet confirmed. Run it to refresh both.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, FILL_RED, GREEN, INK, LINE, MUTED,
                    ORANGE, RED, WHITE, arrow, box, rect, save, text)

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
        out.append(rect(col_x(i), YA, BW, BH, WHITE, ORANGE, rx=6, width=1.6, dash="5 4"))
        out.append(text(col_x(i) + BW / 2, YA + BH / 2 + 6, w, 16, INK, "bold"))
    out.append(text(col_x(K) + 20, YA + BH / 2 + 6, "权重少几十倍，猜得快", 14, MUTED, anchor="start"))
    if not full:
        return out

    out.append(label(YB, "大模型一次核对", INK))
    for i, w in enumerate(TARGET):
        out += box(col_x(i), YB, BW, BH, w, FILL_ORANGE, ORANGE, 16, "bold")
    out.append(text(col_x(K) + 20, YB + BH / 2 + 6, "连同前文一起送入，一次前向核对", 14, MUTED, anchor="start"))
    for i in range(K):
        out.append(arrow(col_x(i) + BW / 2, YA + BH + 2, col_x(i) + BW / 2, YB - 2, MUTED, 1.4))

    out.append(label(YC, "从前往后比对"))
    for i, (v, stroke, fill) in enumerate(VERDICT):
        dash = "5 4" if v == "作废" else None
        out.append(rect(col_x(i), YC, BW, BH, fill, stroke, rx=6, width=1.6, dash=dash))
        out.append(text(col_x(i) + BW / 2, YC + BH / 2 + 6, v, 15, stroke, "bold"))
    out.append(text(col_x(K) + 20, YC + BH / 2 + 6, "猜错处换成大模型的词，后面的作废", 14, MUTED, anchor="start"))

    out.append(label(YR, "这一轮得到"))
    for i, w in enumerate(["吞", "吐", "率"]):
        out += box(col_x(i), YR, BW, BH, w, FILL_ORANGE, ORANGE, 16, "bold")
    out.append(text(col_x(3) + 10, YR + BH / 2 + 6, "一次核对得到 3 个词；逐词生成需 3 次前向",
                    15, GREEN, "bold", anchor="start"))
    return out


def _hl(i0, i1, color):
    """A dashed frame around guess columns i0..i1, from the guess row through the verdict."""
    x = col_x(i0) - 5
    w = col_x(i1) + BW + 5 - x
    return rect(x, YA + 1, w, YC + BH + 5 - (YA + 1), "none", color, rx=8, width=2, dash="6 4")


def verify(step):
    """Left-to-right comparison. The canvas matches the full figure so frames do not jump.

    1 both rows are already filled, verdicts blank. 2 the first word is kept.
    3 the second is kept. 4 the third is replaced, the fourth voided, and this
    round's three words appear.
    """
    out = []
    if step == 2:
        out.append(_hl(0, 0, GREEN))
    elif step == 3:
        out.append(_hl(1, 1, GREEN))
    elif step == 4:
        out.append(_hl(2, 3, ORANGE))

    out.append(label(YP, "前文"))
    for i, w in enumerate(PREFIX):
        out += box(col_x(i), YP, BW, BH, w, FILL_BLUE, BLUE, 15, "normal")

    out.append(label(YA, f"小模型连猜 {K} 个", ORANGE))
    for i, w in enumerate(GUESS):
        out.append(text(col_x(i) + BW / 2, YA - 8, f"猜 {i + 1}", 13, MUTED))
        out.append(rect(col_x(i), YA, BW, BH, WHITE, ORANGE, rx=6, width=1.6, dash="5 4"))
        out.append(text(col_x(i) + BW / 2, YA + BH / 2 + 6, w, 16, INK, "bold"))

    out.append(label(YB, "大模型一次核对", INK))
    for i, w in enumerate(TARGET):
        out += box(col_x(i), YB, BW, BH, w, FILL_ORANGE, ORANGE, 16, "bold")
    out.append(text(col_x(K) + 20, YB + BH / 2 + 6, "上一页同时得到的前 4 个输出", 14, MUTED,
                    anchor="start"))

    captions = {1: "从左边开始比", 2: "「吞」相同，保留", 3: "「吐」也相同",
                4: "「量」改为「率」，后面作废"}
    shown = K if step == 4 else step - 1
    out.append(label(YC, "从前往后比对"))
    for i, (v, stroke, fill) in enumerate(VERDICT):
        if i < shown:
            dash = "5 4" if v == "作废" else None
            out.append(rect(col_x(i), YC, BW, BH, fill, stroke, rx=6, width=1.6, dash=dash))
            out.append(text(col_x(i) + BW / 2, YC + BH / 2 + 6, v, 15, stroke, "bold"))
        else:
            out.append(rect(col_x(i), YC, BW, BH, WHITE, LINE, rx=6, width=1.4, dash="5 4"))
    out.append(text(col_x(K) + 20, YC + BH / 2 + 6, captions[step], 14, INK, "bold", anchor="start"))

    out.append(label(YR, "这一轮得到"))
    if step == 4:
        for i, w in enumerate(["吞", "吐", "率"]):
            out += box(col_x(i), YR, BW, BH, w, FILL_ORANGE, ORANGE, 16, "bold")
        out.append(text(col_x(3) + 10, YR + BH / 2 + 6, "一次核对得到 3 个词",
                        15, GREEN, "bold", anchor="start"))
    else:
        for i in range(3):
            out.append(rect(col_x(i), YR, BW, BH, WHITE, LINE, rx=6, width=1.4, dash="5 4"))
    return out


if __name__ == "__main__":
    save("spec-guess", W, YA + BH + 20, build(False))
    for step in range(1, 5):
        save(f"spec-verify-{step}", W, YR + BH + 16, verify(step))
