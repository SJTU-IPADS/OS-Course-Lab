#!/usr/bin/env python3
"""prefix-invariance: appending a token leaves the old positions unchanged.

The grid of kv_grid.py, as in recompute.py: positions as columns, layers
1..3 from the bottom up. In each layer a position computes its query
(orange), key (blue) and value (purple), a triple centred on its column.
The vector of a position is no box of its own: it is the arrow that
carries it up its column, from the token chip into layer 1 and from each
layer into the next (dark and labelled h_4^(l) for "creature"). The
attention of "creature" (position 4) is drawn in every layer: its query
reads the key-value pairs of positions 1..4; the read of each later
position is cut by the mask (dashed, crossed).
1: the prefix "a fluffy blue creature roamed"; column 6 is an empty slot.
2: "the" is appended and computed up its column; its read by "creature"
   is cut as well. Positions 1..5 are framed: every vector there is the
   one of frame 1.
"""
from tk import *
from kv_grid import *

BUS_UP = 22                        # the read line runs this far above a row
T = 3                              # the example: "creature", position 4


def qkv(i, layer):
    x, y = cx(i), ROW_Y[layer]
    return (mvec(x - STEP, y, f"pi-q-{layer}-{i}", Q) +
            mvec(x, y, f"pi-k-{layer}-{i}", K) +
            mvec(x + STEP, y, f"pi-v-{layer}-{i}", V))


def empty_slot(i, layer):
    x, y = cx(i), ROW_Y[layer]
    w = 2 * STEP + MW + 12
    return [rect(x - w / 2, y - HALF - 2, w, 2 * HALF + 4, "none", LINE, rx=4, width=1.4,
                 dash="5 4")]


def flow(i):
    """The vector of position i moving up its column: from the chip into
    layer 1, then from each layer into the next. The example's column is
    drawn dark, as the new token's in kv-cache.py."""
    x = cx(i)
    c, w = (INK, 1.8) if i == T else (LINE, 1.6)
    out = [arrow(x, CHIP_Y - 4, x, ROW_Y[1] + HALF + 4, c, w)]
    for lo in (1, 2):
        out.append(arrow(x, ROW_Y[lo] - HALF - 1, x, ROW_Y[lo + 1] + HALF + 4, c, w))
    return out


def read(layer, last):
    """The query of position T reads the pairs of positions 0..T; the pairs
    of T+1..last are masked."""
    y = ROW_Y[layer]
    top = y - HALF
    bus = top - BUS_UP
    px = lambda j: cx(j) + STEP / 2      # between k and v: the pair
    out = []
    for j in range(T + 1, last + 1):
        out.append(line(px(j - 1), bus, px(j), bus, MUTED, 1.6, dash="4 4"))
        out.append(line(px(j), bus, px(j), top - 1, MUTED, 1.6, dash="4 4"))
        mx, my = px(j), bus + BUS_UP / 2
        out.append(line(mx - 5, my - 5, mx + 5, my + 5, MUTED, 2.2))
        out.append(line(mx - 5, my + 5, mx + 5, my - 5, MUTED, 2.2))
    out.append(elbow([(px(0), top - 1), (px(0), bus), (px(T), bus), (px(T), top - 1)], Q,
                     1.8, head=False))
    for j in range(1, T):
        out.append(line(px(j), bus, px(j), top - 1, Q, 1.6))
    qx = cx(T) - STEP
    out.append(arrow(qx, bus, qx, top - 3, Q, 1.8))
    return out


def frame(n):
    b = layer_labels()
    last = 4 if n == 1 else 5           # last position present
    for i in range(last + 1):
        b += token(i, "upd" if i == 5 else "data")
        b += flow(i)
    for lo in (1, 2):
        # in the gap above layer lo, just over its read line
        b += math(cx(T) - 8, ROW_Y[lo] - HALF - BUS_UP - 12, f"h_4^{{({lo})}}", 20, INK,
                  anchor="end")
    for layer in ROW_Y:
        b += read(layer, last)
        for i in range(6):
            b += qkv(i, layer) if i <= last else empty_slot(i, layer)
    if n == 2:
        x0, x1 = cx(0) - STEP - MW / 2 - 14, cx(4) + STEP + MW / 2 + 14
        y0, y1 = ROW_Y[3] - HALF - BUS_UP - 8, ROW_Y[1] + HALF + 8
        b.append(rect(x0, y0, x1 - x0, y1 - y0, "none", MUTED, rx=10, width=1.6, dash="6 5"))
        b.append(text(x0 + 4, y0 - 8, "与追加前相同", 19, INK, "bold", anchor="start"))
    b += legend(QKV)
    save(f"prefix-invariance-{n}", b, H)


for n in (1, 2):
    frame(n)
