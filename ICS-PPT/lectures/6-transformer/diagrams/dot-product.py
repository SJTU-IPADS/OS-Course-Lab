#!/usr/bin/env python3
"""dot-product: how it is computed and what its sign says.

Left: v = (2, 1, −1, 3) and w = (1, 3, 0, 2): componentwise products, then
their sum, which is v · w (computed here). Right: three small 2D views: w
pointing roughly along v (dot > 0, green), perpendicular to it (= 0), against
it (< 0, red).
"""
import math as m

from tk import *

H = 290
V, W_ = [2, 1, -1, 3], [1, 3, 0, 2]
CH = 36
TOP = 70


def figure():
    b = []
    els, vh = vector(48, TOP, V, "data", cw=56, ch=CH, size=18)
    b += els
    b += math(76, TOP + vh + 38, "v", 26)
    b.append(label(136, TOP + vh / 2 + 12, "·", 34))
    els, _ = vector(166, TOP, W_, "data", cw=56, ch=CH, size=18)
    b += els
    b += math(194, TOP + vh + 38, "w", 26)

    # componentwise products
    px = 262
    b.append(label(px + 75, TOP - 22, "对应分量相乘", 18, MUTED))
    prods = [a * c for a, c in zip(V, W_)]
    for i, (a, c, p) in enumerate(zip(V, W_, prods)):
        y = TOP + (i + .5) * CH + 6
        b.append(mono(px, y, f"{a:>2} × {c} =", 18, MUTED))
        b.append(mono(px + 116, y, f"{p}", 18, INK, "bold"))
    b += vbrace(px + 146, TOP + 4, TOP + vh - 4, INK, "", size=18)
    total = sum(prods)
    b.append(label(px + 172, TOP + vh / 2 - 8, "相加", 18, MUTED, anchor="start"))
    b += math(px + 172, TOP + vh / 2 + 24, f"v · w = {total}", 24, GREEN, anchor="start",
              weight="bold")

    # the sign of v · w in three cases
    b.append(line(556, 30, 556, 270, LINE, 1.2, dash="3 5"))
    cases = [(35, GREEN, "v · w > 0", "方向相近"), (90, INK, "v · w = 0", "互相垂直"),
             (160, RED, "v · w < 0", "方向相反")]
    for k, (deg, c, rel, word) in enumerate(cases):
        cx, cy, r = 650 + k * 146, 140, 52
        b.append(rect(cx - 66, cy - 66, 132, 132, WHITE, LINE, rx=6, width=1.2))
        b.append(line(cx - 58, cy, cx + 58, cy, "#dbe5ec", 1))
        b.append(line(cx, cy - 58, cx, cy + 58, "#dbe5ec", 1))
        a0 = m.radians(20)
        a1 = a0 + m.radians(deg)
        vx, vy = cx + r * m.cos(a0), cy - r * m.sin(a0)
        wx, wy = cx + r * 0.85 * m.cos(a1), cy - r * 0.85 * m.sin(a1)
        if deg == 90:  # the right angle, marked
            u, t = (m.cos(a0), -m.sin(a0)), (m.cos(a1), -m.sin(a1))
            c1 = (cx + 12 * u[0], cy + 12 * u[1])
            c2 = (c1[0] + 12 * t[0], c1[1] + 12 * t[1])
            c3 = (cx + 12 * t[0], cy + 12 * t[1])
            b.append(path(f"M {c1[0]:.1f} {c1[1]:.1f} L {c2[0]:.1f} {c2[1]:.1f} "
                          f"L {c3[0]:.1f} {c3[1]:.1f}", MUTED, 1.4, head=False))
        b.append(arrow(cx, cy, vx, vy, INK, 2.4))
        b.append(arrow(cx, cy, wx, wy, c, 2.4))
        b += math(vx + 4, vy - 6, "v", 20, INK, anchor="start")
        b += math(wx + (6 if m.cos(a1) >= 0 else -6), wy - 6, "w", 20, c,
                  anchor="start" if m.cos(a1) >= 0 else "end")
        b += math(cx, cy + 98, rel, 21, c)
        b.append(label(cx, cy + 124, word, 17, MUTED))
    save("dot-product", b, H)


figure()
