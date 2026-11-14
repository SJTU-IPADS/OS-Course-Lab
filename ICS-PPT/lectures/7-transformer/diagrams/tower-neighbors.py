#!/usr/bin/env python3
"""tower-neighbors: the nearest neighbours of tower in a word-vector model.

One frame, 600 px wide (it sits beside the code that runs the search). A 3D
view with a ground grid: tower in orange, the ten neighbours the search
returns in grey around it, a few unrelated words far away. Points stand on
dashed drop lines so their height reads.
"""
from tk import *

H, WIDTH = 380, 600


def view(ox, oy):
    """Oblique view: x right, y into the page (up-right), z up."""
    def p(x, y, z):
        return ox + x + 0.55 * y, oy - z - 0.32 * y
    return p


P = view(40, 350)


def at(sx, sy, depth):
    """The 3D point drawn at (sx, sy) whose foot lies `depth` into the page."""
    return sx - 40 - 0.55 * depth, depth, 350 - sy - 0.32 * depth


TOWER = at(406, 160, 120)
NEAR = [  # word, position, label side
    ("towers", at(352, 112, 90), "end"),
    ("building", at(470, 114, 150), "start"),
    ("skyscraper", at(336, 202, 60), "end"),
    ("roof", at(486, 205, 110), "start"),
    ("built", at(432, 246, 170), "start"),
    ("dome", at(300, 142, 140), "end"),
    ("facade", at(514, 164, 70), "start"),
    ("constructed", at(362, 242, 40), "end"),
    ("lighthouse", at(418, 76, 200), "start"),
    ("gate", at(258, 170, 180), "end"),
]
FAR = [("banana", at(80, 262, 30), "start"), ("seven", at(150, 66, 210), "start"),
       ("happy", at(520, 300, 20), "start")]


def main():
    b = []
    # ground grid
    for g in range(6):
        b.append(line(*P(90 * g, 0, 0), *P(90 * g, 240, 0), "#dbe5ec", 1))
    for g in range(5):
        b.append(line(*P(0, 60 * g, 0), *P(450, 60 * g, 0), "#dbe5ec", 1))
    o = P(0, 0, 0)
    b.append(arrow(*o, *P(470, 0, 0), MUTED, 1.6))
    b.append(arrow(*o, *P(0, 260, 0), MUTED, 1.6))
    b.append(arrow(*o, *P(0, 0, 300), MUTED, 1.6))
    pts = [(w, xyz, side, MUTED, 17, 5) for w, xyz, side in NEAR + FAR]
    pts.append(("tower", TOWER, "start", ORANGE, 21, 7))
    for w, (x, y, z), side, c, size, r in pts:
        top, foot = P(x, y, z), P(x, y, 0)
        b.append(line(*top, *foot, LINE, 1, dash="3 4"))
        b.append(circle(*foot, 2.5, LINE, LINE, 1))
    for w, (x, y, z), side, c, size, r in pts:
        sx, sy = P(x, y, z)
        dx = r + 5 if side == "start" else -r - 5
        b.append(dot(sx, sy, r, c))
        b.append(text(sx + dx, sy + 6, w, size, c, "bold" if w == "tower" else "normal",
                      side))
    save("tower-neighbors", b, H, w=WIDTH)


main()
