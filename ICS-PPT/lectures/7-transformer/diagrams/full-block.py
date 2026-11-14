#!/usr/bin/env python3
"""full-block: one pre-norm decoder layer on the residual stream.

The stream runs left to right and carries the n position vectors (columns).
Each sublayer branches off it, normalizes first, computes an update (green)
and adds it back.
1: the attention sublayer: LayerNorm, multi-head attention, add.
2: the feed-forward sublayer after it: LayerNorm, FFN, add.
3: the layer is one of L identical layers stacked in sequence.
"""
from tk import *

H = 332
YS = 236            # the residual stream
YB = 96             # top of the sublayer boxes
BH = 60             # box height
COLS, ROWS, C = 4, 5, 16


def grey(t):
    """White mixed with the data grey; t in [0, 1]."""
    base = (0x5a, 0x6b, 0x78)
    r, g, b_ = (round(255 + (c - 255) * t) for c in base)
    return f"#{r:02x}{g:02x}{b_:02x}"


def seq(x, y, seed):
    """n position vectors side by side: one grey column of cells each."""
    out = []
    for j in range(COLS):
        for i in range(ROWS):
            t = 0.12 + 0.6 * abs(pseudo(seed, i, j))
            out.append(rect(x + j * C, y + i * C, C, C, grey(t), WHITE, rx=0, width=1))
        out.append(rect(x + j * C, y, C, ROWS * C, "none", DATA, rx=0, width=1.2))
    out.append(rect(x, y, COLS * C, ROWS * C, "none", DATA, rx=2, width=1.8))
    return out


def sub_box(x, w, title, sym, stroke, fill):
    out = [rect(x, YB, w, BH, fill, stroke, rx=6, width=1.6),
           text(x + w / 2, YB + 26, title, 19, stroke, "bold")]
    out += math(x + w / 2, YB + 50, sym, 18, INK)
    return out


def plus(x, y, r=14):
    return [circle(x, y, r, WHITE, INK, 1.8),
            line(x - 7, y, x + 7, y, INK, 2), line(x, y - 7, x, y + 7, INK, 2)]


def sublayer(b, x_split, ln_x, op_x, op_w, x_add, title, sym, stroke, fill):
    mid = YB + BH / 2
    b.append(elbow([(x_split, YS), (x_split, mid), (ln_x - 4, mid)], INK, 1.8))
    b += sub_box(ln_x, 112, "LayerNorm", "γ, β", MUTED, FILL_GREY)
    b.append(arrow(ln_x + 112 + 4, mid, op_x - 4, mid, INK, 1.8))
    b += sub_box(op_x, op_w, title, sym, stroke, fill)
    b.append(elbow([(op_x + op_w + 4, mid), (x_add, mid), (x_add, YS - 18)], UPD, 2))
    b += plus(x_add, YS)


def frame(n):
    b = []
    sx, ex = 32, 916                     # input and output blocks
    a1, ln1, op1, w1, p1 = 146, 166, 300, 176, 512
    a2, ln2, op2, w2, p2 = 548, 568, 702, 140, 862
    if n == 3:
        # L identical layers, one behind the other
        cx0, cy0, cw, chh = 120, 64, 760, YS + 48 - 64
        for k in (2, 1):
            b.append(rect(cx0 + 8 * k, cy0 - 8 * k, cw, chh, WHITE, LINE, rx=10, width=1.4))
        b.append(rect(cx0, cy0, cw, chh, WHITE, INK, rx=10, width=1.6))
        b.append(text(cx0 + 16, YS + 36, "第", 18, MUTED, anchor="start"))
        b += math(cx0 + 16 + 24, YS + 36, "l", 20, MUTED, anchor="start")
        b.append(text(cx0 + 16 + 36, YS + 36, "层", 18, MUTED, anchor="start"))
        lx = cx0 + cw + 28
        b += math(lx, cy0 - 2, "× L", 28, INK, anchor="start")
        b.append(text(lx, cy0 + 28, "GPT-3：", 17, MUTED, anchor="start"))
        b += math(lx, cy0 + 52, "L = 96", 19, MUTED, anchor="start")

    top = YS - ROWS * C / 2
    b += seq(sx, top, "fb-in")
    b += math(sx + COLS * C / 2, top + ROWS * C + 36, "X", 22)
    b.append(arrow(sx + COLS * C + 4, YS, p1 - 18, YS, INK, 2.4))
    b.append(text((a1 + p1) / 2, YS - 12, "残差", 17, MUTED))
    sublayer(b, a1, ln1, op1, w1, p1, "多头注意力", "W_Q, W_K, W_V, W_O", BLUE, FILL_BLUE)
    if n == 1:
        b.append(arrow(p1 + 18, YS, a2 + 30, YS, INK, 2.4))
    else:
        b.append(arrow(p1 + 18, YS, p2 - 18, YS, INK, 2.4))
    b += math(p1 + 20, YS + 36, "X′ = X + \\up{MHA}(\\up{LN}(X))", 20, INK, anchor="start")
    if n >= 2:
        b.append(text((a2 + p2) / 2 + 30, YS - 12, "残差", 17, MUTED))
        sublayer(b, a2, ln2, op2, w2, p2, "前馈网络", "W_1, W_2", UPD, FILL_UPD)
        b.append(arrow(p2 + 18, YS, ex - 4, YS, INK, 2.4))
        b += seq(ex, top, "fb-out")
        b += math(ex + COLS * C, top + ROWS * C + 36, "X′ + \\up{FFN}(\\up{LN}(X′))", 20, INK,
                  anchor="end")
    save(f"full-block-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
