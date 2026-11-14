#!/usr/bin/env python3
"""feedforward: the position-wise feed-forward network.

Left: one position. x (d = 4 components) is multiplied by W_1 (4d x d),
giving z (4d components); the nonlinearity phi (GELU) acts on each
component of z; W_2 (d x 4d) maps phi(z) back to d components. Each vector
shows its components as bars around zero; z and phi(z) share one scale, so
the negative components that phi sends towards zero are visible. The
numbers are computed from the drawn weights.
Right: every position goes through the same W_1, W_2, and no position
reads another.
"""
import math as m

from tk import *

H = 330
DIM = 4
HID = 4 * DIM
CH = 12          # cell height of the vectors and the weight cells
CW = 40          # width of a vector column
YC = 150         # vertical centre of the drawing
TOP = YC - HID * CH / 2


def gelu(t):
    return 0.5 * t * (1 + m.erf(t / m.sqrt(2)))


X = [0.9, -0.6, 0.3, 1.2]
W1 = [[pseudo("ff-W1", i, j) for j in range(DIM)] for i in range(HID)]
W2 = [[pseudo("ff-W2", i, j) for j in range(HID)] for i in range(DIM)]
B1 = [0.3 * pseudo("ff-b1", i, 0) for i in range(HID)]
Z = [sum(W1[i][j] * X[j] for j in range(DIM)) + B1[i] for i in range(HID)]
PHI = [gelu(v) for v in Z]
Y = [sum(W2[i][j] * PHI[j] for j in range(HID)) for i in range(DIM)]


def barvec(x, y, values, role, scale, cw=CW, ch=CH):
    """A column vector whose cells show each component as a bar from the
    centre line: right for positive, left for negative."""
    stroke, fill = ROLE[role]
    out = [rect(x - 3, y - 3, cw + 6, ch * len(values) + 6, "none", stroke, rx=4, width=1.8)]
    mid = x + cw / 2
    for i, v in enumerate(values):
        cy = y + i * ch
        out.append(rect(x, cy, cw, ch, WHITE, LINE, rx=0, width=1))
        w = (cw / 2 - 2) * max(-1.0, min(1.0, v / scale))
        if abs(w) > 0.6:
            out.append(rect(min(mid, mid + w), cy + 2, abs(w), ch - 4, stroke, "none", rx=0,
                            width=0))
    out.append(line(mid, y, mid, y + ch * len(values), MUTED, 1))
    return out


def gelu_box(x, y, w=64, h=56):
    """A small plot of phi over [-3, 3]."""
    out = [rect(x, y, w, h, WHITE, INK, rx=6, width=1.6)]
    ox, oy = x + w / 2, y + h - 14
    out.append(line(x + 6, oy, x + w - 6, oy, LINE, 1.2))
    out.append(line(ox, y + 6, ox, y + h - 6, LINE, 1.2))
    pts = []
    for k in range(31):
        t = -3 + 6 * k / 30
        pts.append((ox + t * (w / 2 - 6) / 3, oy - gelu(t) * (h - 22) / 3))
    out.append(path("M " + " L ".join(f"{px:.1f} {py:.1f}" for px, py in pts), INK, 2,
                    head=False))
    return out


def mixed(x, y, parts, size=18, fill=INK):
    """A label of text and math pieces, centred at x."""
    def wid(kind, s):
        return width(s, size) if kind == "t" else math_width(s, size + 2)
    total = sum(wid(k, s) for k, s in parts)
    out, cur = [], x - total / 2
    for kind, s in parts:
        if kind == "t":
            out.append(text(cur, y, s, size, fill, anchor="start"))
        else:
            out += math(cur, y, s, size + 2, fill, anchor="start")
        cur += wid(kind, s)
    return out


def frame():
    b = []
    zs = max(abs(v) for v in Z + PHI)
    name_y, dim_y = TOP + HID * CH + 34, TOP + HID * CH + 60
    short = YC - DIM * CH / 2

    A = 24           # arrow length between two objects

    def step(x0, c=INK):
        b.append(arrow(x0 + 6, YC, x0 + 6 + A, YC, c, 1.8))
        return x0 + A + 14

    x = 30
    b += barvec(x, short, X, "data", max(abs(v) for v in X))
    b += math(x + CW / 2, name_y, "x", 22)
    b += math(x + CW / 2, dim_y, "d", 18, MUTED)
    x = step(x + CW)

    b += weights(x, TOP, HID, DIM, CH, CH, seed="ff-W1")
    b += math(x + DIM * CH / 2, name_y, "W_1", 22)
    b += math(x + DIM * CH / 2, dim_y, "4d × d", 18, MUTED)
    x = step(x + DIM * CH)

    b += barvec(x, TOP, Z, "data", zs)
    b += math(x + CW / 2, name_y, "z", 22)
    b += math(x + CW / 2, dim_y, "4d", 18, MUTED)
    x = step(x + CW)

    b += gelu_box(x, YC - 28)
    b += math(x + 32, YC - 40, "φ", 24)
    x = step(x + 64)

    b += barvec(x, TOP, PHI, "data", zs)
    b += math(x + CW / 2, name_y, "φ(z)", 22)
    b += math(x + CW / 2, dim_y, "4d", 18, MUTED)
    x = step(x + CW)

    b += weights(x, YC - DIM * CH / 2, DIM, HID, CH, CH, seed="ff-W2")
    b += math(x + HID * CH / 2, name_y, "W_2", 22)
    b += math(x + HID * CH / 2, dim_y, "d × 4d", 18, MUTED)
    x = step(x + HID * CH, UPD)

    b += barvec(x, short, Y, "upd", max(abs(v) for v in Y))
    b += math(x + CW / 2, name_y, "\\up{FFN}(x)", 22, UPD)
    b += math(x + CW / 2, dim_y, "d", 18, MUTED)
    left_end = x + CW

    # every position, the same parameters
    sep = left_end + 30
    b.append(line(sep, 30, sep, dim_y, LINE, 1.4, dash="4 5"))
    pitch, cw = 58, 24
    centre = (sep + W - 10) / 2
    xs = [centre + (i - 1.5) * pitch for i in range(4)]
    y_in = 50
    y_box = y_in + DIM * CH + 28
    y_out = y_box + 46 + 28
    for i, cx in enumerate(xs):
        vals = [pseudo("ff-pos", i, k) for k in range(DIM)]
        b += math(cx, y_in - 14, f"x_{i + 1}", 20)
        b += barvec(cx - cw / 2, y_in, vals, "data", 1.0, cw=cw)
        b.append(arrow(cx, y_in + DIM * CH + 6, cx, y_box - 4, INK, 1.8))
        b.append(rect(cx - 25, y_box, 50, 46, FILL_BLUE, BLUE, rx=6, width=1.6))
        b.append(text(cx, y_box + 29, "FFN", 16, BLUE, "bold"))
        b.append(arrow(cx, y_box + 50, cx, y_out - 6, UPD, 1.8))
        h = [gelu(sum(W1[r][k] * vals[k] for k in range(DIM)) + B1[r]) for r in range(HID)]
        yv = [sum(W2[r][k] * h[k] for k in range(HID)) for r in range(DIM)]
        b += barvec(cx - cw / 2, y_out, yv, "upd", max(abs(v) for v in yv), cw=cw)
    mid = (xs[0] + xs[-1]) / 2
    b += mixed(mid, name_y, [("t", "共用 "), ("m", "W_1, W_2")], 18, INK)
    b.append(text(mid, dim_y, "位置之间不交换信息", 17, MUTED))
    save("feedforward", b, H)


if __name__ == "__main__":
    frame()
