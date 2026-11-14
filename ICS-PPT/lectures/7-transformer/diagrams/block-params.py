#!/usr/bin/env python3
"""block-params: the parameters of one multi-head attention block of GPT-3.

One head enlarged on the left: W_Q, W_K, Value_down (128 x 12,288 each) and
Value_up (12,288 x 128), 6,291,456 entries in all. On the right the 96 heads
of the block, each drawn as its four small matrices, and the product
96 x 6,291,456 = 603,979,776.
"""
from tk import *

H = 360
COLS, ROWS = 16, 6
SW, SH = 44, 42             # slot of one head in the grid
BX, BY = 296, 54            # top-left of the grid
MATS = [("W_Q", Q, FILL_Q), ("W_K", K, FILL_K), ("\\up{Value}_↓", V, FILL_V)]


def mini(x, y):
    """One head: three wide strips and one tall strip."""
    out = [rect(x, y, SW - 6, SH - 8, WHITE, LINE, rx=4, width=1.2)]
    for k, (_, c, f) in enumerate(MATS):
        out.append(rect(x + 12, y + 6 + k * 8, 22, 4, c, c, rx=0, width=0.6))
    out.append(rect(x + 4, y + 5, 4, 24, V, V, rx=0, width=0.6))
    return out


b = []
# ---- one head, enlarged
lx = 50
b.append(text(lx + 100, BY + 4, "一个头", 20, INK, "bold"))
for k, (name, c, f) in enumerate(MATS):
    y = BY + 34 + k * 40
    b.append(rect(lx + 34, y, 96, 12, f, c, rx=1, width=1.6))
    b += math(lx + 140, y + 13, name, 20, c, anchor="start")
b.append(rect(lx, BY + 28, 12, 96, FILL_V, V, rx=1, width=1.6))
b += math(lx + 6, BY + 150, "\\up{Value}_↑", 20, V)
b.append(note(lx + 100, BY + 192, "四个矩阵各 1,572,864"))
b.append(text(lx + 100, BY + 226, "合计 6,291,456", 20, INK, "bold"))

# ---- 96 heads
b.append(line(BX - 34, BY - 10, BX - 34, BY + ROWS * SH - 4, LINE, 1.4))
b.append(text(BX + COLS * SW / 2 - 3, BY - 16, "一个注意力块：96 个头", 19, INK))
for r in range(ROWS):
    for c in range(COLS):
        b += mini(BX + c * SW, BY + r * SH)

b.append(text(W / 2, H - 28, "96 × 6,291,456 = 603,979,776 ≈ 6 亿", 24, INK, "bold"))
save("block-params", b, H)
