#!/usr/bin/env python3
"""analogies: more directions that carry meaning, in gender-direction's style.

Points are placed by hand in a 2D view: they show the relations the video
reports, not coordinates of a trained model. One panel per frame:
1: E(Hitler) + E(Italy) − E(Germany) ≈ E(Mussolini): the step Germany → Italy,
   copied to Hitler, lands near Mussolini.
2: E(sushi) + E(Germany) − E(Japan) ≈ E(bratwurst), the same construction.
3: the nearest neighbours of cat: beast and monster among them (only these
   two are named in the video; the other neighbours are unlabelled dots).

The examples are parallel, so the figure is drawn once, in its final state;
frame() keeps the step numbers only to say which parts are drawn.
"""
from tk import *

H = 400
PW = 330          # panel width
OY = 372          # baseline of every panel's axes


def ox(k):
    return 18 + k * (PW + 17)


def analogy(k, a, b_, c, d, A, B, C, D, sides):
    """a -> b is the step; c + (b - a) lands near d. A..D are offsets from the
    panel origin (right, up). sides gives each label's (dx, dy, anchor)."""
    x0 = ox(k) + 10
    pos = {w: (x0 + p[0], OY - p[1]) for w, p in zip((a, b_, c, d), (A, B, C, D))}
    step = (pos[b_][0] - pos[a][0], pos[b_][1] - pos[a][1])
    ideal = (pos[c][0] + step[0], pos[c][1] + step[1])
    out = axes(x0, OY, PW - 20, 270)
    out.append(vec(*pos[a], *pos[b_], ORANGE))
    out.append(vec(*pos[c], *ideal, ORANGE, dash="6 5"))
    out.append(circle(*ideal, 7, WHITE, ORANGE, 2))
    out.append(line(*ideal, *pos[d], GREEN, 1.6, dash="3 4"))
    for w in (a, b_, c):
        dx, dy, anc = sides[w]
        out += point(*pos[w], w, INK, dx=dx, dy=dy, anchor=anc)
    dx, dy, anc = sides[d]
    out += point(*pos[d], d, GREEN, dx=dx, dy=dy, anchor=anc, size=20)
    return out


def title(k, line1, line2=None):
    cx = ox(k) + PW / 2
    out = math(cx, 30, line1, 20)
    if line2:
        out += math(cx, 58, line2, 20)
    return out


def frame(n):
    b = []
    b += title(0, "E(\\up{Hitler}) + E(\\up{Italy}) − E(\\up{Germany})", "≈ E(\\up{Mussolini})")
    b += analogy(0, "Germany", "Italy", "Hitler", "Mussolini",
                 (40, 60), (165, 30), (55, 185), (190, 165),
                 {"Germany": (-2, -12, "start"), "Italy": (10, 22, "start"),
                  "Hitler": (-2, -12, "start"), "Mussolini": (-10, -14, "start")})
    if n >= 2:
        b += title(1, "E(\\up{sushi}) + E(\\up{Germany}) − E(\\up{Japan})", "≈ E(\\up{bratwurst})")
        b += analogy(1, "Japan", "Germany", "sushi", "bratwurst",
                     (45, 150), (140, 50), (120, 225), (225, 117),
                     {"Japan": (-2, -12, "start"), "Germany": (-30, 26, "start"),
                      "sushi": (-2, -12, "start"), "bratwurst": (12, 6, "start")})
    if n >= 3:
        k = 2
        x0 = ox(k) + 10
        b += axes(x0, OY, PW - 20, 270)
        b.append(label(ox(k) + PW / 2, 44, "cat 的近邻", 20))
        cx, cy = x0 + 140, OY - 140
        b.append(circle(cx, cy, 92, "none", LINE, 1.4).replace("/>", ' stroke-dasharray="5 5"/>'))
        for dx, dy in [(-46, -40), (-60, 18), (-20, 60), (34, -58), (-70, -10), (10, -30),
                       (-34, 30)]:
            b.append(dot(cx + dx, cy + dy, 4, MUTED))
        for dx, dy in [(-110, -150), (130, -100), (-105, 110), (120, 105)]:  # far words
            b.append(dot(cx + dx, cy + dy, 4, LINE))
        b += point(cx, cy, "cat", INK, dx=10, dy=6, size=20, r=6)
        b += point(cx + 48, cy + 44, "beast", ORANGE, dx=10, dy=6, size=20)
        b += point(cx + 56, cy - 18, "monster", ORANGE, dx=10, dy=-6, size=20)
    save("analogies", b, H)


frame(3)
