#!/usr/bin/env python3
"""gpt3-matrices: GPT-3's weights are 27,938 matrices of eight kinds.

One frame. Left: a stack of weight matrices. Right: the eight categories in
the order the video counts them (the table later pages fill in), every
count still unknown, the total known.
"""
from tk import *

H = 380


def table(x, y, w=440, rh=32, size=18):
    rows = len(TALLY) + 1
    out = [rect(x, y, w, rh * rows + 16, WHITE, LINE, rx=6, width=1.4)]
    for i, (name, sym, n, c) in enumerate(TALLY):
        top = y + 6 + i * rh
        base = top + rh / 2 + size * 0.36
        out.append(text(x + 16, base, name, size, INK, anchor="start"))
        out += math(x + 150, base, sym, size + 2, c, anchor="start")
        out.append(text(x + w - 16, base, "?", size, LINE, anchor="end", font=MONO))
    top = y + 6 + len(TALLY) * rh
    base = top + rh / 2 + size * 0.36 + 4
    out.append(line(x + 10, top + 4, x + w - 10, top + 4, INK, 1.4))
    out.append(text(x + 16, base, "合计", size, INK, "bold", anchor="start"))
    total = sum(n for _, _, n, _ in TALLY)
    out.append(text(x + w - 16, base, f"{total:,}", size, INK, "bold", anchor="end", font=MONO))
    return out


b = []
b.append(label(520, 32, "175,181,291,520 个权重，组织为 27,938 个矩阵", 21, INK, "bold"))

# a stack of matrices
sx, sy, cs, dx, dy, k = 70, 178, 15, 16, -14, 7
for s in range(k - 1, -1, -1):
    b += weights(sx + s * dx, sy + s * dy, 8, 10, cs, cs, seed=f"stack{s}")
b.append(label(sx + 75 + 3 * dx, sy + 8 * cs + 34, "27,938 个矩阵", 20, INK, "bold"))

tx, ty = 560, 52
b.append(arrow(sx + 10 * cs + (k - 1) * dx + 20, 190, tx - 14, 190, INK, 2))
b.append(label((sx + 10 * cs + (k - 1) * dx + tx) / 2 + 3, 176, "分为八类", 18, MUTED))
b += table(tx, ty)
save("gpt3-matrices", b, H)
