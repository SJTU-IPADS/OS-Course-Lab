#!/usr/bin/env python3
"""temperature: softmax with a temperature T, on the video's inputs.

One figure bound to the deck variable T (`lecture.variable("T", ...)` in
lecture.py), drawn at its default. The probabilities, their bars and the
"T = ..." label carry the expressions they are computed from
(lecturekit.bind), and the viewer computes them again for the value the
slider or the box sets, any value from 0 to 10. `inspect` checks that the
file matches the expressions at the default.

The inputs are the video's; p = softmax(x, T), with T = 0 the limit (all the
weight on the largest input). Inputs on the left as signed bars, the
probabilities on the right, and between them an arrow labelled with the
formula, T in red under it.
"""
from lecturekit.bind import Bindings

from tk import *

T_DEFAULT = 1                            # lecture.variable("T", 1, ...)

H = 336
NAMES = ["Dumbledore", "Flitwick", "McGonagall", "Quirrell", "Snape", "Sprout", "Trelawney"]
X = [6.0, -5.0, 4.0, 1.5, 9.9, -2.3, 4.0]
Y0, RH, BH = 74, 37, 24
NX = 140

bind = Bindings(T=T_DEFAULT)
bind.let("x", X)
bind.let("p", "softmax(x, T)")


def rich(x, y, parts, size=22, anchor="middle"):
    """One line of math from (text, level[, colour[, binding]]) parts; level is
    '' (base), 'sup', 'sub' or 'supsub' (a subscript inside a superscript). A
    part with a binding takes its text from that expression."""
    geo = {"": (1.0, 0.0), "sup": (0.7, -0.42), "sub": (0.7, 0.25), "supsub": (0.5, -0.24)}
    spans, cur = [], 0.0
    for part in parts:
        s, lvl = part[0], part[1]
        c = part[2] if len(part) > 2 else INK
        k, off = geo[lvl]
        dy = off * size - cur
        cur = off * size
        style = "italic" if any(ch.isascii() and ch.isalpha() for ch in s) else "normal"
        span = (f'<tspan font-size="{k * size:.1f}" font-style="{style}" fill="{c}" '
                f'dy="{dy:.1f}">{esc(s)}</tspan>')
        spans.append(bind.text(span, part[3]) if len(part) > 3 else span)
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{MATH}" font-size="{size}" '
            f'text-anchor="{anchor}">' + "".join(spans) + "</text>")


def num(v, nd):
    return f"{v:.{nd}f}".replace("-", "−")


b = [bind.declare()]
bottom = Y0 + 6 * RH + BH
b.append(label(300, 44, "输入", 20, MUTED))
b.append(label(840, 44, "输出", 20, MUTED))
zero, scale = 298, 13
for i, (who, v) in enumerate(zip(NAMES, X)):
    top = Y0 + i * RH
    cy = top + BH / 2 + 6
    b.append(text(NX, cy + 1, who, 19, INK, anchor="end"))
    b.append(text(222, cy, num(v, 1), 18, INK, anchor="end", font=MONO))
    w = max(1.5, abs(v) * scale)
    if v < 0:
        b.append(rect(zero - w, top, w, BH, FILL_RED, RED, rx=2, width=1.4))
    else:
        b.append(rect(zero, top, w, BH, FILL_BLUE, BLUE, rx=2, width=1.4))
    b.append(bind.text(text(686, cy, "", 18, INK, anchor="end", font=MONO), f"fmt(p[{i}], 2)"))
    b.append(bind.attrs(rect(698, top, 0, BH, FILL_BLUE, BLUE, rx=2, width=1.4),
                        width=f"max(1.5, p[{i}] * 290)"))
b.append(line(zero, Y0 - 6, zero, bottom + 6, INK, 1.2))
b.append(line(698, Y0 - 6, 698, bottom + 6, INK, 1.2))
# the arrow, labelled with the formula, T in red
my = (Y0 + bottom) / 2
fx = 529
b.append(rich(fx, my - 52, [("e", ""), ("x", "sup"), ("i", "supsub"), (" / ", "sup"),
                            ("T", "sup", RED)], 24))
b.append(line(fx - 54, my - 42, fx + 54, my - 42, INK, 1.2))
b.append(rich(fx, my - 14, [("Σ", ""), ("j", "sub"), (" e", ""), ("x", "sup"),
                            ("j", "supsub"), (" / ", "sup"), ("T", "sup", RED)], 22))
b.append(arrow(458, my, 600, my, INK, 2))
b.append(rich(fx, my + 34, [("T", "", RED), ("", "", RED, "' = ' + fmt(T)")], 22))
save("temperature", b, H)
