#!/usr/bin/env python3
"""logits: the raw output of W_U, before softmax, and the probabilities after.

One frame, on the video's seven-name example. W_U's shading follows the
matrix entries on screen in the video, the last vector and the logits are
the numbers on screen, and the probabilities are computed here.
"""
import math as m

from tk import *

H = 320
NAMES = ["Dumbledore", "Flitwick", "McGonagall", "Quirrell", "Snape", "Sprout", "Trelawney"]
WU = [  # the matrix on screen, row by row (one row per name)
    [-0.04, -0.04, 0.01, -0.03, 0.00, -0.01, -0.04],
    [-2.99, -2.88, -3.35, 2.32, 1.94, 2.58, 3.34],
    [0.12, -0.02, 0.12, -0.16, 0.06, -0.15, 0.19],
    [-0.01, 0.06, 0.16, -0.18, 0.03, -0.05, 0.32],
    [0.10, 0.09, 0.10, 0.37, 0.15, -0.12, -0.05],
    [0.10, -0.22, 0.09, 0.09, -0.15, -0.19, -0.09],
    [0.11, -0.06, 0.05, -0.41, 0.33, 0.24, 0.28],
]
V = [5.4, 7.1, 6.0, 5.4, 4.2, 6.4, 4.3]
X = [-0.8, -5.0, 0.5, 1.5, 3.4, -2.3, 2.5]
P = [m.exp(x) / sum(m.exp(y) for y in X) for x in X]
TOP, CH = 70, 28


def column(x, strs, cw, role="data", fills=None):
    stroke, _ = ROLE[role]
    out = [rect(x - 3, TOP - 3, cw + 6, CH * len(strs) + 6, "none", stroke, rx=4, width=1.8)]
    for i, s in enumerate(strs):
        out.append(rect(x, TOP + i * CH, cw, CH, fills[i] if fills else WHITE, LINE, rx=0,
                        width=1))
        out.append(text(x + cw / 2, TOP + i * CH + CH / 2 + 6, s, 16, INK, font=MONO))
    return out


def main():
    b = []
    big = max(abs(v) for r in WU for v in r)
    gx = 96
    for i, r in enumerate(WU):
        for j, v in enumerate(r):
            b.append(rect(gx + j * CH, TOP + i * CH, CH, CH, weight_fill(v / big), WHITE, rx=0,
                          width=1))
    b.append(rect(gx, TOP, 7 * CH, 7 * CH, "none", INK, rx=2, width=1.6))
    b += math(gx + 3.5 * CH, TOP - 20, "W_U", 26)
    vx = gx + 7 * CH + 22
    b += column(vx, [f"{v:.1f}" for v in V], 48, "hl")
    b.append(note(vx + 24, TOP + 7 * CH + 34, "最后的向量"))
    b.append(label(vx + 76, TOP + 3.5 * CH + 9, "=", 28))
    lx = vx + 104
    b += column(lx, [f"{x:+.1f}".replace("-", "−") for x in X], 58)
    b.append(text(lx + 29, TOP - 20, "logits（未归一化）", 18, INK, "bold"))
    for i, name in enumerate(NAMES):
        b.append(text(lx + 76, TOP + i * CH + CH / 2 + 6, name, 17, INK, anchor="start"))
    ax = lx + 196
    b.append(arrow(ax, TOP + 3.5 * CH, ax + 90, TOP + 3.5 * CH, INK, 2))
    b.append(text(ax + 45, TOP + 3.5 * CH - 12, "softmax", 19, INK, font=MATH))
    px = ax + 108
    b += column(px, [f"{p:.2f}" for p in P], 52)
    b.append(text(px + 26, TOP - 20, "概率", 18, INK, "bold"))
    for i, p in enumerate(P):
        y = TOP + i * CH
        b.append(rect(px + 64, y + 5, max(1.5, p * 170), CH - 10, FILL_BLUE, BLUE, rx=2,
                      width=1.2))
    b.append(note(px + 26, TOP + 7 * CH + 34, f"总和 {sum(P):.0f}"))
    save("logits", b, H, left=-27)


main()
