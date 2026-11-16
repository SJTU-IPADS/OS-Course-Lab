#!/usr/bin/env python3
"""kv-cache: one decode step layer by layer, with a K, V cache per layer.

The grid of kv_grid.py, as in prefix-invariance.py: positions as columns,
layers 1..3 from the bottom up. Each layer keeps a cache (a rounded frame)
with the key (blue) and value (purple) of every processed position, a pair
centred on its column; the prefix is "a fluffy blue creature roamed".
1: the three caches, five K, V pairs each; slot 6 is still empty.
2: "the" enters layer 1: it computes its q, k, v; k and v fill slot 6 of
   the layer-1 cache, and q (orange, right of the cache) reads all six
   pairs of that layer.
3: layer 1 is done and its query is dropped; the updated hidden vector
   h_6^(1) does the same in layer 2.
4: layer 3 likewise; its result gives the distribution of the next token.
"""
from tk import *
from kv_grid import *

BOX = 32                           # half height of a cache frame
BUS_UP = 12                        # the read line runs this far above a frame
QX = cx(5) + 54                    # q sits right of slot 6
NEXT = [("verdant", 0.38), ("forest", 0.22), ("dark", 0.12), ("quiet", 0.08)]


def pair(i, layer):
    x, y = cx(i), ROW_Y[layer]
    return (mvec(x - STEP / 2, y, f"kv-k-{layer}-{i}", K) +
            mvec(x + STEP / 2, y, f"kv-v-{layer}-{i}", V))


def empty_slot(i, layer):
    x, y = cx(i), ROW_Y[layer]
    w = STEP + MW + 10
    return [rect(x - w / 2, y - HALF - 2, w, 2 * HALF + 4, "none", LINE, rx=4, width=1.4,
                 dash="5 4")]


def query(layer):
    """q of the new position in this layer and the read of all six pairs."""
    y = ROW_Y[layer]
    top = y - HALF
    bus = y - BOX - BUS_UP
    out = mvec(QX, y, f"kv-q-{layer}", Q)
    out += math(QX, y + HALF + 24, "q_6", 20, Q)
    out.append(elbow([(cx(0), top - 1), (cx(0), bus), (QX, bus), (QX, top - 3)], Q, 1.8))
    for i in range(1, 6):
        out.append(line(cx(i), top - 1, cx(i), bus, Q, 1.6))
    return out


def dist(x, y, items, pick, bar_w=64, bar_h=22, gap=8, size=17):
    """Next-token probabilities: words right-aligned at x, bars after them."""
    out = []
    pmax = max(p for _, p in items)
    for k, (word, p) in enumerate(items):
        top = y + k * (bar_h + gap)
        on = word == pick
        c, f = (UPD, FILL_UPD) if on else (DATA, FILL_DATA)
        out.append(text(x, top + bar_h / 2 + size * 0.36, word, size, c if on else INK,
                        "bold" if on else "normal", "end"))
        bw = bar_w * p / pmax
        out.append(rect(x + 10, top, bw, bar_h, f, c, rx=2, width=1.4))
        out.append(text(x + 10 + bw + 6, top + bar_h / 2 + size * 0.36, f"{p:.0%}", size - 1,
                        MUTED, anchor="start"))
    return out


def frame(n):
    b = layer_labels()
    active = n - 1                     # layer the new token is in (0: none)
    for layer, y in ROW_Y.items():
        b.append(rect(cx(0) - 30, y - BOX, cx(5) - cx(0) + 60, 2 * BOX, FILL_GREY, LINE,
                      rx=8, width=1.4))
        for i in range(5):
            b += pair(i, layer)
        b += pair(5, layer) if layer <= active else empty_slot(5, layer)
    for i in range(5):
        b += token(i)
    if n >= 2:
        b += token(5, "upd")
        b.append(arrow(cx(5), CHIP_Y - 4, cx(5), ROW_Y[1] + BOX + 4, INK, 1.8))
        b += query(active)
    for layer in range(1, active):
        y0, y1 = ROW_Y[layer] - BOX - 2, ROW_Y[layer + 1] + BOX + 4
        b.append(arrow(cx(5), y0, cx(5), y1, INK, 1.8))
        b += math(cx(5) - 10, (y0 + y1) / 2 + 7, f"h_6^{{({layer})}}", 20, INK, anchor="end")
    if n == 4:
        y = ROW_Y[3]
        lx = QX + 122
        b.append(arrow(QX + 12, y, lx - 72, y, UPD, 1.8))
        b.append(text(lx + 40, y - 26, "下一个 token", 17, MUTED))
        b += dist(lx, y - 11, NEXT, "verdant")
    # q keeps its legend line, empty until the new token has a query
    b += legend(QKV if n >= 2 else [None] + QKV[1:])
    save(f"kv-cache-{n}", b, H)


for n in (1, 2, 3, 4):
    frame(n)
