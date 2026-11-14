#!/usr/bin/env python3
"""embedding-matrix: W_E has one column per token of the vocabulary.

1: W_E as a grid of weights, a vocabulary word above each column (the first
   few, one in the middle, the last), 50,257 columns across, 12,288 rows down.
2: the input tower picks its column; the column is copied out as the
   embedding E(tower), a data vector holding the same numbers.
"""
from tk import *

H = 400
CW, CH, ROWS = 40, 26, 8
GX, GY = 185, 124
COLS = ["aah", "aardvark", "aardwolf", "aargh", "ab", "aback", None, "tower", None,
        "zyzzyva"]
TOWER = COLS.index("tower")


def rot(x, y, s, size=17, fill=INK, weight="normal", angle=-52):
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{FONT}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="start" '
            f'transform="rotate({angle} {x:.1f} {y:.1f})">{esc(s)}</text>')


def value(i, word):
    """Entry i of a word's column: the number its cell's shade stands for."""
    return round(pseudo("WE", i, word) * 9.9, 1)


def frame(n):
    b = []
    right = GX + len(COLS) * CW
    bottom = GY + ROWS * CH
    for j, word in enumerate(COLS):
        x = GX + j * CW
        if word is None:
            for i in range(ROWS):
                b.append(text(x + CW / 2, GY + (i + .5) * CH + 6, "⋯", 17, MUTED))
            b.append(text(x + CW / 2, GY - 14, "…", 18, MUTED))
            continue
        for i in range(ROWS):
            b.append(rect(x, GY + i * CH, CW, CH, weight_fill(value(i, word) / 9.9), WHITE,
                          rx=0, width=1))
        hl = n == 2 and j == TOWER
        b.append(rot(x + CW / 2 - 4, GY - 10, word, 18 if hl else 17, ORANGE if hl else INK,
                     "bold" if hl else "normal"))
    b.append(rect(GX, GY, right - GX, bottom - GY, "none", INK, rx=2, width=1.6))
    b += math(GX - 22, (GY + bottom) / 2 + 10, "W_E", 30, anchor="end")
    b += brace(GX, right, 52, INK, "50,257 个 token", size=18, below=False)
    b += vbrace(right + 10, GY, bottom, INK, "12,288 维", size=18)

    if n == 2:
        cx = GX + (TOWER + .5) * CW
        b.append(rect(cx - CW / 2 - 3, GY - 3, CW + 6, ROWS * CH + 6, "none", ORANGE, rx=3,
                      width=2.6))
        vx = 800
        els, _ = vector(vx, GY, [value(i, "tower") for i in range(ROWS)], "data", cw=64,
                        ch=CH, size=15)
        b += els
        b += math(vx + 82, (GY + bottom) / 2 + 9, "E(\\up{tower})", 26, anchor="start")
        y = bottom + 22
        b.append(elbow([(cx, bottom + 6), (cx, y), (vx + 32, y), (vx + 32, bottom + 8)],
                       ORANGE, 2.2))
        b.append(label((cx + vx + 32) / 2, y + 25, "取出 tower 这一列", 18,
                       ORANGE))
    save(f"embedding-matrix-{n}", b, H)


for n in (1, 2):
    frame(n)
