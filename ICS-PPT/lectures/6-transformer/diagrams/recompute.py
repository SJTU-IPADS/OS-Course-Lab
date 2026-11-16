#!/usr/bin/env python3
"""recompute: rerunning the whole prefix after one token is appended.

The grid of kv_grid.py: columns are positions, rows are layers 1..3 from
the bottom up; each state is a hidden vector (grey, data).
1: the states after processing "a fluffy blue creature roamed".
2: "the" is appended and the extended prefix is run again: layer 1 is
   computed at all six positions. The five old states, which were already
   known, are hatched in INK: repeated work. No other figure of the deck
   is hatched, and the states keep the grey of data, so the mark takes no
   colour of the part's code (orange is the query on the next pages).
3: all three layers are recomputed: 3 x 5 old states are repeated work.
"""
from tk import *
from kv_grid import *

SCH, SCW = 12, 28                  # a hidden state: 4 cells of 28 x 12
SHALF = CELLS * SCH / 2 + 3        # half height with its border
HATCH_GAP = 6                      # hatching: distance between two strokes


def hatch(left, top, w, h, gap=HATCH_GAP):
    """Strokes at 45 degrees across the rectangle, clipped to it."""
    out = []
    c = left + top + gap / 2
    while c < left + w + top + h:
        x0, x1 = max(left, c - top - h), min(left + w, c - top)
        if x1 - x0 > 3:                 # no stub in a corner
            out.append(line(x0, c - x0, x1, c - x1, INK, 1.2))
        c += gap
    return out


def state(i, layer, seed, repeated=False, empty=False):
    x, y = cx(i), ROW_Y[layer]
    if empty:
        return mvec(x, y, seed, DATA, ch=SCH, mw=SCW, pad=3, dash="5 4")
    out = mvec(x, y, seed, DATA, ch=SCH, mw=SCW, pad=3, width=1.8)
    if repeated:
        out += hatch(x - SCW / 2, y - CELLS * SCH / 2, SCW, CELLS * SCH)
    return out


def frame(n):
    b = layer_labels()
    for i in range(5):
        b += token(i)
    if n >= 2:
        b += token(5, "upd")
    done = {1: 0, 2: 1, 3: 3}[n]          # layers rerun so far
    # the vector of each position moves up through the layers
    for i in range(6):
        if i == 5 and n == 1:
            continue
        b.append(arrow(cx(i), CHIP_Y - 4, cx(i), ROW_Y[1] + SHALF + 4, LINE, 1.6))
        for lo, hi in ((1, 2), (2, 3)):
            if i < 5 or hi <= done:
                b.append(arrow(cx(i), ROW_Y[lo] - SHALF - 2, cx(i), ROW_Y[hi] + SHALF + 4,
                               LINE, 1.6))
    for layer, y in ROW_Y.items():
        for i in range(6):
            seed = f"rc-{layer}-{i}"
            if i < 5:
                b += state(i, layer, seed, repeated=layer <= done)
            else:
                b += state(i, layer, seed, empty=layer > done)
        if layer <= done:
            b.append(text(cx(5) + 40, y + 7, "5 个重复", 19, INK, "bold", anchor="start"))
    if n == 3:
        bx = cx(5) + 140
        y0, y1 = ROW_Y[3] - 26, ROW_Y[1] + 26
        b.append(path(f"M {bx - 8:.1f} {y0:.1f} L {bx:.1f} {y0:.1f} L {bx:.1f} {y1:.1f} "
                      f"L {bx - 8:.1f} {y1:.1f}", INK, 1.8, head=False))
        b += math(bx + 14, (y0 + y1) / 2 + 7, "3 × 5 = 15", 22, INK, anchor="start")
    save(f"recompute-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
