#!/usr/bin/env python3
"""output-matrix: the Value_up matrices of all heads form the output matrix.

1: per head h, its value map as two matrices: Value_down_h (128 x 12,288)
   and Value_up_h (12,288 x 128). Heads 1, 2, 3, ..., 96, one tint each.
2: all Value_up_h side by side form one matrix W_O (12,288 x 12,288) of the
   whole block; what papers call the head's value matrix W_V^(h) is
   Value_down_h alone.
3: the block product: W_O times the stacked head outputs [o_1; ...; o_96]
   equals Value_up_1 o_1 + ... + Value_up_96 o_96; each block of W_O and each
   o_h has its head's tint.
The shapes are drawn at 12,288 : 128 = 110 : 20 px.
"""
from tk import *

H = 420
L, T = 110, 20
HEADS = [1, 2, 3, 96]
TINT = {1: "#6c5a9c", 2: "#9c6ab8", 3: "#5a6fb0", 96: "#a8578a"}
LIGHT = {1: "#e3def0", 2: "#efe3f5", 3: "#e0e5f3", 96: "#f3e2ec"}
BOXES = {1: 40, 2: 250, 3: 460, 96: 820}
BY = 8
OX, OY = 214, 236           # top-left of W_O
BLOCK_X = {1: OX, 2: OX + T, 3: OX + 2 * T, 96: OX + 3 * T + 40}


def mat(x, y, w, h, h_):
    return rect(x, y, w, h, LIGHT[h_], TINT[h_], rx=1, width=1.8)


def mixed(x, y, parts, size=18):
    """Text and math pieces left to right: parts = [('t'|'m', s, colour)]."""
    out = []
    for kind, s, c in parts:
        if kind == "t":
            out.append(text(x, y, s, size, c, anchor="start"))
            x += width(s, size)
        else:
            out += math(x, y, s, size + 2, c, anchor="start")
            x += math_width(s, size + 2) * 0.84    # serif italics run narrower
    return out


def box(h_):
    x = BOXES[h_]
    out = [rect(x, BY, 180, 160, WHITE, LINE, rx=6, width=1.4),
           text(x + 90, BY + 24, f"头 {h_}", 19, INK, "bold")]
    out.append(mat(x + 18, BY + 38, T, L, h_))
    out.append(mat(x + 54, BY + 38, L, T, h_))
    out += math(x + 109, BY + 84, f"\\up{{Value}}↓_{{{h_}}}", 20, TINT[h_])
    out += math(x + 48, BY + 146, f"\\up{{Value}}↑_{{{h_}}}", 20, TINT[h_], anchor="start")
    return out


def frame(n):
    b = []
    for h_ in HEADS:
        b += box(h_)
    b.append(text(740, BY + 90, "⋯", 30, MUTED))
    b += mixed(40, H - 42, [("m", "\\up{Value}↓_h", V), ("t", "：128 × 12,288，", INK),
                            ("m", "\\up{Value}↑_h", V), ("t", "：12,288 × 128", INK)])

    if n >= 2:
        # Value_up_h -> its block of W_O
        for h_ in HEADS:
            sx = BOXES[h_] + 18 + T / 2
            b.append(arrow(sx, BY + 38 + L + 4, BLOCK_X[h_] + T / 2, OY - 8, MUTED, 1.4,
                           dash="5 4"))
            b.append(mat(BLOCK_X[h_], OY, T, L, h_))
        b.append(text(OX + 3 * T + 20, OY + L / 2 + 8, "⋯", 22, MUTED))
        b.append(rect(OX - 4, OY - 4, 4 * T + 40 + 8, L + 8, "none", INK, rx=3, width=1.6))
        b += math(OX - 22, OY + 50, "W_O", 26, V, anchor="end")
        b.append(text(OX - 22, OY + 78, "12,288 × 12,288", 17, MUTED, anchor="end"))
        b += mixed(40, H - 16, [("t", "论文中头 h 的值矩阵 ", INK),
                                ("m", "W_V^{(h)}", V), ("t", " 只指 ", INK),
                                ("m", "\\up{Value}↓_h", V)])

    if n >= 3:
        x = OX + 4 * T + 40 + 26
        b.append(label(x, OY + L / 2 + 8, "×", 26))
        vx = x + 22
        y = OY - 5
        for h_ in HEADS:
            if h_ == 96:
                b.append(text(vx + T / 2, y + 26, "⋮", 20, MUTED))
                y += 40
            b.append(mat(vx, y, T, T, h_))
            b += math(vx + T + 8, y + 16, f"o_{{{h_}}}", 19, TINT[h_], anchor="start")
            y += T
        b.append(rect(vx - 3, OY - 8, T + 6, 4 * T + 40 + 6, "none", INK, rx=3, width=1.6))
        ex = vx + 72
        b.append(label(ex, OY + L / 2 + 8, "=", 26))
        parts = []
        for k, h_ in enumerate((1, 2, 96)):
            if h_ == 96:
                parts.append(("t", " + ⋯ + ", INK))
            elif k:
                parts.append(("t", " + ", INK))
            parts.append(("m", f"\\up{{Value}}↑_{{{h_}}} o_{{{h_}}}", TINT[h_]))
        b += mixed(ex + 22, OY + L / 2 + 8, parts, 18)
        b += math(ex + 22, OY + L / 2 + 52, "= Σ_h \\up{Value}↑_h o_h", 22, INK, anchor="start")
    save(f"output-matrix-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
