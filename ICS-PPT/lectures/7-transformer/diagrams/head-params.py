#!/usr/bin/env python3
"""head-params: the query and key matrices of one head and their sizes.

W_Q and W_K are 128 x 12,288 each: 12,288 columns (the embedding
dimension), 128 rows (the key/query space). They are drawn wide and thin at
a 1:20 ratio (the true ratio, 1:96, would be a line); the labels give the
true shape and the entry count, 1,572,864 each.
"""
from tk import *

H = 280
ROWS, COLS = 4, 64
CW, CH = 10, 8            # 640 x 32 px: 1:20
GX = 114
GW = COLS * CW


def matrix(y, name, color, seed, rows_label):
    out = weights(GX, y, ROWS, COLS, CW, CH, seed=seed)
    out += math(GX - 26, y + ROWS * CH / 2 + 9, name, 28, color, anchor="end")
    # rows: the key/query space
    x = GX + GW + 10
    out.append(path(f"M {x} {y} L {x + 8} {y} L {x + 8} {y + ROWS * CH} L {x} {y + ROWS * CH}",
                    INK, 1.6, head=False))
    out.append(text(x + 18, y + ROWS * CH / 2 + 7, rows_label, 19, INK, anchor="start"))
    out.append(text(GX + GW, y + ROWS * CH + 30, "128 × 12,288 = 1,572,864", 20, color,
                    "bold", anchor="end"))
    return out


b = []
# columns: the embedding dimension
y0 = 62
b.append(path(f"M {GX} {y0} L {GX} {y0 - 8} L {GX + GW} {y0 - 8} L {GX + GW} {y0}", INK, 1.6,
              head=False))
b.append(text(GX + GW / 2, y0 - 18, "12,288 列：嵌入维数", 19, INK))
b += matrix(y0 + 12, "W_Q", Q, "WQ", "128 行：键查询空间维数")
b += matrix(y0 + 132, "W_K", K, "WK", "128 行")
save("head-params", b, H)
