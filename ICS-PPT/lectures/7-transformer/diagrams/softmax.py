#!/usr/bin/env python3
"""softmax: turning seven raw scores into a probability distribution.

The names and the scores are the video's example; every other number is
computed here.
1: the raw scores x_i as signed bars: some are negative and they do not
   sum to 1.
2: e^{x_i} of each: all positive; their sum.
3: each e^{x_i} divided by that sum: the probabilities.
"""
import math as m

from tk import *

H = 375
NAMES = ["Dumbledore", "Flitwick", "McGonagall", "Quirrell", "Snape", "Sprout", "Trelawney"]
X = [-0.8, -5.0, 0.5, 1.5, 3.4, -2.3, 2.5]
E = [m.exp(x) for x in X]
TOTAL = sum(E)
P = [e / TOTAL for e in E]
Y0, RH, BH = 70, 38, 24     # top of the rows, row pitch, bar height
NX = 140                    # right edge of the names


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


def row_y(i):
    return Y0 + i * RH


def column(vx, values, nd, zero, scale, signed=False):
    """Values right-aligned at vx, bars from `zero` (both ways when signed)."""
    out = []
    for i, v in enumerate(values):
        top = row_y(i)
        out.append(text(vx, top + BH / 2 + 6, num(v, nd), 18, INK, anchor="end", font=MONO))
        w = max(1.5, abs(v) * scale)
        if signed and v < 0:
            out.append(rect(zero - w, top, w, BH, FILL_RED, RED, rx=2, width=1.4))
        else:
            out.append(rect(zero, top, w, BH, FILL_BLUE, BLUE, rx=2, width=1.4))
    bottom = row_y(len(values) - 1) + BH
    out.append(line(zero, Y0 - 6, zero, bottom + 6, INK, 1.2))
    return out


def frame(n):
    b = []
    for i, name in enumerate(NAMES):
        b.append(text(NX, row_y(i) + BH / 2 + 7, name, 19, INK, anchor="end"))
    foot = row_y(6) + BH + 36
    # 1: the raw scores
    b.append(rich(300, 42, [("x", ""), ("i", "sub")], 24))
    b += column(222, X, 1, 330, 19, signed=True)
    b.append(text(282, foot, "不是概率分布：有负数，和不为 1", 18, RED))
    if n >= 2:
        b.append(rich(590, 42, [("e", ""), ("x", "sup"), ("i", "supsub")], 26))
        b += column(522, E, 2, 534, 160 / max(E))
        b.append(text(590, foot, f"总和 {TOTAL:.2f}", 18, INK, font=FONT))
    if n >= 3:
        b.append(rich(880, 42, [("e", ""), ("x", "sup"), ("i", "supsub"),
                                (f" / {TOTAL:.2f}", "")], 26))
        b += column(806, P, 2, 818, 200)
        b.append(text(880, foot, f"总和 {sum(P):.0f}", 18, GREEN, "bold"))
    save(f"softmax-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
