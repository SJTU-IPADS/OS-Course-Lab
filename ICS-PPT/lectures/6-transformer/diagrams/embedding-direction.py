#!/usr/bin/env python3
"""embedding-direction: a direction that carries meaning, and the dot product.

Left: points placed by hand in a 2D view, showing the relation the video
draws, not coordinates of a trained model. The step woman - man, moved to
king, lands near queen, but not on it.
Right, past a dotted divider: three small 2D views: w pointing roughly along
v (v · w > 0, green), perpendicular to it (= 0), against it (< 0, red).
"""
import math as m

from tk import *

H = 320
X0, Y0 = 20, 296   # origin of the left view

MAN, WOMAN = (78, 262), (158, 162)
KING, QUEEN = (276, 160), (386, 42)
STEP = (WOMAN[0] - MAN[0], WOMAN[1] - MAN[1])
IDEAL = (KING[0] + STEP[0], KING[1] + STEP[1])

DIV = 590          # x of the divider
CX0, PITCH = 678, 140   # centre of the first small view, distance between views
CY = 130           # centre of the small views


def direction():
    b = []
    b += axes(X0, Y0, DIV - 30 - X0, Y0 - 20)
    b += point(*MAN, "man", INK, dx=-12, dy=22, anchor="end")
    b += point(*WOMAN, "woman", INK, dx=-12, dy=-8, anchor="end")
    b += point(*KING, "king", INK, dx=12, dy=20)
    b.append(vec(*MAN, *WOMAN, ORANGE))
    b += math((MAN[0] + WOMAN[0]) / 2 + 14, (MAN[1] + WOMAN[1]) / 2 + 6,
              "E(\\up{woman}) − E(\\up{man})", 20, ORANGE, anchor="start")
    b.append(vec(*KING, *IDEAL, ORANGE, dash="6 5"))
    b.append(circle(*IDEAL, 7, WHITE, ORANGE, 2))
    b.append(text(IDEAL[0] - 14, IDEAL[1] - 14, "king + (woman − man)", 19, ORANGE,
                  anchor="end"))
    b += point(*QUEEN, "queen", GREEN, dx=14, dy=-6, size=20)
    b.append(line(*IDEAL, *QUEEN, GREEN, 1.6, dash="3 4"))
    b.append(text(QUEEN[0] + 14, QUEEN[1] + 22, "实际的 queen 有偏离", 18, GREEN,
                  anchor="start"))
    return b


def signs():
    b = [line(DIV, 30, DIV, H - 30, LINE, 1.2, dash="3 5")]
    cases = [(35, GREEN, "v · w > 0", "方向相近"), (90, INK, "v · w = 0", "互相垂直"),
             (160, RED, "v · w < 0", "方向相反")]
    for k, (deg, c, rel, word) in enumerate(cases):
        cx, cy, r = CX0 + k * PITCH, CY, 52
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
    return b


save("embedding-direction", direction() + signs(), H)
