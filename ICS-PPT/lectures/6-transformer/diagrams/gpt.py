#!/usr/bin/env python3
"""gpt: the three words behind the initials GPT, each with what it means.

One frame. Generative, Pretrained, Transformer side by side, the initial of
each word in orange, a one-line gloss under each word.
"""
from tk import *

H = 260
WORD = 42
# (word, gloss, column centre, column width)
COLS = [
    ("Generative", "生成新的文本", 160, 290),
    ("Pretrained", "先在大量数据上学习，可再针对具体任务微调", 520, 400),
    ("Transformer", "一种神经网络结构", 880, 290),
]


def initial_word(x, y, word, size):
    """The word centred at x, its first letter orange and bold."""
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{FONT}" font-size="{size}" '
            f'fill="{INK}" text-anchor="middle">'
            f'<tspan fill="{ORANGE}" font-weight="bold">{word[0]}</tspan>{esc(word[1:])}</text>')


b = []
top, h = 40, 180
for word, gloss, cx, w in COLS:
    b.append(rect(cx - w / 2, top, w, h, WHITE, LINE, rx=8, width=1.4))
    b.append(initial_word(cx, top + 76, word, WORD))
    b.append(line(cx - 60, top + 104, cx + 60, top + 104, LINE, 1.4))
    b.append(label(cx, top + 142, gloss, 19, INK))
save("gpt", b, H)
