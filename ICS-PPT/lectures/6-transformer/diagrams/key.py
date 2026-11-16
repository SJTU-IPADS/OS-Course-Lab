#!/usr/bin/env python3
"""key: the answer an adjective gives, and the vector that encodes it.

Frames 1 and 2 have the form of query-1 and query-2 (same token row, same
bubbles, same sizes and positions), so the two pages read side by side.

1: the context of query-1, with the answer in focus: fluffy and blue are
   highlighted in blue (keys) under their bubbles "我是形容词，在这个位置";
   creature's orange question stays as it was.
2: k_2 = W_K E_2: the weight grid W_K (128 × 12,288, drawn wide and short,
   cells from another seed than W_Q) maps the tall grey E_2 (12,288) to the
   short blue k_2 (128). Each of the three is labelled with its symbol, its
   name and its size.
3: from the same embeddings E_1 .. E_8, W_Q gives the queries q_i (orange)
   and W_K the keys k_i (blue, the row below).
4: the 128-dimensional query/key space drawn in 2D: the keys of fluffy and
   blue point close to the query of creature, those of the and roamed point
   elsewhere. The arrows have equal length and their angles follow the dot
   products of attn_data (SCORES[k][3] = 5 cos θ), listed on the right.
"""
import math as _m

from tk import *
from attn_data import WORDS, SCORES, CREATURE

H = 380
CS, GAP = 19, 14
CHIP_Y = 70                                  # frames 1, 2: as in query.py
X0 = (W - row_width(WORDS, CS, GAP)) / 2
ROW_Y, ROW_X0 = 10, X0 + 60                  # frame 3: room for the labels


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


def frame1():
    b = []
    roles = ["data"] * len(WORDS)
    roles[1] = roles[2] = "k"
    els, spans = chip_row(X0, CHIP_Y, WORDS, roles, size=CS, gap=GAP)
    b += els
    cs = [c for _, c, _ in spans]
    b += bubble(cs[3], 10, 42, "我前面有形容词吗？", "q", 19, tail_to=CHIP_Y - 2)
    ans = "我是形容词，在这个位置"
    aw = width(ans, 18) + 28
    b += bubble(cs[1], 134, 42, ans, "k", 18, w=aw, tail_to=CHIP_Y + 38,
                left=cs[1] + 24 - aw)
    b += bubble(cs[2], 134, 42, ans, "k", 18, w=aw, tail_to=CHIP_Y + 38,
                left=cs[2] - 24)
    save("key-1", b, H)


def frame2():
    b = []
    roles = ["data"] * len(WORDS)
    roles[1] = "k"
    els, spans = chip_row(X0, CHIP_Y, WORDS, roles, size=CS, gap=GAP)
    b += els
    top, ech, en = 132, 18, 13                      # E_2: tall
    ey0 = top
    mid = ey0 + en * ech / 2
    kh = 4 * 16
    kx, mx, ex = 170, 252, 720
    b += col(kx, mid - kh / 2, 4, "k2", "k", cw=28, ch=16, dots=False)
    b.append(text(kx + 62, mid + 12, "=", 34, INK))
    b += weights(mx, mid - kh / 2, 4, 28, 16, 16, seed="WK")
    b += col(ex, ey0, en, "fluffy", "data", cw=28, ch=ech)
    b += math(kx + 14, mid + kh / 2 + 32, "k_2", 22, K)
    b.append(text(kx + 14, mid + kh / 2 + 56, "对问题的回答", 17, K))
    b.append(text(kx + 14, mid + kh / 2 + 80, "128 维", 17, MUTED))
    b += math(mx + 28 * 8, mid - kh / 2 - 16, "W_K", 24, INK)
    b.append(text(mx + 28 * 8, mid + kh / 2 + 30, "键矩阵", 17, INK))
    b.append(text(mx + 28 * 8, mid + kh / 2 + 54, "128 × 12,288", 18, MUTED))
    b += math(ex + 44, mid, "E_2", 22, DATA, anchor="start")
    b.append(text(ex + 44, mid + 26, "当前 token", 17, DATA, anchor="start"))
    b.append(text(ex + 44, mid + 50, "12,288 维", 17, MUTED, anchor="start"))
    save("key-2", b, H)


