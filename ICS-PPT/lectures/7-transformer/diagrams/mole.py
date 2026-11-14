#!/usr/bin/env python3
"""mole: one lookup vector, three meanings.

1: three phrases; the token mole (orange) in each gets the same vector from
   the lookup table (the numbers are the video's).
2: a 2D view of the embedding space: E(mole) and three dashed directions,
   one per meaning.
3: for each phrase, the change ΔE (green) that attention must add to move
   E(mole) toward the meaning of that phrase.
"""
import math as _m

from tk import *

H = 340
PHRASES = [["American", "shrew", "mole"],
           ["One", "mole", "of", "carbon", "dioxide"],
           ["Take", "a", "biopsy", "of", "the", "mole"]]
ROW_X, ROW_Y, ROW_DY = 28, 22, 82
CS = 19                                      # chip font size
VX, VY = 470, 34                             # the lookup vector
VALUES = [5.8, 9.9, 2.5, 3.7, 9.1, "⋮", 2.1]

O = (610, 296)                               # origin of the 2D view


def polar(deg, r):
    a = _m.radians(deg)
    return O[0] + r * _m.cos(a), O[1] - r * _m.sin(a)


TIP = polar(40, 170)
MEANINGS = [  # (label, ray angle, where ΔE lands, label placement)
    ("动物 (鼹鼠)", 80, 228, (12, 6, "start")),
    ("化学单位 (摩尔)", 50, 296, (12, 4, "start")),
    ("皮肤上的痣", 17, 330, (0, -16, "end")),
]


def frame(n):
    b = []
    # the phrases and the lookup
    for r, words in enumerate(PHRASES):
        y = ROW_Y + r * ROW_DY
        roles = ["hl" if w == "mole" else "data" for w in words]
        els, spans = chip_row(ROW_X, y, words, roles, size=CS, gap=8, h=34, pad=9)
        b += els
        k = words.index("mole")
        cx = spans[k][1]
        lane = y + 34 + 16
        b.append(elbow([(cx, y + 36), (cx, lane), (VX - 10, lane)], MUTED, 1.8))
    els, vh = vector(VX, VY, VALUES, "data", cw=54, ch=30, size=16)
    b += els
    b += math(VX + 27, VY + vh + 30, "E(\\up{mole})", 21, INK)
    b.append(text(VX + 27, VY + vh + 58, "同一个查表向量", 18, MUTED))

    if n >= 2:
        b.append(line(O[0] - 20, O[1], W - 24, O[1], LINE, 1.4))
        b.append(line(O[0], O[1] + 14, O[0], 22, LINE, 1.4))
        for name, deg, _, (dx, dy, anchor) in MEANINGS:
            end = polar(deg, 1000)
            # clip the ray to the plot box
            t = min((W - 40 - O[0]) / max(end[0] - O[0], 1e-6),
                    (O[1] - 34) / max(O[1] - end[1], 1e-6))
            ex, ey = O[0] + (end[0] - O[0]) * t, O[1] + (end[1] - O[1]) * t
            b.append(line(*O, ex, ey, MUTED, 1.6, dash="6 6"))
            b.append(text(ex + dx, ey + dy, name, 18, INK, anchor=anchor))
        b.append(vec(*O, *TIP, INK, 2.8, shrink=0))
        b += math(TIP[0] + 14, TIP[1] + 34, "E(\\up{mole})", 21, INK, anchor="start")
    if n >= 3:
        for _, deg, r, _ in MEANINGS:
            b.append(vec(*TIP, *polar(deg, r), GREEN, 2.6, shrink=2))
            b.append(dot(*polar(deg, r), 4, GREEN))
        b.append(text((O[0] + W - 24) / 2 + 10, O[1] + 34, "注意力计算要加上的 ΔE，取决于上下文",
                      18, GREEN, "bold"))
    save(f"mole-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
