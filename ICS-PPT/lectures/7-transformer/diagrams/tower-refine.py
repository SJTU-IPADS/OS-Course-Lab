#!/usr/bin/env python3
"""tower-refine: context moves the embedding of tower, step by step.

A 2D view of the embedding space; points are placed by hand.
1: "tower": E(tower) (grey) sits among large, tall things.
2: "Eiffel tower": an update ΔE (green, dashed) moves it toward Paris,
   France and things made of steel; the new vector is green.
3: "miniature Eiffel tower": a second update moves it away from the large,
   tall things; the vector of step 2 stays, pale.
The current phrase is shown above the plot.
"""
from tk import *

H = 400
O = (500, 372)
T1, T2, T3 = (290, 160), (790, 172), (780, 316)
TALL = [("skyscraper", (214, 132), -10, 6, "end"),
        ("building", (352, 112), 10, -6, "start"),
        ("castle", (236, 200), -10, 6, "end")]
PARIS = [("Paris", (836, 112), 10, -6, "start"),
         ("France", (920, 150), 10, 6, "start"),
         ("钢铁", (860, 200), 10, 6, "start")]
PALE = "#9ec7ae"                    # a refined vector that a later step replaced
PHRASES = {1: ["tower"], 2: ["Eiffel", "tower"], 3: ["miniature", "Eiffel", "tower"]}


def region(cx, cy, rx, ry):
    return (f'<ellipse cx="{cx:.1f}" cy="{cy:.1f}" rx="{rx:.1f}" ry="{ry:.1f}" '
            f'fill="{FILL_GREY}" stroke="none"/>')


def frame(n):
    b = []
    # the phrase, its last chip (tower) fixed in place
    words = PHRASES[n]
    tower_left = 600
    left = tower_left - (row_width(words, gap=10) - row_width(["tower"]))
    els, _ = chip_row(left, 12, words, ["data"] * (len(words) - 1) + ["hl"])
    b += els

    b.append(region(282, 158, 150, 78))
    b.append(text(282, 68, "高大的事物", 18, MUTED))
    if n >= 2:
        b.append(region(872, 156, 118, 74))
    b.append(line(60, O[1], W - 40, O[1], LINE, 1.4))
    b.append(line(O[0], O[1] + 12, O[0], 70, LINE, 1.4))
    for word, (x, y), dx, dy, anchor in TALL:
        b += point(x, y, word, MUTED, dx, dy, 17, anchor, r=4)
    if n >= 2:
        for word, (x, y), dx, dy, anchor in PARIS:
            b += point(x, y, word, MUTED, dx, dy, 17, anchor, r=4)

    b.append(vec(*O, *T1, INK, 2.8, shrink=0))
    b += math(T1[0] + 14, T1[1] - 12, "E(\\up{tower})", 20, INK, anchor="start")
    if n >= 2:
        b.append(vec(*T1, *T2, GREEN, 2, dash="7 5", shrink=2))
        if n == 2:
            b.append(vec(*O, *T2, GREEN, 2.8, shrink=0))
        else:
            b.append(line(*O, *T2, PALE, 2))
        b.append(text(T2[0] - 10, T2[1] - 16, "Eiffel tower", 18, GREEN, anchor="end"))
    if n >= 3:
        b.append(vec(*T2, *T3, GREEN, 2, dash="7 5", shrink=2))
        b.append(vec(*O, *T3, GREEN, 2.8, shrink=0))
        b.append(text(T3[0] + 14, T3[1] + 6, "miniature Eiffel tower", 18, GREEN,
                      anchor="start"))
    save(f"tower-refine-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
