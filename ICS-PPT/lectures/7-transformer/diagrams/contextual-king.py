#!/usr/bin/env python3
"""contextual-king: the vector that starts as E(king) absorbs its context.

The line of text above is the video's example; the 2D view is placed by
hand. Each frame adds one context step as a green arrow, tip to tail:
1: E(king), then + lived in Scotland.
2: + took the throne by murdering his predecessor.
3: + described in Shakespearean language; the sum, from the origin, is the
   king of this text.
"""
from tk import *

H = 400
O = (80, 382)


def P(x, y):
    return O[0] + x, O[1] - y


K0 = P(170, 150)
STEPS = [((180, 50), "+ 生活在苏格兰"), ((200, 60), "+ 谋杀前任而即位"),
         ((220, -110), "+ 用莎士比亚式的语言描写")]


def sentence(cx, y, size=22):
    """The line of text, King boxed: three runs, so the browser spaces the words."""
    head, word, tail = "The", "King", "doth wake tonight and takes his rouse ..."
    gap = 10
    kw = width(word, size) + 12
    total = width(head, size) + kw + width(tail, size) + 2 * gap
    left = cx - total / 2 + width(head, size) + gap
    return [text(left - gap, y, head, size, INK, anchor="end"),
            rect(left, y - size - 3, kw, size + 13, FILL_ORANGE, ORANGE, rx=4, width=1.6),
            text(left + kw / 2, y, word, size, ORANGE, "bold"),
            text(left + kw + gap, y, tail, size, INK, anchor="start")]


def frame(n):
    b = sentence(540, 38)
    b += axes(*O, 900, 300)
    b.append(vec(*O, *K0, DATA, 2.6, shrink=0))
    b += math(K0[0] + 10, K0[1] + 26, "E(\\up{king})", 21, DATA, anchor="start")
    at = K0
    for k, ((dx, dy), word) in enumerate(STEPS[:n]):
        to = (at[0] + dx, at[1] - dy)
        b.append(vec(*at, *to, GREEN, 2.6, shrink=0))
        mx, my = (at[0] + to[0]) / 2, (at[1] + to[1]) / 2
        if dy > 0:
            b.append(text(mx - 12, my - 14, word, 20, GREEN, "bold", "end"))
        else:
            b.append(text(mx + 14, my - 18, word, 20, GREEN, "bold", "start"))
        at = to
    if n == 3:
        b.append(vec(*O, *at, INK, 3.2, shrink=0))
        b.append(text(at[0] + 12, at[1] + 30, "上下文中的 king", 20, INK, "bold", "start"))
    save(f"contextual-king-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
