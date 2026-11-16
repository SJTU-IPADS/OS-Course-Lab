#!/usr/bin/env python3
"""query: the question a noun asks, and the vector that encodes it.

1: creature asks "我前面有形容词吗？" (orange); fluffy and blue answer
   "我是形容词，在这个位置" (blue, the colour of the keys to come).
2: q_4 = W_Q E_4: the weight grid W_Q (128 × 12,288, drawn wide and short)
   maps the tall grey E_4 (12,288) to the short orange q_4 (128). Each of the
   three is labelled with its symbol, its name and its size.
3: the same W_Q applied to every E_i gives q_1 .. q_8.
"""
from tk import *
from attn_data import WORDS

H = 380
CS, GAP = 19, 14
CHIP_Y = 70
X0 = (W - row_width(WORDS, CS, GAP)) / 2


def mix(c1, c2, t):
    a = [int(c1[i:i + 2], 16) for i in (1, 3, 5)]
    b_ = [int(c2[i:i + 2], 16) for i in (1, 3, 5)]
    return "#%02x%02x%02x" % tuple(round(x + (y - x) * t) for x, y in zip(a, b_))


def col(x, y, n, seed, role="data", cw=28, ch=14, dots=True, size=15):
    """A vector as a column of n shaded cells, ⋮ in the cell before last."""
    stroke, fill = ROLE[role]
    out = []
    for i in range(n):
        cy = y + i * ch
        if dots and i == n - 2:
            out.append(rect(x, cy, cw, ch, WHITE, WHITE, rx=0, width=1))
            out.append(text(x + cw / 2, cy + ch / 2 + size * 0.33, "⋮", size, stroke))
        else:
            t = 0.12 + 0.5 * (pseudo(seed, i, 0) + 1) / 2
            out.append(rect(x, cy, cw, ch, mix(fill, stroke, t), WHITE, rx=0, width=1))
    out.append(rect(x - 3, y - 3, cw + 6, n * ch + 6, "none", stroke, rx=4, width=1.8))
    return out


def bubble(cx, top, h, s, role, size, w=None, tail_to=None, left=None):
    """A speech bubble whose tail points at (cx, tail_to)."""
    stroke, fill = ROLE[role]
    w = w or width(s, size) + 28
    left = cx - w / 2 if left is None else left
    out = []
    edge = top + h if tail_to > top else top
    out.append(f'<path d="M {cx - 8:.1f} {edge:.1f} L {cx:.1f} {tail_to:.1f} '
               f'L {cx + 8:.1f} {edge:.1f} Z" fill="{fill}" stroke="{stroke}" '
               f'stroke-width="1.6"/>')
    out.append(rect(left, top, w, h, fill, stroke, rx=10, width=1.6))
    out.append(rect(cx - 7, edge - 2, 14, 4, fill, "none", rx=0, width=0))
    out.append(text(left + w / 2, top + h / 2 + size * 0.36, s, size, stroke))
    return out


def frame(n):
    b = []
    roles = ["data"] * len(WORDS)
    if n == 2:
        roles[3] = "hl"
    els, spans = chip_row(X0, CHIP_Y, WORDS, roles, size=CS, gap=GAP)
    b += els
    cs = [c for _, c, _ in spans]

    if n == 1:
        b += bubble(cs[3], 10, 42, "我前面有形容词吗？", "q", 19, tail_to=CHIP_Y - 2)
        ans = "我是形容词，在这个位置"
        aw = width(ans, 18) + 28
        b += bubble(cs[1], 134, 42, ans, "k", 18, w=aw, tail_to=CHIP_Y + 38,
                    left=cs[1] + 24 - aw)
        b += bubble(cs[2], 134, 42, ans, "k", 18, w=aw, tail_to=CHIP_Y + 38,
                    left=cs[2] - 24)

    if n == 2:
        top, ech, en = 132, 18, 13                      # E_4: tall
        ey0 = top
        mid = ey0 + en * ech / 2
        qh = 4 * 16
        qx, mx, ex = 170, 252, 720
        b += col(qx, mid - qh / 2, 4, "q4", "q", cw=28, ch=16, dots=False)
        b.append(text(qx + 62, mid + 12, "=", 34, INK))
        b += weights(mx, mid - qh / 2, 4, 28, 16, 16, seed="WQ")
        b += col(ex, ey0, en, "creature", "data", cw=28, ch=ech)
        b += math(qx + 14, mid + qh / 2 + 32, "q_4", 22, Q)
        b.append(text(qx + 14, mid + qh / 2 + 56, "查询的问题", 17, Q))
        b.append(text(qx + 14, mid + qh / 2 + 80, "128 维", 17, MUTED))
        b += math(mx + 28 * 8, mid - qh / 2 - 16, "W_Q", 24, INK)
        b.append(text(mx + 28 * 8, mid + qh / 2 + 30, "查询矩阵", 17, INK))
        b.append(text(mx + 28 * 8, mid + qh / 2 + 54, "128 × 12,288", 18, MUTED))
        b += math(ex + 44, mid, "E_4", 22, DATA, anchor="start")
        b.append(text(ex + 44, mid + 26, "当前 token", 17, DATA, anchor="start"))
        b.append(text(ex + 44, mid + 50, "12,288 维", 17, MUTED, anchor="start"))

    if n == 3:
        ey, eh = 138, 8 * 14
        qy = ey + eh + 50
        for i, c in enumerate(cs):
            b += math(c, ey - 12, f"E_{i + 1}", 19, DATA)
            b += col(c - 14, ey, 8, WORDS[i])
            b.append(arrow(c, ey + eh + 8, c, qy - 8, Q, 2))
            b += math(c + 6, ey + eh + 31, "W_Q", 17, INK, anchor="start")
            b += col(c - 14, qy, 3, "q" + WORDS[i], "q", dots=False)
            b += math(c, qy + 3 * 14 + 26, f"q_{i + 1}", 19, Q)
    save(f"query-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
