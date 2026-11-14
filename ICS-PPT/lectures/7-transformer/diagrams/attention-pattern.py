#!/usr/bin/env python3
"""attention-pattern: from raw scores to an attention pattern (no mask yet).

Keys run down the left, queries across the top, as in attention-scores.
1: the creature column of the dot-product grid, pulled out: its raw scores
   SCORES[k][3] can be any real number.
2: softmax down that column turns them into weights in [0, 1] that sum to 1
   (attn_data.column(3, causal=False)).
3: every column normalised the same way (pattern(causal=False)): the
   attention pattern, each cell shaded by its weight.
Weights are shown to two decimals, rounded so that every column still adds
up to 1.00 (largest remainder).
"""
from tk import *
from attn_data import WORDS, N, SCORES, CREATURE, column, pattern

H = 430
CELL = 40
GX, GY = 190, 92
RX = GX + N * CELL + 56          # the column pulled out of the grid
CWX = 66                         # its cell width
AX0 = RX + CWX + 112             # the softmax arrow
SX = AX0 + 92                    # the softmax result


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
    """Fill and text colour of a cell holding weight h/100."""
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


def bracket(x, label, color):
    y0, y1 = GY + 2, GY + N * CELL - 2
    return [f'<path d="M {x:.1f} {y0:.1f} L {x + 8:.1f} {y0:.1f} L {x + 8:.1f} {y1:.1f} '
            f'L {x:.1f} {y1:.1f}" fill="none" stroke="{color}" stroke-width="1.6"/>',
            text(x + 18, (y0 + y1) / 2 + 7, label, 19, color, "bold", anchor="start")]


def frame(n):
    b = labels()
    if n < 3:
        fills = [[FILL_ORANGE if j == CREATURE else WHITE for j in range(N)] for i in range(N)]
        b += grid(GX, GY, N, N, CELL, fills=fills)
        b += dots_in_grid(GX, GY, SCORES, CELL, color=MUTED)
        for i in (1, 2):
            cx, cy = GX + (CREATURE + .5) * CELL, GY + (i + .5) * CELL
            b.append(circle(cx, cy, CELL * 0.42 * (SCORES[i][CREATURE] / 4.6) ** .5, ORANGE,
                            ORANGE, 1.2))
        # the creature column, its raw scores
        for i in range(N):
            v = SCORES[i][CREATURE]
            b.append(rect(RX, GY + i * CELL, CWX, CELL, WHITE, LINE, rx=0, width=1))
            b.append(text(RX + CWX / 2, GY + (i + .5) * CELL + 6, f"{v:+.2f}".replace("-", "−"),
                          17, RED if v < 0 else INK, font=MONO))
        b.append(rect(RX - 3, GY - 3, CWX + 6, N * CELL + 6, "none", ORANGE, rx=4, width=2))
        b += math(RX + CWX / 2, GY - 12, "k_i · q_4", 19, INK)
        b += bracket(RX + CWX + 10, "任意实数", INK)
    if n == 2:
        b.append(arrow(AX0, GY + 4 * CELL, SX - 14, GY + 4 * CELL, INK, 2.2))
        b.append(text((AX0 + SX - 14) / 2, GY + 4 * CELL - 12, "softmax", 19, INK))
        hs = round_col(column(CREATURE, causal=False))
        for i, h in enumerate(hs):
            f, tc = shade(h)
            b.append(rect(SX, GY + i * CELL, CWX, CELL, f, LINE, rx=0, width=1))
            b.append(text(SX + CWX / 2, GY + (i + .5) * CELL + 6, f"{h / 100:.2f}", 17, tc,
                          font=MONO))
        b.append(rect(SX - 3, GY - 3, CWX + 6, N * CELL + 6, "none", ORANGE, rx=4, width=2))
        b.append(text(SX + CWX / 2, GY - 12, "权重", 18, INK))
        b += bracket(SX + CWX + 10, "和为 1", GREEN)
    if n == 3:
        cols = [round_col(column(q, causal=False)) for q in range(N)]
        fills = [[shade(cols[j][i])[0] for j in range(N)] for i in range(N)]
        b += grid(GX, GY, N, N, CELL, fills=fills)
        for i in range(N):
            for j in range(N):
                h = cols[j][i]
                b.append(text(GX + (j + .5) * CELL, GY + (i + .5) * CELL + 5.5,
                              f"{h / 100:.2f}", 15, shade(h)[1]))
        b.append(rect(GX + CREATURE * CELL, GY, CELL, N * CELL, "none", ORANGE, rx=0, width=2.4))
        tx = GX + N * CELL + 60
        b.append(text(tx, GY + N * CELL / 2 + 7, "注意力模式：每列和为 1", 20, INK, "bold",
                      anchor="start"))
    save(f"attention-pattern-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
