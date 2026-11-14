#!/usr/bin/env python3
"""multi-head: many heads, each with its own pattern, each proposing a change.

1: heads 1, 2, ..., 96, each its own attention grid (keys rows, queries
   columns, the causal mask in grey) over the first words of the video's
   sentence "John hit the brakes sharply, they screeched loudly, and he jolted
   forward.", with the video's head types and the head's own W_Q, W_K. Under
   the mask a later word attends to an earlier one, so the adverb head shows
   sharply (query) attending to hit (key).
2: for one position (creature), every head proposes a change
   Delta E^(1), Delta E^(2), ..., Delta E^(96) (green columns).
3: their sum is added to E_4: E'_4 = E_4 + sum_h Delta E^(h).
"""
from tk import *

H = 420
WORDS = ["John", "hit", "the", "brakes", "sharply", "they"]
N = len(WORDS)
C = 28                      # cell of the small grids
GY = 70
HEADS = [  # (number, type, (key, query) of the pair it links)
    (1, "副词 → 动词", (1, 4)),
    (2, "主语 → 动词", (0, 1)),
    (96, "先行词 → 代词", (3, 5)),
]
GXS = [104, 364, 660]


def weights_of(pair):
    """W[k][q]: the pair gets 0.8 of its column; other columns mostly look at
    themselves; nothing above the diagonal (the mask)."""
    out = [[0.0] * N for _ in range(N)]
    for q in range(N):
        if (q == pair[1]):
            out[pair[0]][q] = 0.8
            rest = [k for k in range(q + 1) if k != pair[0]]
            for k in rest:
                out[k][q] = 0.2 / len(rest)
        else:
            out[q][q] = 0.45 if q else 1.0
            for k in range(q):
                out[k][q] = 0.55 / q
    return out


def head(gx, number, kind, pair):
    b = []
    for j, w in enumerate(WORDS):
        cx = gx + (j + .5) * C
        b.append(f'<text x="{cx - 2:.1f}" y="{GY - 8:.1f}" font-family="{FONT}" font-size="16" '
                 f'fill="{ORANGE if j == pair[1] else INK}" text-anchor="start" '
                 f'transform="rotate(-55 {cx - 2:.1f} {GY - 8:.1f})">{w}</text>')
    for i, w in enumerate(WORDS):
        b.append(text(gx - 8, GY + (i + .5) * C + 6, w, 16, BLUE if i == pair[0] else INK,
                      anchor="end"))
    fills = [[FILL_GREY if k > q else WHITE for q in range(N)] for k in range(N)]
    b += grid(gx, GY, N, N, C, fills=fills)
    wt = weights_of(pair)
    for k in range(N):
        for q in range(N):
            if k > q:
                continue
            on = (k, q) == pair
            b.append(circle(gx + (q + .5) * C, GY + (k + .5) * C, max(1.5, C * 0.44 * wt[k][q]),
                            ORANGE if on else INK, ORANGE if on else INK, 1))
    mid = gx + N * C / 2
    wq, wk = f"W_Q^{{({number})}}", f"W_K^{{({number})}}"
    head_w = width(f"头 {number}", 20)
    total = head_w + 12 + math_width(wq, 20) + 10 + math_width(wk, 20)
    x = mid - total / 2
    b.append(text(x, GY + N * C + 28, f"头 {number}", 20, INK, "bold", anchor="start"))
    x += head_w + 12
    b += math(x, GY + N * C + 28, wq, 20, Q, anchor="start")
    x += math_width(wq, 20) + 10
    b += math(x, GY + N * C + 28, wk, 20, K, anchor="start")
    b.append(text(mid, GY + N * C + 54, kind, 18, INK))
    return b


def column(x, y, role, cells=5, cw=18, ch=13):
    stroke, fill = ROLE[role]
    out = [rect(x, y, cw, cells * ch, fill, stroke, rx=2, width=1.6)]
    for s in range(1, cells):
        out.append(line(x + 2, y + s * ch, x + cw - 2, y + s * ch, stroke, 0.8))
    return out


def frame(n):
    b = []
    for gx, (number, kind, pair) in zip(GXS, HEADS):
        b += head(gx, number, kind, pair)
    b.append(text((GXS[1] + N * C + GXS[2] - 70) / 2, GY + N * C / 2 + 8, "⋯", 30, MUTED))

    ty = GY + N * C + 96                    # top of the bottom row of vectors
    cy = ty + 38                            # its middle line
    if n >= 2:
        b.append(text(40, ty - 8, "creature 的位置", 17, MUTED, anchor="start"))
        b += column(50, ty + 6, "data")
        b += math(74, cy + 6, "E_4", 20, DATA, anchor="start")
        for gx, (number, _, _) in zip(GXS, HEADS):
            mid = gx + N * C / 2
            b.append(arrow(mid, GY + N * C + 64, mid, ty + 2, UPD, 1.6))
            b += column(mid - 9, ty + 6, "upd")
            b += math(mid + 15, cy + 6, f"ΔE^{{({number})}}", 20, UPD, anchor="start")
    if n >= 3:
        m0, m1, m2 = (gx + N * C / 2 for gx in GXS)
        b.append(label((110 + m0 - 9) / 2, cy + 9, "+", 26))
        b.append(label((m0 + 70 + m1 - 9) / 2, cy + 9, "+", 26))
        b.append(label((m1 + 70 + m2 - 9) / 2, cy + 9, "+ ⋯ +", 24))
        ex = m2 + 132
        b.append(label(ex - 26, cy + 9, "=", 26))
        b += column(ex, ty + 6, "upd")
        b += math(ex + 24, cy + 6, "E′_4", 20, UPD, anchor="start")
        b += math(W - 20, ty - 26, "E′_4 = E_4 + Σ_h ΔE^{(h)}", 20, UPD, anchor="end")
    save(f"multi-head-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
