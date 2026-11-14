#!/usr/bin/env python3
"""embedding-space: a high-dimensional vector is seen through a projection.

1: points in a 3D frame (oblique view) dropped onto a plane through two of
   the axes, with dashed drop lines: a projection from three dimensions to two.
2: a 12,288-dimensional embedding is projected onto a chosen 3D subspace,
   where the word vectors appear as a small labelled cloud. The projection
   keeps only part of what the vector holds.
"""
import random

from tk import *

H = 390


def view(ox, oy, k=1.0):
    """An oblique projection: x to the right, y into the page (up-right), z up."""
    def p(x, y, z):
        return ox + k * (x + 0.55 * y), oy - k * (z + 0.32 * y)
    return p


def frame3(p, xl, yl, zl, color=MUTED):
    """Three axes from the origin of the view."""
    o = p(0, 0, 0)
    return [arrow(*o, *p(xl, 0, 0), color, 1.6), arrow(*o, *p(0, yl, 0), color, 1.6),
            arrow(*o, *p(0, 0, zl), color, 1.6)]


def frame(n):
    b = []
    # ---- left: 3D -> 2D
    p = view(70, 310, 0.85)
    plane = [p(0, 0, 0), p(320, 0, 0), p(320, 250, 0), p(0, 250, 0)]
    b.append(path("M " + " L ".join(f"{x:.1f} {y:.1f}" for x, y in plane) + " Z",
                  LINE, 1.2, fill=FILL_BLUE, head=False))
    for g in range(1, 4):  # grid lines on the plane
        b.append(line(*p(80 * g, 0, 0), *p(80 * g, 250, 0), "#cfdde6", 1))
        b.append(line(*p(0, 62.5 * g, 0), *p(320, 62.5 * g, 0), "#cfdde6", 1))
    b += frame3(p, 350, 280, 250)
    pts = [(50, 70, 150), (150, 10, 140), (250, 40, 175), (90, 200, 30), (290, 110, 70),
           (200, 210, 130)]
    for x, y, z in pts:
        top, foot = p(x, y, z), p(x, y, 0)
        b.append(line(*top, *foot, DATA, 1.4, dash="4 4"))
        b.append(circle(*foot, 5, WHITE, BLUE, 1.8))
        b.append(dot(*top, 6, INK))
    b.append(label(250, 372, "三维到二维的投影", 20))

    if n == 2:
        b.append(line(487, 40, 487, 350, LINE, 1.2, dash="3 5"))
        # the embedding: a long column of numbers
        vx, vy = 522, 92
        vals = [0.8, -1.2, 2.1, -0.3, 1.0, "⋮", 0.6]
        els, vh = vector(vx, vy, vals, "data", cw=58, ch=30, size=15)
        b += els
        b.append(label(vx + 29, vy - 16, "12,288 维", 18, INK, "bold"))
        ay = vy + vh / 2
        b.append(arrow(vx + 76, ay, vx + 216, ay, INK, 2))
        b.append(label(vx + 146, ay - 14, "投影", 18, MUTED))
        b.append(label(vx + 146, ay + 28, "选定的三维子空间", 17, MUTED))
        # the projected cloud
        q = view(772, 290, 0.9)
        b += frame3(q, 230, 170, 220)
        words = [("king", (40, 40, 150), "end"), ("queen", (70, 60, 175), "start"),
                 ("cat", (180, 10, 160), "end"), ("dog", (205, 30, 180), "start"),
                 ("tower", (60, 60, 80), "end"), ("building", (105, 85, 70), "start")]
        marks = [q(*xyz) for _, xyz, _ in words]
        boxes = []  # the label of each word, as (left, top, right, bottom)
        for (w, _, anc), (sx, sy) in zip(words, marks):
            wd = width(w, 18)
            left = sx + 9 if anc == "start" else sx - 9 - wd
            boxes.append((left - 6, sy - 28, left + wd + 6, sy + 2))

        def clear(sx, sy):
            near = any((sx - mx) ** 2 + (sy - my) ** 2 < 40 ** 2 for mx, my in marks)
            under = any(l <= sx <= r and t <= sy <= bt for l, t, r, bt in boxes)
            ox, oy = q(0, 0, 0)  # keep off the depth axis
            ex, ey = q(0, 170, 0)
            u = max(0, min(1, ((sx - ox) * (ex - ox) + (sy - oy) * (ey - oy))
                           / ((ex - ox) ** 2 + (ey - oy) ** 2)))
            axis = (sx - ox - u * (ex - ox)) ** 2 + (sy - oy - u * (ey - oy)) ** 2 < 12 ** 2
            return not (near or under or axis) and sx < 1010

        rng = random.Random(7)
        kept = 0
        while kept < 18:
            sx, sy = q(rng.uniform(15, 215), rng.uniform(10, 160), rng.uniform(15, 190))
            if clear(sx, sy):
                b.append(dot(sx, sy, 3.5, LINE))
                kept += 1
        for (w, _, anc), (sx, sy) in zip(words, marks):
            dx = 9 if anc == "start" else -9
            b += point(sx, sy, w, INK, dx=dx, dy=-7, size=18, anchor=anc, r=5)
        b.append(label(760, 372, "高维到三维的投影只保留部分信息", 20))
    save(f"embedding-space-{n}", b, H)


for n in (1, 2):
    frame(n)
