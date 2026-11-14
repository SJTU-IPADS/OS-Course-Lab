#!/usr/bin/env python3
"""query-lifetime: queries are used once, keys and values are kept.

One layer over three decode steps t = 6, 7, 8, one row per step. In each
row the lower line is the cache of that layer, a key (blue) and a value
(purple) per position up to t; the upper line holds the queries. The query
of step t reads every pair up to and including its own. The pairs of
earlier positions are the same in every row; the queries of earlier steps
(dashed) are no longer held.
"""
from tk import *

H = 384
PITCH, X0 = 92, 192
STEPS = (6, 7, 8)
ROW_Q = {6: 58, 7: 162, 8: 266}    # centre of each row's query line
DROP = 50                          # query line to cache line
CELLS, CH, MW = 3, 10, 13


def cx(pos):
    return X0 + (pos - 1) * PITCH


def tint(color_hex, t):
    base = tuple(int(color_hex[k:k + 2], 16) for k in (1, 3, 5))
    r, g, b_ = (round(255 + (c - 255) * t) for c in base)
    return f"#{r:02x}{g:02x}{b_:02x}"


def mvec(x, y, seed, color, dash=None):
    top, left = y - CELLS * CH / 2, x - MW / 2
    if dash:
        return [rect(left - 2, top - 2, MW + 4, CELLS * CH + 4, "none", LINE, rx=3, width=1.4,
                     dash=dash)]
    out = [rect(left - 2, top - 2, MW + 4, CELLS * CH + 4, WHITE, color, rx=3, width=1.6)]
    for k in range(CELLS):
        t = 0.15 + 0.6 * abs(pseudo(seed, k, 0))
        out.append(rect(left, top + k * CH, MW, CH, tint(color, t), WHITE, rx=0, width=1))
    return out


def frame():
    b = []
    half = CELLS * CH / 2 + 2
    for pos in range(1, 9):
        b.append(text(cx(pos), 18, str(pos), 17, MUTED))
    b.append(text(cx(1) - 50, 18, "位置", 17, MUTED, anchor="end"))
    for t in STEPS:
        yq = ROW_Q[t]
        yp = yq + DROP
        b.append(text(cx(1) - 50, (yq + yp) / 2 + 7, f"第 {t} 步", 19, INK, "bold",
                      anchor="end"))
        for pos in range(1, t + 1):
            b += mvec(cx(pos) - 8, yp, f"ql-k-{pos}", K)
            b += mvec(cx(pos) + 8, yp, f"ql-v-{pos}", V)
        for pos in range(STEPS[0], t):
            b += mvec(cx(pos), yq, "", Q, dash="4 3")
        b += mvec(cx(t), yq, f"ql-q-{t}", Q)
        b += math(cx(t) + 14, yq + 7, f"q_{t}", 20, Q, anchor="start")
        # the read: every pair up to t, gathered into q_t
        bus = yp - half - 12
        b.append(elbow([(cx(1), yp - half), (cx(1), bus), (cx(t), bus), (cx(t), yq + half + 3)],
                       Q, 1.8))
        for pos in range(2, t + 1):
            b.append(line(cx(pos), yp - half, cx(pos), bus, Q, 1.6))
        b.append(text(cx(8) + 46, yp + 6, f"读取 {t} 组", 18, INK, anchor="start"))
    # one legend line under the rows
    ly = ROW_Q[8] + DROP + 54
    x = cx(1) - 10
    for sym, name, c, f, dash in [("k", "键", K, FILL_K, None), ("v", "值", V, FILL_V, None),
                                  ("q", "查询", Q, FILL_Q, None),
                                  ("", "已丢弃的查询", LINE, WHITE, "4 3")]:
        b.append(rect(x, ly - 15, 16, 18, f, c, rx=3, width=1.6, dash=dash))
        if sym:
            b += math(x + 24, ly, sym, 20, c, anchor="start")
            b.append(text(x + 38, ly, name, 18, c, anchor="start"))
            x += 38 + width(name, 18) + 30
        else:
            b.append(text(x + 24, ly, name, 18, MUTED, anchor="start"))
    save("query-lifetime", b, H)


if __name__ == "__main__":
    frame()
