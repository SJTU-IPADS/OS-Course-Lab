#!/usr/bin/env python3
"""Why one forward pass can check four guessed words.

The words are the lecture's own sentence, shared with spec_decode.py. Every
output position holds the word after its input, as on llm-read-once: 「是」
in, 「吞」 out. Frames stay on one canvas; the weights box never moves.

1. Plain decoding: 「是」 goes in, one word comes out. Nothing further is
   written, so there is nothing more to put in.
2. The small model writes four words down.
3. 「是」 and the four guesses are five vectors, sent in together.
4. The same pass yields five outputs: the first four answer the four
   guesses, the fifth is the spare word for when all four are right.

Run it to refresh assets/spec-why-1.svg … spec-why-4.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from spec_decode import GUESS, PREFIX, TARGET
from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE,
                    arrow, box, rect, save, text)

W = 1120
BW, BH, GAP = 76, 34, 10
PITCH = BW + GAP
PREFIX_X = 200
GUESS_X = PREFIX_X + 4 * PITCH + 22
Y_WORD = 32

# One forward pass. The box never moves; the rows beside it change.
IN_W, IN_H = 96, 38
OUT_W = 88
ARROW = 28
PITCH_V = 44
WB_W, WB_H = 220, 230
BOX_Y = 100
IN_X = (W - (IN_W + ARROW + WB_W + ARROW + OUT_W)) / 2
BOX_X = IN_X + IN_W + ARROW
OUT_X = BOX_X + WB_W + ARROW
H = BOX_Y + WB_H + 36

INPUTS = [PREFIX[-1]] + GUESS                  # 是 吞 吐 量 ，
OUTPUTS = TARGET + ["…"]                       # 吞 吐 率 。 and the spare word
IN_LABELS = ["前文最后一个词"] + [f"猜 {i + 1}" for i in range(len(GUESS))]
OUT_LABELS = [f"核对猜 {i + 1}" for i in range(len(GUESS))] + ["4 个全对时多得一个词"]


def gx(i):
    return GUESS_X + i * PITCH


def px(i):
    return PREFIX_X + i * PITCH


def token(x, y, w, h, word, fill, stroke, dash=None, size=16, weight="bold", color=INK,
          sub=None):
    out = [rect(x, y, w, h, fill, stroke, rx=6, width=1.6, dash=dash)]
    if sub is None:
        if word:
            out.append(text(x + w / 2, y + h / 2 + size * 0.36, word, size, color, weight))
    else:
        out.append(text(x + w / 2, y + h / 2 - 2, word, size, color, weight))
        out.append(text(x + w / 2, y + h / 2 + 14, sub, 12, MUTED))
    return out


def headers():
    span = 4 * PITCH - GAP
    return [text(PREFIX_X + span / 2, 18, "前文", 14, BLUE, "bold"),
            text(GUESS_X + span / 2, 18, "小模型写下", 14, ORANGE, "bold")]


def words(guesses_written):
    out = [text(24, Y_WORD + BH / 2 + 6, "已写下", 16, INK, "bold", anchor="start")]
    for i, w in enumerate(PREFIX):
        out += token(px(i), Y_WORD, BW, BH, w, FILL_BLUE, BLUE, size=15, weight="normal")
    for i, w in enumerate(GUESS):
        if guesses_written:
            out += token(gx(i), Y_WORD, BW, BH, w, WHITE, ORANGE, dash="5 4")
        else:
            out += token(gx(i), Y_WORD, BW, BH, "?", WHITE, MUTED, dash="5 4",
                         size=16, weight="normal", color=MUTED)
    return out


def weights():
    return box(BOX_X, BOX_Y, WB_W, WB_H, "一次前向", FILL_GREY, INK, 18, "bold",
               sub="126 层权重，读一遍", sub_size=13)


def vector(y, word, confirmed):
    """One input vector at height y: a confirmed word is blue, a guess dashed orange."""
    top = y - IN_H / 2
    if confirmed:
        return token(IN_X, top, IN_W, IN_H, word, FILL_BLUE, BLUE, size=15, sub="向量")
    return token(IN_X, top, IN_W, IN_H, word, WHITE, ORANGE, dash="5 4", size=15, sub="向量")


def in_arrow(y):
    return arrow(IN_X + IN_W + 2, y, BOX_X - 2, y, INK, 1.8)


def out_arrow(y):
    return arrow(BOX_X + WB_W + 2, y, OUT_X - 2, y, INK, 1.8)


def plain(caption):
    """Frames 1 and 2: 「是」 in, the word after it out."""
    y = BOX_Y + WB_H / 2
    out = weights() + vector(y, PREFIX[-1], True) + [in_arrow(y), out_arrow(y)]
    out += token(OUT_X, y - IN_H / 2, OUT_W, IN_H, "下一个词", FILL_ORANGE, ORANGE, size=14)
    out.append(text(OUT_X + OUT_W + 10, y + 5, "「是」后面的词", 13, MUTED, anchor="start"))
    out.append(text(W / 2, H - 10, caption, 16, INK, "bold"))
    return out


def together(show_answers):
    """Frames 3 and 4: five vectors in, five outputs, each the word after its input."""
    n = len(INPUTS)
    first = BOX_Y + WB_H / 2 - (n - 1) * PITCH_V / 2
    ys = [first + i * PITCH_V for i in range(n)]
    out = weights()
    out.append(text(IN_X + IN_W / 2, ys[0] - IN_H / 2 - 14, f"{n} 个向量，同时输入",
                    14, ORANGE, "bold"))
    for i, (w, y) in enumerate(zip(INPUTS, ys)):
        out += vector(y, w, i == 0)
        out.append(text(IN_X - 10, y + 5, IN_LABELS[i], 13, MUTED, anchor="end"))
        out.append(in_arrow(y))
        if show_answers:
            word = OUTPUTS[i]
            out.append(out_arrow(y))
            out += token(OUT_X, y - IN_H / 2, OUT_W, IN_H, word, FILL_ORANGE, ORANGE,
                         size=16, color=MUTED if word == "…" else INK)
            out.append(text(OUT_X + OUT_W + 10, y + 5, OUT_LABELS[i], 13, MUTED, anchor="start"))
        else:
            out.append(rect(OUT_X, y - IN_H / 2, OUT_W, IN_H, WHITE, LINE, rx=6,
                            width=1.4, dash="5 4"))
    if show_answers:
        out.append(text(OUT_X + OUT_W / 2, ys[0] - IN_H / 2 - 14, "同时得到", 14, ORANGE, "bold"))
    return out


def build(step):
    out = headers() + words(step >= 2)
    if step == 1:
        out += plain("逐词生成：一次前向得到一个词。后面的词还没写下，没有别的输入")
    elif step == 2:
        out += plain("小模型写下 4 个词，它们可以和「是」一起输入")
    else:
        out += together(step == 4)
    return out


if __name__ == "__main__":
    for step in range(1, 5):
        save(f"spec-why-{step}", W, H, build(step))
