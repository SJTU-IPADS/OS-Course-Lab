#!/usr/bin/env python3
"""A token becomes a vector, and the direction of a vector carries meaning.

Left: every token of the sentence is looked up in the embedding matrix (a parameter of
the model, purple, one row per token of the vocabulary) and becomes one row of d numbers.
The numbers drawn are schematic.
Right: four word vectors on a plane, a schematic projection of the d-dimensional space.
The difference from "man" to "woman" equals the difference from "king" to "queen".
Run it to refresh ../assets/kv-token-vector.svg.
"""

from kv_kit import *
from kv_matrix import WORDS
from svgkit import box

W, H = 1000, 252
N = 4
TX, TW = 40, 64                           # token cells
BX, BW = 150, 140                         # the embedding matrix
RX, RW = 340, 232                         # the rows of numbers
Y0, RH, RP = 52, 30, 40                   # first row, row height, row pitch
NUMBERS = [(0.21, -1.30, 0.87), (-0.64, 0.95, 1.42), (1.08, 0.33, -0.71),
           (-0.92, -0.18, 0.56)]

OX, OY, UNIT = 700, 212, 100              # the plane: origin and pixels per unit
MAN, GENDER, ROYAL = (1.0, 0.25), (-0.5, 0.75), (0.9, 0.75)


def lookup():
    out = []
    top, bottom = Y0 - 8, Y0 + (N - 1) * RP + RH + 8
    out += box(BX, top, BW, bottom - top, "嵌入矩阵", fill=FILL_PURPLE, stroke=PURPLE, size=17,
               sub="每个 token 一行", sub_size=13)
    out += brace(RX + 46, RX + RW - 8, Y0 - 8, MUTED, "d 个数", 14, below=False)
    for i in range(N):
        y = Y0 + i * RP
        cy = y + RH / 2
        out += cell(TX, y, TW, RH, "", "mid")
        out.append(text(TX + TW / 2, cy + 6, WORDS[i], 16, INK, "bold"))
        out.append(arrow(TX + TW + 4, cy, BX - 5, cy, MUTED, 1.6))
        out.append(arrow(BX + BW + 3, cy, RX - 5, cy, MUTED, 1.6))
        out += cell(RX, y, RW, RH, "", "mid")
        out.append(mono(RX + 12, cy + 5, f"x{i + 1}", 14, INK, "bold"))
        for k, value in enumerate(NUMBERS[i]):
            out.append(mono(RX + 100 + k * 50, cy + 5, f"{value:.2f}", 13, INK, anchor="end"))
        out.append(text(RX + RW - 18, cy + 5, "⋯", 14, MUTED, "bold"))
    return out


def point(v):
    return OX + v[0] * UNIT, OY - v[1] * UNIT


def add(a, b):
    return a[0] + b[0], a[1] + b[1]


def between(a, b, trim=9):
    """The segment from a to b, shortened at both ends so it clears the arrow heads."""
    (x1, y1), (x2, y2) = point(a), point(b)
    length = ((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5
    ux, uy = (x2 - x1) / length * trim, (y2 - y1) / length * trim
    return x1 + ux, y1 + uy, x2 - ux, y2 - uy


def plane():
    woman, king = add(MAN, GENDER), add(MAN, ROYAL)
    queen = add(king, GENDER)
    out = [arrow(OX - 14, OY, OX + 2.4 * UNIT, OY, LINE, 1.4),
           arrow(OX, OY + 14, OX, OY - 1.95 * UNIT, LINE, 1.4)]
    for v in (MAN, woman, king, queen):
        out.append(arrow(OX, OY, *point(v), INK, 1.8))
    for a, b in ((MAN, woman), (king, queen)):
        out.append(arrow(*between(a, b), BLUE, 2.2, dash="6 4"))
    for v, word, dx, dy, anchor in ((MAN, "男人", 8, 16, "start"), (woman, "女人", -4, -11, "middle"),
                                    (king, "国王", 10, 6, "start"), (queen, "王后", 10, 4, "start")):
        x, y = point(v)
        out.append(text(x + dx, y + dy, word, 16, INK, "bold", anchor))
    out.append(text(OX + 1.15 * UNIT, H - 8, "「国王」−「男人」+「女人」≈「王后」", 15, BLUE, "bold"))
    return out


if __name__ == "__main__":
    save("kv-token-vector", W, H, lookup() + plane())
