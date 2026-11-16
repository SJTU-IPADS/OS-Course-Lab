#!/usr/bin/env python3
"""attention-example: one attention head computed by hand on two tokens.

Every token is a column, as on the attention-formula page: q_i = W_Q E_i,
keys are the rows and queries the columns of K^T Q, Softmax runs down each
column. Two tokens (fluffy, creature), embedding dimension 4, d_k = 4.
The upper band holds the inputs and their projections, the lower band the
attention pattern and the output; each frame adds to the one before.
1: the givens: E (columns fluffy, creature) and W_Q, W_K, W_V.
2: W_Q E = Q, W_K E = K, W_V E = V, written beside each weight matrix.
3: K^T Q (keys k_1, k_2 as rows, queries q_1, q_2 as columns), divided by
   √d_k = 2, Softmax per column: the attention pattern A, shaded by weight.
4: V A, one column per token: Delta E_1 = 0.50 v_1 + 0.50 v_2 and
   Delta E_2 = 0.88 v_1 + 0.12 v_2; the creature column of A and of V A is
   outlined.
Numbers are computed here (steps()); check_math.py recomputes them and
checks the two-decimal values on the slide.
"""
import math as _m

from tk import *

E = [[1, 0], [0, 1], [1, 1], [0, 1]]           # columns: fluffy, creature
W_Q = [[1, 0, 0, 0], [0, 0, 1, 1], [0, 1, 1, 0], [0, 1, 0, 1]]
W_K = [[0, 0, 0, 1], [1, 0, 0, 0], [0, 0, 1, 0], [1, 0, 0, 0]]
W_V = [[1, 0, 0, 0], [0, 0, 1, 0], [0, 1, 0, 0], [0, 0, 0, 1]]
WORDS = ["fluffy", "creature"]
D_K = 4


def matmul(a, b_):
    return [[sum(a[i][t] * b_[t][j] for t in range(len(b_))) for j in range(len(b_[0]))]
            for i in range(len(a))]


def transpose(a):
    return [list(r) for r in zip(*a)]


def steps():
    """Every matrix of the example: projections, scores, pattern, output."""
    q, k, v = matmul(W_Q, E), matmul(W_K, E), matmul(W_V, E)
    scores = matmul(transpose(k), q)
    scaled = [[s / _m.sqrt(D_K) for s in row] for row in scores]
    cols = []
    for j in range(len(scaled[0])):
        ex = [_m.exp(row[j]) for row in scaled]
        cols.append([e / sum(ex) for e in ex])
    pattern = transpose(cols)
    return {"Q": q, "K": k, "V": v, "scores": scores, "scaled": scaled,
            "A": pattern, "out": matmul(v, pattern)}


def num(v, dec=False):
    """Integers as they are, weights and outputs (dec) to two decimals,
    U+2212 for minus."""
    s = f"{v:.2f}" if dec or not float(v).is_integer() else str(int(v))
    return s.replace("-", "−")


def mix(c1, c2, t):
    a = [int(c1[i:i + 2], 16) for i in (1, 3, 5)]
    b_ = [int(c2[i:i + 2], 16) for i in (1, 3, 5)]
    return "#%02x%02x%02x" % tuple(round(x + (y - x) * t) for x, y in zip(a, b_))


H = 376
NS = 17                       # size of every number in a cell
ONE = weight_fill(0.45)       # a weight entry of 1; zeros stay white

# upper band: E, then W_X E = X three times
Y1, CH1 = 38, 26              # top of the matrices, row height
HEAD1 = Y1 - 12               # baseline of the column labels
LAB1 = Y1 + 4 * CH1 + 30      # baseline of the names under the matrices
EX, ECW = 32, 68              # E: wide columns, so that the words fit above
WCW = 28                      # weight matrix cell width
XCW = 44                      # Q, K, V cell width
GX0, GSTEP = 206, 272         # first weight matrix, distance between groups

# lower band: K^T Q -> / √d_k -> softmax -> A -> V A
Y2, CELL = 232, 52            # top of the grids, grid cell
HEAD2 = Y2 - 12
LAB2 = Y2 + 2 * CELL + 30
G1, G2, G3 = 62, 270, 486     # left edges of the three grids
OX, OCW = 698, 56             # the output V A
TX = 832                      # its two decompositions


def table(x, y, vals, cw, ch, stroke, fills=None, colors=None, dec=False):
    """A matrix of numbered cells inside a role-coloured outline."""
    out = []
    for i, row in enumerate(vals):
        for j, v in enumerate(row):
            f = fills[i][j] if fills else WHITE
            c = colors[i][j] if colors else INK
            out.append(rect(x + j * cw, y + i * ch, cw, ch, f, LINE, rx=0, width=1))
            out.append(text(x + (j + .5) * cw, y + (i + .5) * ch + NS * 0.36, num(v, dec), NS, c,
                            font=MONO))
    out.append(rect(x - 3, y - 3, cw * len(vals[0]) + 6, ch * len(vals) + 6, "none", stroke,
                    rx=4, width=1.8))
    return out