def frame3():
    b = []
    els, spans = chip_row(ROW_X0, ROW_Y, WORDS, size=CS, gap=GAP)
    b += els
    ey, eh = 76, 8 * 14
    qy = ey + eh + 38
    ky = qy + 3 * 14 + 40
    for i, (_, c, _) in enumerate(spans):
        b += math(c, ey - 12, f"E_{i + 1}", 19, DATA)
        b += col(c - 14, ey, 8, WORDS[i])
        qc, kc = c - 9, c + 11
        b.append(arrow(qc, ey + eh + 6, qc, qy - 7, Q, 2))
        b += col(qc - 9, qy, 3, "q" + WORDS[i], "q", cw=18, dots=False)
        b += math(qc, qy + 42 + 22, f"q_{i + 1}", 18, Q)
        b.append(arrow(kc, ey + eh + 6, kc, ky - 7, K, 2))
        b += col(kc - 9, ky, 3, "k" + WORDS[i], "k", cw=18, dots=False)
        b += math(kc, ky + 42 + 22, f"k_{i + 1}", 18, K)
    lx = ROW_X0 - 30
    b += math(lx, ey + eh / 2 + 7, "E_i", 21, DATA, anchor="end")
    b += math(lx, qy + 28, "q_i = W_Q E_i", 21, Q, anchor="end")
    b += math(lx, ky + 28, "k_i = W_K E_i", 21, K, anchor="end")
    save("key-3", b, H)


def frame4():
    b = []
    ox, oy, half = 400, 204, 160               # origin and half side of the plane
    for k in range(-4, 5):
        g = k * half / 4
        c = LINE if k == 0 else "#dbe4ea"
        b.append(line(ox + g, oy - half, ox + g, oy + half, c, 1.4 if k == 0 else 1))
        b.append(line(ox - half, oy + g, ox + half, oy + g, c, 1.4 if k == 0 else 1))
    b.append(text(ox - half, oy - half - 12, "查询/键空间（128 维）", 17, MUTED, anchor="start"))

    r, qa = 132, 205.0                          # arrow length, angle of q_4
    side = {1: -1, 2: 1, 4: -1, 5: 1}           # which side of q_4 each key lies on

    def tip(deg, ln=r):
        a = _m.radians(deg)
        return ox + ln * _m.cos(a), oy - ln * _m.sin(a)

    place = {1: (-12, 6, "end"), 2: (8, 22, "start"), 4: (10, -4, "start"),
             5: (10, 6, "start")}
    for k in (1, 2, 4, 5):
        th = _m.degrees(_m.acos(SCORES[k][CREATURE] / 5.0))
        deg = qa + side[k] * th
        x, y = tip(deg)
        b.append(vec(ox, oy, x, y, K, 2.4, shrink=0))
        dx, dy, anchor = place[k]
        b += math(x + dx, y + dy, f"k_{k + 1} (\\up{{{WORDS[k]}}})", 19, K, anchor=anchor)
    x, y = tip(qa)
    b.append(vec(ox, oy, x, y, Q, 3, shrink=0))
    b += math(x - 12, y + 26, f"q_4 (\\up{{creature}})", 19, Q, anchor="end")

    tx = ox + half + 80
    b.append(text(tx, 108, "方向接近，点积大", 20, INK, "bold", anchor="start"))
    for row, k in enumerate((1, 2, 4, 5)):
        y = 156 + row * 36
        b += math(tx, y, f"k_{k + 1} · q_4", 20, INK, anchor="start")
        v = SCORES[k][CREATURE]
        b.append(text(tx + 150, y, f"{v:+.2f}".replace("-", "−"), 19,
                      INK if v > 0 else RED, anchor="end", font=MONO))
        b.append(text(tx + 168, y, WORDS[k], 18, MUTED, anchor="start"))
    save("key-4", b, H)


frame1()
frame2()
frame3()
frame4()
