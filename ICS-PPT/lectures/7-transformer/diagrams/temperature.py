#!/usr/bin/env python3
"""temperature: softmax with a temperature T, on the video's inputs.

The inputs and the three temperatures are the video's; the outputs are
computed here (T = 0 is the limit: all the weight on the largest input).
The formula and a T scale sit above the table, T in red.
1: T = 10: the distribution is flatter.
2: T = 1.76: the largest input dominates.
3: T = 0: all the weight on 9.9.
"""
import math as m

from tk import *

H = 400
NAMES = ["Dumbledore", "Flitwick", "McGonagall", "Quirrell", "Snape", "Sprout", "Trelawney"]
X = [6.0, -5.0, 4.0, 1.5, 9.9, -2.3, 4.0]
TEMPS = {1: 10, 2: 1.76, 3: 0}
Y0, RH, BH = 128, 37, 24
NX = 140


def softmax(x, t):
    if t == 0:
        top = max(x)
        hits = [1.0 if v == top else 0.0 for v in x]
        return [h / sum(hits) for h in hits]
    e = [m.exp(v / t) for v in x]
    s = sum(e)
    return [v / s for v in e]


def rich(x, y, parts, size=22, anchor="middle"):
    """One line of math from (text, level[, colour]) parts; level is '' (base),
    'sup', 'sub' or 'supsub' (a subscript inside a superscript)."""
    geo = {"": (1.0, 0.0), "sup": (0.7, -0.42), "sub": (0.7, 0.25), "supsub": (0.5, -0.24)}
    spans, cur = [], 0.0
    for part in parts:
        s, lvl = part[0], part[1]
        c = part[2] if len(part) > 2 else INK
        k, off = geo[lvl]
        dy = off * size - cur
        cur = off * size
        style = "italic" if any(ch.isascii() and ch.isalpha() for ch in s) else "normal"
        spans.append(f'<tspan font-size="{k * size:.1f}" font-style="{style}" fill="{c}" '
                     f'dy="{dy:.1f}">{esc(s)}</tspan>')
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{MATH}" font-size="{size}" '
            f'text-anchor="{anchor}">' + "".join(spans) + "</text>")


def num(v, nd):
    return f"{v:.{nd}f}".replace("-", "−")


def tlabel(t):
    return f"{t:g}"


def frame(n):
    t = TEMPS[n]
    p = softmax(X, t)
    b = []
    # the formula, T in red
    fx = 250
    b.append(rich(fx - 72, 62, [("p", ""), ("i", "sub"), (" =", "")], 26, "end"))
    b.append(rich(fx, 46, [("e", ""), ("x", "sup"), ("i", "supsub"), (" / ", "sup"),
                           ("T", "sup", RED)], 28))
    b.append(line(fx - 62, 56, fx + 62, 56, INK, 1.4))
    b.append(rich(fx, 90, [("Σ", ""), ("j", "sub"), (" e", ""), ("x", "sup"), ("j", "supsub"),
                           (" / ", "sup"), ("T", "sup", RED)], 26))
    # the T scale
    sx0, sx1, sy = 560, 960, 74
    b.append(line(sx0, sy, sx1, sy, INK, 1.4))
    for k in range(11):
        x = sx0 + (sx1 - sx0) * k / 10
        b.append(line(x, sy - (6 if k % 5 == 0 else 4), x, sy + (6 if k % 5 == 0 else 4),
                      INK, 1.2))
        if k % 5 == 0:
            b.append(text(x, sy + 24, str(k), 16, MUTED, font=MONO))
    tx = sx0 + (sx1 - sx0) * t / 10
    b.append(path(f"M {tx - 8:.1f} {sy - 22:.1f} L {tx + 8:.1f} {sy - 22:.1f} "
                  f"L {tx:.1f} {sy - 8:.1f} Z", RED, 1, fill=RED, head=False))
    b.append(rich(tx + (14 if t < 9 else -14), sy - 24,
                  [("T", "", RED), (f" = {tlabel(t)}", "", RED)], 24,
                  "start" if t < 9 else "end"))
    # the table
    bottom = Y0 + 6 * RH + BH
    zero, scale = 298, 13
    for i, (name, v, q) in enumerate(zip(NAMES, X, p)):
        top = Y0 + i * RH
        cy = top + BH / 2 + 6
        b.append(text(NX, cy + 1, name, 19, INK, anchor="end"))
        b.append(text(222, cy, num(v, 1), 18, INK, anchor="end", font=MONO))
        w = max(1.5, abs(v) * scale)
        if v < 0:
            b.append(rect(zero - w, top, w, BH, FILL_RED, RED, rx=2, width=1.4))
        else:
            b.append(rect(zero, top, w, BH, FILL_BLUE, BLUE, rx=2, width=1.4))
        b.append(text(686, cy, num(q, 2), 18, INK, anchor="end", font=MONO))
        b.append(rect(698, top, max(1.5, q * 290), BH, FILL_BLUE, BLUE, rx=2, width=1.4))
    b.append(line(zero, Y0 - 6, zero, bottom + 6, INK, 1.2))
    b.append(line(698, Y0 - 6, 698, bottom + 6, INK, 1.2))
    my = (Y0 + bottom) / 2
    b.append(arrow(458, my, 600, my, INK, 2))
    b.append(text(529, my - 12, "softmax", 20, INK, font=MATH))
    b.append(rich(529, my + 30, [("T", "", RED), (f" = {tlabel(t)}", "", RED)], 20))
    save(f"temperature-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
