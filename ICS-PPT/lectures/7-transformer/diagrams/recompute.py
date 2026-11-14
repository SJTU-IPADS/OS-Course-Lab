#!/usr/bin/env python3
"""recompute: rerunning the whole prefix after one token is appended.

A grid of hidden states: columns are positions, rows are layers 1..3 from
the bottom up; each state is a small column vector.
1: the states after processing "a fluffy blue creature roamed" (grey).
2: "the" is appended and the extended prefix is run again: layer 1 is
   computed at all six positions; orange marks the five old positions,
   whose states were already known.
3: all three layers are recomputed: 3 x 5 old states are repeated work.
"""
from tk import *

H = 352
WORDS = "a fluffy blue creature roamed the".split()
PITCH, X0 = 112, 196               # column pitch, centre of position 1
ROW_Y = {1: 248, 2: 158, 3: 68}    # centre of each layer's row
CELLS, CH, CW = 4, 12, 28          # a state vector: 4 cells
CHIP_Y = 306


def cx(i):
    return X0 + i * PITCH


def tint(color_hex, t):
    base = tuple(int(color_hex[k:k + 2], 16) for k in (1, 3, 5))
    r, g, b_ = (round(255 + (c - 255) * t) for c in base)
    return f"#{r:02x}{g:02x}{b_:02x}"


def state(x, y, seed, color=DATA, dash=None):
    """A hidden vector centred at (x, y): a column of shaded cells. With
    dash, only the empty outline of a slot."""
    top, left = y - CELLS * CH / 2, x - CW / 2
    if dash:
        return [rect(left - 3, top - 3, CW + 6, CELLS * CH + 6, "none", LINE, rx=4,
                     width=1.4, dash=dash)]
    out = [rect(left - 3, top - 3, CW + 6, CELLS * CH + 6, WHITE, color, rx=4, width=1.8)]
    for k in range(CELLS):
        t = 0.15 + 0.55 * abs(pseudo(seed, k, 0))
        out.append(rect(left, top + k * CH, CW, CH, tint(color, t), WHITE, rx=0, width=1))
    return out


def token(x, word, role="data"):
    els, _ = chip(x, CHIP_Y, word, role, size=18, h=34, anchor="middle")
    return els


def frame(n):
    b = []
    for layer, y in ROW_Y.items():
        b.append(text(cx(0) - 54, y + 7, f"第 {layer} 层", 19, INK, anchor="end"))
    for i, word in enumerate(WORDS[:5]):
        b += token(cx(i), word)
    if n >= 2:
        b += token(cx(5), WORDS[5], "upd")
    done = {1: 0, 2: 1, 3: 3}[n]          # layers rerun so far
    # the vector of each position moves up through the layers
    half = CELLS * CH / 2 + 3
    for i in range(6):
        if i == 5 and n == 1:
            continue
        b.append(arrow(cx(i), CHIP_Y - 2, cx(i), ROW_Y[1] + half + 4, LINE, 1.6))
        for lo, hi in ((1, 2), (2, 3)):
            if i < 5 or hi <= done:
                b.append(arrow(cx(i), ROW_Y[lo] - half - 2, cx(i), ROW_Y[hi] + half + 4,
                               LINE, 1.6))
    for layer, y in ROW_Y.items():
        for i in range(6):
            seed = f"rc-{layer}-{i}"
            if i < 5:
                b += state(cx(i), y, seed, ORANGE if layer <= done else DATA)
            elif layer <= done:
                b += state(cx(i), y, seed)
            else:
                b += state(cx(i), y, seed, dash="5 4")
        if layer <= done:
            b.append(text(cx(5) + 56, y + 7, "5 个重复", 19, ORANGE, "bold", anchor="start"))
    if n == 3:
        bx = cx(5) + 160
        y0, y1 = ROW_Y[3] - 26, ROW_Y[1] + 26
        b.append(path(f"M {bx - 8:.1f} {y0:.1f} L {bx:.1f} {y0:.1f} L {bx:.1f} {y1:.1f} "
                      f"L {bx - 8:.1f} {y1:.1f}", ORANGE, 1.8, head=False))
        b += math(bx + 14, (y0 + y1) / 2 + 7, "3 × 5 = 15", 22, ORANGE, anchor="start")
    save(f"recompute-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
