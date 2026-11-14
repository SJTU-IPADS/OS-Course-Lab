#!/usr/bin/env python3
"""kv-cache: one decode step layer by layer, with a K, V cache per layer.

Each layer keeps a cache (a rounded frame) with the key (blue) and value
(purple) of every processed position; the prefix is "a fluffy blue
creature roamed".
1: the three caches, five K, V pairs each; slot 6 is still empty.
2: "the" enters layer 1: it computes its q, k, v; k and v fill slot 6 of
   the layer-1 cache, and q reads all six pairs of that layer.
3: layer 1 is done and its query is dropped; the updated hidden vector
   h_6^(1) does the same in layer 2.
4: layer 3 likewise; its result gives the distribution of the next token.
"""
from tk import *

H = 384
WORDS = "a fluffy blue creature roamed the".split()
PITCH, X0 = 108, 176
ROW_Y = {1: 274, 2: 170, 3: 66}
CELLS, CH, MW = 4, 11, 13          # a k or v vector: 4 cells, 13 px wide
CHIP_Y = 336
QX_OFF = 54                        # q sits right of slot 6
NEXT = [("verdant", 0.38), ("forest", 0.22), ("dark", 0.12), ("quiet", 0.08)]


def cx(i):
    return X0 + i * PITCH


def tint(color_hex, t):
    base = tuple(int(color_hex[k:k + 2], 16) for k in (1, 3, 5))
    r, g, b_ = (round(255 + (c - 255) * t) for c in base)
    return f"#{r:02x}{g:02x}{b_:02x}"


def mvec(x, y, seed, color):
    """A small column vector centred at (x, y)."""
    top, left = y - CELLS * CH / 2, x - MW / 2
    out = [rect(left - 2, top - 2, MW + 4, CELLS * CH + 4, WHITE, color, rx=3, width=1.6)]
    for k in range(CELLS):
        t = 0.15 + 0.6 * abs(pseudo(seed, k, 0))
        out.append(rect(left, top + k * CH, MW, CH, tint(color, t), WHITE, rx=0, width=1))
    return out


def pair(i, layer):
    x, y = cx(i), ROW_Y[layer]
    return (mvec(x - 9, y, f"kv-k-{layer}-{i}", K) +
            mvec(x + 9, y, f"kv-v-{layer}-{i}", V))


def empty_slot(i, layer):
    x, y = cx(i), ROW_Y[layer]
    return [rect(x - 20, y - CELLS * CH / 2 - 4, 40, CELLS * CH + 8, "none", LINE, rx=4,
                 width=1.4, dash="5 4")]


def query(layer):
    """q of the new position in this layer and the read of all six pairs."""
    y = ROW_Y[layer]
    qx = cx(5) + QX_OFF
    top = y - CELLS * CH / 2 - 2
    bus = y - 32 - 12
    out = mvec(qx, y, f"kv-q-{layer}", Q)
    out += math(qx, y + CELLS * CH / 2 + 26, "q_6", 20, Q)
    out.append(elbow([(cx(0), top - 1), (cx(0), bus), (qx, bus), (qx, top - 3)], Q, 1.8))
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


def legend(x, y, items):
    """One entry per line, downwards from y."""
    out = []
    for sym, name, c, f in items:
        out.append(rect(x, y - 15, 16, 18, f, c, rx=3, width=1.6))
        out += math(x + 24, y, sym, 20, c, anchor="start")
        out.append(text(x + 38, y, name, 18, c, anchor="start"))
        y += 32
    return out


def frame(n):
    b = []
    active = n - 1                     # layer the new token is in (0: none)
    for layer, y in ROW_Y.items():
        b.append(text(cx(0) - 46, y + 7, f"第 {layer} 层", 19, INK, anchor="end"))
        b.append(rect(cx(0) - 30, y - 32, cx(5) - cx(0) + 60, 64, FILL_GREY, LINE, rx=8,
                      width=1.4))
        for i in range(5):
            b += pair(i, layer)
        b += pair(5, layer) if layer <= active else empty_slot(5, layer)
    for i, word in enumerate(WORDS[:5]):
        els, _ = chip(cx(i), CHIP_Y, word, "data", size=18, h=34, anchor="middle")
        b += els
    items = [("k", "键", K, FILL_K), ("v", "值", V, FILL_V)]
    if n >= 2:
        els, _ = chip(cx(5), CHIP_Y, WORDS[5], "upd", size=18, h=34, anchor="middle")
        b += els
        b.append(arrow(cx(5), CHIP_Y - 4, cx(5), ROW_Y[1] + 36, INK, 1.8))
        b += query(active)
        items.append(("q", "查询", Q, FILL_Q))
    for layer in range(1, active):
        y0, y1 = ROW_Y[layer] - 34, ROW_Y[layer + 1] + 36
        b.append(arrow(cx(5), y0, cx(5), y1, INK, 1.8))
        b += math(cx(5) - 10, (y0 + y1) / 2 + 7, f"h_6^{{({layer})}}", 20, INK, anchor="end")
    if n == 4:
        qx = cx(5) + QX_OFF
        y = ROW_Y[3]
        lx = qx + 152
        b.append(arrow(qx + 12, y, lx - 82, y, UPD, 1.8))
        b.append(text(lx + 40, y - 26, "下一个 token", 17, MUTED))
        b += dist(lx, y - 11, NEXT, "verdant")
    b += legend(cx(5) + 104, ROW_Y[1] - 26, items)
    save(f"kv-cache-{n}", b, H)


for n in (1, 2, 3, 4):
    frame(n)
