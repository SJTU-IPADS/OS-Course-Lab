#!/usr/bin/env python3
"""training-targets: one passage, a prediction target at every position.

1: from the prefix "a fluffy blue creature" the model predicts the next
   word, roamed (green); the words after it are not used.
2: every position at once: under each word, the word that follows it (green,
   shifted by one); the word after forest is not in the passage and is drawn
   as "…".
"""
from tk import *
from attn_data import WORDS

H = 250
CS, GAP = 20, 24
CHIP_Y = 52
X0 = (W - row_width(WORDS, CS, GAP)) / 2


def faded_chip(x, y, word, size=CS, h=36, pad=10):
    w = width(word, size) + 2 * pad
    return [rect(x, y, w, h, WHITE, LINE, rx=4, width=1.4, dash="4 3"),
            text(x + w / 2, y + h / 2 + size * 0.36, word, size, MUTED)]


def frame(n):
    b = []
    spans, x = [], X0
    for i, word in enumerate(WORDS):
        w = width(word, CS) + 20
        if n == 1 and i >= 5:
            b += faded_chip(x, CHIP_Y, word)
        else:
            role = "upd" if (n == 1 and i == 4) else "data"
            b += chip(x, CHIP_Y, word, role, CS)[0]
        spans.append((x, x + w / 2, x + w))
        x += w + GAP
    bot = CHIP_Y + 36

    if n == 1:
        x0, x1 = spans[0][0], spans[3][2]
        y = bot + 10
        b.append(f'<path d="M {x0:.1f} {y:.1f} L {x0:.1f} {y + 8:.1f} L {x1:.1f} {y + 8:.1f} '
                 f'L {x1:.1f} {y:.1f}" fill="none" stroke="{INK}" stroke-width="1.6"/>')
        b.append(text((x0 + x1) / 2, y + 34, "前文", 19, INK, "bold"))
        rc = spans[4][1]
        b.append(curve(x1 - 40, y + 12, rc, bot + 6, bend=70, color=GREEN, width=2.2))
        b.append(text(rc + 16, y + 60, "预测", 19, GREEN, "bold", anchor="start"))
        b.append(text(rc, CHIP_Y - 14, "下一个词", 18, GREEN, "bold"))

    if n == 2:
        ty = bot + 46
        for i, (_, c, _) in enumerate(spans):
            nxt = WORDS[i + 1] if i + 1 < len(WORDS) else "…"
            b.append(arrow(c, bot + 6, c, ty - 6, GREEN, 2))
            b += chip(c, ty, nxt, "upd", 18, pad=9, h=34, anchor="middle")[0]
        b.append(text(W / 2, ty + 34 + 52, "一段文本同时提供 8 个预测目标", 20, INK, "bold"))
    save(f"training-targets-{n}", b, H)


for n in (1, 2):
    frame(n)
