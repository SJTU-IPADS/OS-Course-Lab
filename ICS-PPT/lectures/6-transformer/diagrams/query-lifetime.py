#!/usr/bin/env python3
"""query-lifetime: queries are used once, keys and values are kept.

One layer over three decode steps t = 6, 7, 8, one row per step. Positions
1..6 stand on the columns of kv_grid.py, so the pairs of kv-cache.py stay
in place; positions 7 and 8 continue the grid to the right. In each row the
lower line is the cache of that layer, a key (blue) and a value (purple)
per position up to t; the upper line holds the queries. The query of step
t (orange) reads every pair up to and including its own. The pairs of
earlier positions are the same in every row; the queries of earlier steps
(dashed) are no longer held. The labels on the left name the step and the
number of pairs it reads.
"""
from tk import *
from kv_grid import H, LABEL_X, STEP, cx as grid_x, mvec

STEPS = (6, 7, 8)
ROW_Q = {6: 58, 7: 162, 8: 266}    # centre of each row's query line
DROP = 50                          # query line to cache line
CELLS, CH = 3, 10                  # a vector of this figure: 3 cells


def cx(pos):
    return grid_x(pos - 1)


def vec(x, y, seed, color, dash=None):
    return mvec(x, y, seed, color, cells=CELLS, ch=CH, dash=dash)


def frame():
    b = []
    half = CELLS * CH / 2 + 2
    for pos in range(1, 9):
        b.append(text(cx(pos), 18, str(pos), 17, MUTED))
    b.append(text(LABEL_X, 18, "位置", 17, MUTED, anchor="end"))
    for t in STEPS:
        yq = ROW_Q[t]
        yp = yq + DROP
        mid = (yq + yp) / 2
        b.append(text(LABEL_X, mid - 4, f"第 {t} 步", 19, INK, "bold", anchor="end"))
        b.append(text(LABEL_X, mid + 21, f"读取 {t} 组", 17, MUTED, anchor="end"))
        for pos in range(1, t + 1):
            b += vec(cx(pos) - STEP / 2, yp, f"ql-k-{pos}", K)
            b += vec(cx(pos) + STEP / 2, yp, f"ql-v-{pos}", V)
        for pos in range(STEPS[0], t):
            b += vec(cx(pos), yq, "", Q, dash="4 3")
        b += vec(cx(t), yq, f"ql-q-{t}", Q)
        b += math(cx(t) + 14, yq + 7, f"q_{t}", 20, Q, anchor="start")
        # the read: every pair up to t, gathered into q_t
        bus = yp - half - 12
        b.append(elbow([(cx(1), yp - half), (cx(1), bus), (cx(t), bus), (cx(t), yq + half + 3)],
                       Q, 1.8))
        for pos in range(2, t + 1):
            b.append(line(cx(pos), yp - half, cx(pos), bus, Q, 1.6))
    # one legend line under the rows
    ly = ROW_Q[8] + DROP + 50
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