def weight_table(x, y, vals):
    fills = [[ONE if v else WHITE for v in row] for row in vals]
    colors = [[INK if v else MUTED for v in row] for row in vals]
    out = []
    for i, row in enumerate(vals):
        for j, v in enumerate(row):
            out.append(rect(x + j * WCW, y + i * CH1, WCW, CH1, fills[i][j], WHITE, rx=0,
                            width=1))
            out.append(text(x + (j + .5) * WCW, y + (i + .5) * CH1 + NS * 0.36, num(v), NS,
                            colors[i][j], font=MONO))
    out.append(rect(x, y, 4 * WCW, 4 * CH1, "none", INK, rx=2, width=1.6))
    return out


def shade(w):
    """Fill and text colour of a pattern cell holding weight w."""
    return mix(WHITE, DATA, 0.08 + 0.85 * w), (WHITE if w >= 0.7 else INK)


def step(x0, x1, y, top, bottom=None):
    """An arrow from grid to grid; `top` names the operation (math), `bottom`
    adds a remark under the arrow (math if it holds a subscript)."""
    cx = (x0 + x1) / 2
    out = [arrow(x0, y, x1, y, INK, 2)] + math(cx, y - 12, top, 20, INK)
    if bottom and "_" in bottom:
        out += math(cx, y + 26, bottom, 17, MUTED)
    elif bottom:
        out.append(text(cx, y + 26, bottom, 17, MUTED))
    return out


def frame(n):
    s = steps()
    b = []
    # ---- upper band: the givens
    for j, w in enumerate(WORDS):
        b.append(text(EX + (j + .5) * ECW, HEAD1, w, 17, INK))
    b += table(EX, Y1, E, ECW, CH1, DATA)
    b += math(EX + ECW, LAB1, "E", 22, DATA)
    groups = [("W_Q", W_Q, "Q", "q", Q), ("W_K", W_K, "K", "k", K), ("W_V", W_V, "V", "v", V)]
    for g, (wname, wv, name, sym, color) in enumerate(groups):
        wx = GX0 + g * GSTEP
        b += weight_table(wx, Y1, wv)
        b += math(wx + 2 * WCW, LAB1, wname, 22, color)
        if n >= 2:
            ex = wx + 4 * WCW
            b += math(ex + 26, Y1 + 2 * CH1 + 8, "E =", 22, INK)
            rx = ex + 52
            for j in range(2):
                b += math(rx + (j + .5) * XCW, HEAD1, f"{sym}_{j + 1}", 19, color)
            b += table(rx, Y1, s[name], XCW, CH1, color)
            b += math(rx + XCW, LAB1, name, 22, color)
    # ---- lower band: scores, scaling, softmax per column
    if n >= 3:
        mid = Y2 + CELL
        for i in range(2):
            b += math(G1 - 12, Y2 + (i + .5) * CELL + 7, f"k_{i + 1}", 19, K, anchor="end")
        for gx in (G1, G2, G3):
            for j in range(2):
                b += math(gx + (j + .5) * CELL, HEAD2, f"q_{j + 1}", 19, Q)
        b += table(G1, Y2, s["scores"], CELL, CELL, INK)
        b += math(G1 + CELL, LAB2, "K^TQ", 20, INK)
        b += step(G1 + 2 * CELL + 16, G2 - 16, mid, "÷√d_k", "√d_k = 2")
        b += table(G2, Y2, s["scaled"], CELL, CELL, INK)
        b += math(G2 + CELL, LAB2, "K^TQ / √d_k", 20, INK)
        b += step(G2 + 2 * CELL + 16, G3 - 16, mid, "\\up{softmax}", "逐列")
        pa = s["A"]
        fills = [[shade(w)[0] for w in row] for row in pa]
        colors = [[shade(w)[1] for w in row] for row in pa]
        b += table(G3, Y2, pa, CELL, CELL, INK, fills, colors, dec=True)
        b.append(text(G3 + CELL + 4, LAB2, "注意力模式", 18, INK, "bold", anchor="end"))
        b += math(G3 + CELL + 12, LAB2, "A", 20, INK, anchor="start")
    # ---- the output: V A, one column per token
    if n >= 4:
        b.append(rect(G3 + CELL, Y2, CELL, 2 * CELL, "none", ORANGE, rx=0, width=2.6))
        x0, x1 = G3 + 2 * CELL + 16, OX - 16
        b.append(arrow(x0, mid, x1, mid, INK, 2))
        lx = (x0 + x1) / 2 - 27                    # "左乘 V", 54 px wide
        b.append(text(lx, mid - 12, "左乘", 18, INK, anchor="start"))
        b += math(lx + 42, mid - 12, "V", 20, V, anchor="start")
        for j in range(2):
            b += math(OX + (j + .5) * OCW, HEAD2, f"ΔE_{j + 1}", 19, UPD)
        b += table(OX, Y2, s["out"], OCW, CH1, UPD, dec=True)
        b.append(rect(OX + OCW, Y2, OCW, 4 * CH1, "none", ORANGE, rx=0, width=2.6))
        b += math(OX + OCW, LAB2, "V A", 20, UPD)
        pa = s["A"]
        for j in range(2):
            terms = " + ".join(f"{pa[i][j]:.2f} v_{i + 1}" for i in range(2))
            b += math(TX, Y2 + 36 + j * 40, f"ΔE_{j + 1} = {terms}", 19,
                      UPD if j == 0 else ORANGE, anchor="start",
                      weight="normal" if j == 0 else "bold")
    save(f"attention-example-{n}", b, H)


if __name__ == "__main__":
    for n in (1, 2, 3, 4):
        frame(n)
