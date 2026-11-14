#!/usr/bin/env python3
"""softmax-behavior: how the output follows the inputs.

Same names as softmax.py; inputs on the left, outputs (computed) on the
right, the edited inputs outlined in orange.
1: the video's edited input: Quirrell 8.5 is much larger than the rest and
   takes almost all the weight.
2: Quirrell back to 1.5, Trelawney raised to 3.3, close to Snape's 3.4: the
   two share most of the weight.
3: Quirrell at 5.0, between the two: its weight moves smoothly with its input.
"""
import math as m

from tk import *

H = 390
NAMES = ["Dumbledore", "Flitwick", "McGonagall", "Quirrell", "Snape", "Sprout", "Trelawney"]
F1 = [-0.8, -5.1, 0.5, 8.5, 3.4, -2.2, 2.4]          # on screen in the video
F2 = [-0.8, -5.1, 0.5, 1.5, 3.4, -2.2, 3.3]
F3 = [-0.8, -5.1, 0.5, 5.0, 3.4, -2.2, 3.3]
EDITED = {1: {3}, 2: {4, 6}, 3: {3}}
Y0, RH, BH = 70, 38, 24
NX = 140


def softmax(x):
    e = [m.exp(v) for v in x]
    s = sum(e)
    return [v / s for v in e]


def num(v, nd):
    return f"{v:.{nd}f}".replace("-", "−")


def row_y(i):
    return Y0 + i * RH


def frame(n):
    x = {1: F1, 2: F2, 3: F3}[n]
    p = softmax(x)
    b = []
    bottom = row_y(6) + BH
    b.append(label(300, 42, "输入", 20, MUTED))
    b.append(label(790, 42, "输出", 20, MUTED))
    zero, scale = 312, 13
    for i, (name, v, q) in enumerate(zip(NAMES, x, p)):
        top = row_y(i)
        cy = top + BH / 2 + 6
        b.append(text(NX, cy + 1, name, 19, INK, anchor="end"))
        if i in EDITED[n]:
            b.append(rect(170, top - 4, 62, BH + 8, FILL_ORANGE, ORANGE, rx=4, width=1.6))
        b.append(text(222, cy, num(v, 1), 18, INK, anchor="end", font=MONO))
        w = max(1.5, abs(v) * scale)
        if v < 0:
            b.append(rect(zero - w, top, w, BH, FILL_RED, RED, rx=2, width=1.4))
        else:
            b.append(rect(zero, top, w, BH, FILL_BLUE, BLUE, rx=2, width=1.4))
        b.append(text(700, cy, num(q, 2), 18, INK, anchor="end", font=MONO))
        b.append(rect(712, top, max(1.5, q * 250), BH, FILL_BLUE, BLUE, rx=2, width=1.4))
    b.append(line(zero, Y0 - 6, zero, bottom + 6, INK, 1.2))
    b.append(line(712, Y0 - 6, 712, bottom + 6, INK, 1.2))
    # the softmax box between them
    bx, bw = 470, 130
    b.append(rect(bx, (Y0 + bottom) / 2 - 36, bw, 72, FILL_GREY, LINE, rx=6, width=1.4))
    b.append(text(bx + bw / 2, (Y0 + bottom) / 2 + 8, "softmax", 22, INK, font=MATH))
    b.append(arrow(436, (Y0 + bottom) / 2, bx - 6, (Y0 + bottom) / 2, INK, 2))
    b.append(arrow(bx + bw + 6, (Y0 + bottom) / 2, 640, (Y0 + bottom) / 2, INK, 2))
    foot = bottom + 40
    if n == 1:
        s = f"Quirrell 的输入远大于其余输入，得到 {p[3]:.2f}"
    elif n == 2:
        s = f"Snape 与 Trelawney 的输入接近，合计得到 {round(p[4], 2) + round(p[6], 2):.2f}"
    else:
        series = []
        for q in (1.5, 5.0, 8.5):
            xs = list(F2)
            xs[3] = q
            series.append(f"{softmax(xs)[3]:.2f}")
        s = "Quirrell 的输入 1.5 → 5.0 → 8.5，输出 " + " → ".join(series)
    b.append(text(520, foot, s, 19, ORANGE, "bold"))
    save(f"softmax-behavior-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
