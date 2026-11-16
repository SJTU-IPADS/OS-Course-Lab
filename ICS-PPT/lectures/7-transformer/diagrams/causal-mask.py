#!/usr/bin/env python3
"""causal-mask: later words must not influence earlier ones.

Keys run down the left, queries across the top, as in attention-scores. The
cells where the key comes after the query (row > column) are red. Masking:
their scores are set to −∞ before softmax (pattern(causal=True)); the red
cells come out 0 and every column still sums to 1.
Weights are shown to two decimals, rounded so that each column of the
softmax output adds up to 1.00 (largest remainder).
"""
from tk import *
from attn_data import WORDS, N, column

H = 446
CELL = 40
GX, GY = 330, 92


def mix(c1, c2, t):
    a = [int(c1[i:i + 2], 16) for i in (1, 3, 5)]
    b_ = [int(c2[i:i + 2], 16) for i in (1, 3, 5)]
    return "#%02x%02x%02x" % tuple(round(x + (y - x) * t) for x, y in zip(a, b_))


def round_col(ws):
    """Hundredths that add up to exactly 100 (largest remainder)."""
    raw = [w * 100 for w in ws]
    out = [int(r) for r in raw]
    for i in sorted(range(len(ws)), key=lambda i: raw[i] - out[i], reverse=True)[:100 - sum(out)]:
        out[i] += 1
    return out


def shade(h):
    return mix(WHITE, DATA, 0.08 + 0.85 * h / 100), (WHITE if h >= 50 else INK)


def labels():
    b = []
    for j, w in enumerate(WORDS):
        cx = GX + (j + .5) * CELL
        b.append(f'<text x="{cx:.1f}" y="{GY - 40:.1f}" font-family="{FONT}" font-size="18" '
                 f'fill="{INK}" text-anchor="start" transform="rotate(-35 {cx:.1f} {GY - 40:.1f})">'
                 f'{w}</text>')
        b += math(cx, GY - 12, f"q_{j + 1}", 19, Q)
    for i, w in enumerate(WORDS):
        cy = GY + (i + .5) * CELL + 7
        b.append(text(GX - 52, cy, w, 18, INK, anchor="end"))
        b += math(GX - 24, cy, f"k_{i + 1}", 19, K)
    return b


def figure():
    b = labels()
    masked = [[i > j for j in range(N)] for i in range(N)]
    cols = [round_col(column(q, causal=True)) for q in range(N)]
    fills = [[FILL_RED if masked[i][j] else shade(cols[j][i])[0] for j in range(N)]
             for i in range(N)]
    b += grid(GX, GY, N, N, CELL, fills=fills)
    for i in range(N):
        for j in range(N):
            y = GY + (i + .5) * CELL + 5.5
            x = GX + (j + .5) * CELL
            if masked[i][j]:
                b.append(text(x, y, "0", 15, RED))
            else:
                h = cols[j][i]
                b.append(text(x, y, f"{h / 100:.2f}", 15, shade(h)[1]))
    # column sums
    sy = GY + N * CELL + 26
    b.append(text(GX - 10, sy, "列和", 17, MUTED, anchor="end"))
    for j in range(N):
        s = sum(cols[j][i] for i in range(N) if not masked[i][j])
        c = GREEN if s == 100 else RED
        b.append(text(GX + (j + .5) * CELL, sy, f"{s / 100:.2f}", 15, c, "bold"))

    tx = GX + N * CELL + 50
    b.append(rect(tx, GY + 24, 26, 26, FILL_RED, LINE, rx=0, width=1))
    b.append(text(tx + 38, GY + 44, "键在查询之后", 18, INK, anchor="start"))
    b.append(text(tx, GY + 104, "softmax 之前，", 19, INK, anchor="start"))
    b.append(text(tx, GY + 134, "把红色格子的分数设为 −∞", 19, INK, anchor="start"))
    b += math(tx, GY + 184, "e^{−∞} = 0", 22, INK, anchor="start")
    b.append(text(tx, GY + 222, "各列的和仍为 1", 19, GREEN, "bold", anchor="start"))
    save("causal-mask", b, H)


figure()
