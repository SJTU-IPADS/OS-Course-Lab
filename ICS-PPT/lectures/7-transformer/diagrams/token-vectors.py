#!/usr/bin/env python3
"""token-vectors: every token is looked up as a vector of numbers.

The tokens of "the cleverest thinker", each above its column vector (the
numbers the video shows for these tokens). Beside them, the vectors read as
points of a space, drawn in three dimensions: dog, cat, rabbit close together;
Paris, France, Italy; run, walk, jump.
"""
from tk import *

H = 400
TOKS = ["the", "cle", "ve", "rest", "thinker"]
VALS = [[2.6, 7.7, 4.5, 5.6, 0.2, 6.1],
        [3.6, 4.3, 6.9, 0.6, 6.6, 6.6],
        [5.6, 4.3, 9.8, 1.0, 2.1, 1.6],
        [1.6, 1.1, 6.5, 1.4, 1.9, 3.7],
        [9.7, 4.6, 9.7, 6.0, 7.3, 0.4]]
CX0, PITCH, CW, CH = 96, 92, 56, 28

# the 3D view: origin and the three axis vectors (oblique projection)
O = (650, 300)
AX, AY, AZ = (330, 0), (0, -235), (-84, 62)


def P(x, y, z):
    return (O[0] + x * AX[0] + y * AY[0] + z * AZ[0],
            O[1] + x * AX[1] + y * AY[1] + z * AZ[1])


CLUSTERS = [  # word, (x, y, z), label dx, dy, anchor
    [("dog", (.16, .70, .30), -10, -8, "end"), ("cat", (.24, .78, .45), 10, -8, "start"),
     ("rabbit", (.28, .62, .20), 10, 18, "start")],
    [("Paris", (.70, .80, .55), -10, -6, "end"), ("France", (.80, .86, .35), 10, -6, "start"),
     ("Italy", (.86, .72, .50), 10, 18, "start")],
    [("run", (.50, .30, .15), -10, -6, "end"), ("walk", (.60, .24, .35), 10, 18, "start"),
     ("jump", (.62, .36, .05), 10, -8, "start")],
]


def figure():
    b = []
    top = 60
    for k, (tok, vals) in enumerate(zip(TOKS, VALS)):
        cx = CX0 + k * PITCH
        els, _ = chip(cx, top, tok, size=19, anchor="middle")
        b += els
        b.append(arrow(cx, top + 40, cx, top + 70, INK, 1.8))
        els, _ = vector(cx - CW / 2, top + 78, vals + ["⋮"], cw=CW, ch=CH, size=16)
        b += els
    b.append(note(CX0 - 14, top + 61, "查表", anchor="end"))

    # the floor (x-z plane) and the axes
    f = [P(0, 0, 0), P(1, 0, 0), P(1, 0, 1), P(0, 0, 1)]
    d = "M " + " L ".join(f"{x:.1f} {y:.1f}" for x, y in f) + " Z"
    b.append(path(d, "none", 0, fill=FILL_GREY, head=False))
    for t in (.25, .5, .75):
        b.append(line(*P(t, 0, 0), *P(t, 0, 1), WHITE, 1.4))
        b.append(line(*P(0, 0, t), *P(1, 0, t), WHITE, 1.4))
    b.append(arrow(*O, *P(1.05, 0, 0), LINE, 1.6))
    b.append(arrow(*O, *P(0, 1.08, 0), LINE, 1.6))
    b.append(arrow(*O, *P(0, 0, 1.12), LINE, 1.6))
    for group in CLUSTERS:
        for word, (x, y, z), dx, dy, anc in group:
            px, py = P(x, y, z)
            fx, fy = P(x, 0, z)
            b.append(line(px, py, fx, fy, LINE, 1.2, dash="3 4"))
            b.append(circle(fx, fy, 2.2, LINE, LINE, 0.5))
    for group in CLUSTERS:
        for word, (x, y, z), dx, dy, anc in group:
            b += point(*P(x, y, z), word, INK, dx=dx, dy=dy, size=18, anchor=anc)
    b.append(label(820, 34, "含义相近的词，向量相近", 19, INK, "bold"))
    save("token-vectors", b, H)


figure()
