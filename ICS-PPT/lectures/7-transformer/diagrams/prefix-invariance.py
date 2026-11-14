#!/usr/bin/env python3
"""prefix-invariance: appending a token leaves the old states unchanged.

The grid of recompute.py: positions as columns, layers 1..3 from the bottom
up, "the" appended at position 6.
1: the state of "creature" (position 4) in layer 2 is computed from the
   layer-1 states of positions 1..4 only; positions 5 and 6 are masked.
   By induction over the layers, no old state depends on position 6, so
   the old states are the ones computed before.
2: each old state comes with the key and value its layer computed from it;
   those K, V of positions 1..5 in every layer can be reused.
"""
from tk import *

H = 380
WORDS = "a fluffy blue creature roamed the".split()
PITCH, X0 = 112, 196
ROW_Y = {1: 274, 2: 170, 3: 66}
CELLS, CH, CW = 4, 11, 28
CHIP_Y = 334
TARGET = (2, 3)            # layer 2, position 4 (index 3)


def cx(i):
    return X0 + i * PITCH


def tint(color_hex, t):
    base = tuple(int(color_hex[k:k + 2], 16) for k in (1, 3, 5))
    r, g, b_ = (round(255 + (c - 255) * t) for c in base)
    return f"#{r:02x}{g:02x}{b_:02x}"


def state(x, y, seed, color=DATA, cw=CW, width=1.8):
    top, left = y - CELLS * CH / 2, x - cw / 2
    out = [rect(left - 3, top - 3, cw + 6, CELLS * CH + 6, WHITE, color, rx=4, width=width)]
    for k in range(CELLS):
        t = 0.15 + 0.55 * abs(pseudo(seed, k, 0))
        out.append(rect(left, top + k * CH, cw, CH, tint(color, t), WHITE, rx=0, width=1))
    return out


def token(x, word, role="data"):
    els, _ = chip(x, CHIP_Y, word, role, size=18, h=34, anchor="middle")
    return els


def frame(n):
    b = []
    half = CELLS * CH / 2 + 3
    for layer, y in ROW_Y.items():
        b.append(text(cx(0) - 54, y + 7, f"第 {layer} 层", 19, INK, anchor="end"))
    for i, word in enumerate(WORDS):
        b += token(cx(i), word, "upd" if i == 5 else "data")

    # the old positions
    rx0, rx1 = cx(0) - 34, cx(4) + 62
    ry0, ry1 = ROW_Y[3] - half - 12, ROW_Y[1] + half + 12
    if n == 1:
        b.append(rect(rx0, ry0, rx1 - rx0, ry1 - ry0, "none", MUTED, rx=10, width=1.6,
                      dash="6 5"))
        b.append(text(rx0 + 4, ry0 - 10, "旧位置的状态不变", 19, INK, "bold", anchor="start"))
    else:
        b.append(rect(rx0, ry0, rx1 - rx0, ry1 - ry0, FILL_GREEN, UPD, rx=10, width=2))
        b.append(text(rx0 + 4, ry0 - 10, "各层的 K、V 可以复用", 19, UPD, "bold",
                      anchor="start"))

    # what the target state reads: positions 1..4 of the layer below
    tl, ti = TARGET
    tx, ty = cx(ti), ROW_Y[tl] + half + 3
    src_y = ROW_Y[tl - 1] - half - 1
    for j in range(ti + 1):
        b.append(vec(cx(j), src_y, tx + (j - ti / 2) * 7, ty, INK, 1.8, shrink=0))
    for j in (ti + 1, ti + 2):
        # masked: the line stops short of the target
        x0, y0 = cx(j), src_y
        ex, ey = x0 + (tx + 14 - x0) * 0.62, y0 + (ty - y0) * 0.62
        b.append(line(x0, y0, ex, ey, MUTED, 1.6, dash="4 4"))
        b.append(line(ex - 6, ey - 6, ex + 6, ey + 6, MUTED, 2.2))
        b.append(line(ex - 6, ey + 6, ex + 6, ey - 6, MUTED, 2.2))

    for layer, y in ROW_Y.items():
        for i in range(6):
            target = (layer, i) == TARGET
            b += state(cx(i), y, f"rc-{layer}-{i}", INK if target else DATA,
                       width=2.6 if target else 1.8)
            if n == 2 and i < 5:
                b += state(cx(i) + 30, y, f"pi-k-{layer}-{i}", K, cw=11)
                b += state(cx(i) + 47, y, f"pi-v-{layer}-{i}", V, cw=11)
    if n == 2:
        # legend, on the line of the region's title
        lx, ly = rx1 + 40, ry0 - 10
        for k, (sym, name, c, f) in enumerate([("k", "键", K, FILL_K), ("v", "值", V, FILL_V)]):
            x = lx + k * 90
            b.append(rect(x, ly - 15, 16, 18, f, c, rx=3, width=1.6))
            b += math(x + 24, ly, sym, 20, c, anchor="start")
            b.append(text(x + 38, ly, name, 18, c, anchor="start"))
    b += math(tx, ROW_Y[tl] - half - 10, "h_4^{(2)}", 20, INK)
    save(f"prefix-invariance-{n}", b, H)


for n in (1, 2):
    frame(n)
