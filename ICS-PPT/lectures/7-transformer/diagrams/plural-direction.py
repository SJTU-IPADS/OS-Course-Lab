#!/usr/bin/env python3
"""plural-direction: E(cats) − E(cat) as a direction, measured by dot products.

The dot products are the values on screen in the video (a word-vector
model, not GPT-3).
1: E(cat), E(cats) and the step plur = E(cats) − E(cat) (orange).
2: plur · E(w) for octopus and three plurals of it: the plurals score higher.
3: plur · E(w) for one, two, three, four: the values rise.

The examples are parallel, so the figure is drawn once, in its final state;
frame() keeps the step numbers only to say which parts are drawn.
"""
from tk import *

H = 360
BASE = 214          # the zero line of the bar charts
SCALE = 36          # px per unit
OCTO = [("octopus", -2.76), ("octopuses", 1.38), ("octopi", 1.67), ("octopodes", 2.30)]
COUNT = [("one", -2.40), ("two", 0.79), ("three", 1.27), ("four", 1.80)]


def num(v):
    return f"{v:.2f}".replace("-", "−")


def bars(x0, items):
    bw, gap = 44, 36
    out = [line(x0 - 14, BASE, x0 + 4 * bw + 3 * gap + 14, BASE, INK, 1.4)]
    out += math(x0 + (4 * bw + 3 * gap) / 2, 44, "\\up{plur} · E(w)", 22)
    for i, (word, v) in enumerate(items):
        left = x0 + i * (bw + gap)
        h = abs(v) * SCALE
        c, f = (GREEN, FILL_GREEN) if v > 0 else (RED, FILL_RED)
        top = BASE - h if v > 0 else BASE
        out.append(rect(left, top, bw, h, f, c, rx=2, width=1.4))
        vy = top - 8 if v > 0 else BASE + h + 20
        out.append(text(left + bw / 2, vy, num(v), 17, c, "bold", font=MONO))
        wy = BASE + 24 if v > 0 else BASE - 10
        out.append(text(left + bw / 2, wy, word, 17, INK))
    return out


def frame(n):
    b = []
    o = (40, 320)
    b += axes(*o, 270, 250)
    cat, cats = (200, 110), (262, 192)
    b.append(vec(*o, *cat, DATA, 2.4, shrink=0))
    b.append(vec(*o, *cats, DATA, 2.4, shrink=0))
    b.append(vec(*cat, *cats, ORANGE, 2.6, shrink=2))
    b += math(cat[0] - 8, cat[1] - 12, "E(\\up{cat})", 21, INK, anchor="end")
    b += math(cats[0] + 10, cats[1] + 10, "E(\\up{cats})", 21, INK, anchor="start")
    b += math(cat[0] + 40, cat[1] + 32, "\\up{plur}", 22, ORANGE, anchor="start", weight="bold")
    b += math(170, 44, "\\up{plur} = E(\\up{cats}) − E(\\up{cat})", 22, ORANGE)
    if n >= 2:
        b += bars(384, OCTO)
    if n >= 3:
        b += bars(722, COUNT)
    save("plural-direction", b, H)


frame(3)
