#!/usr/bin/env python3
"""analogies: more directions that carry meaning, in gender-direction's style.

Points are placed by hand in a 2D view: they show the relations the video
reports, not coordinates of a trained model. One panel per frame:
1: E(Hitler) + E(Italy) − E(Germany) ≈ E(Mussolini): the step Germany → Italy,
   copied to Hitler, lands near Mussolini.
2: E(sushi) + E(Germany) − E(Japan) ≈ E(bratwurst), the same construction.
Below each panel, one sentence says what it shows.

The examples are parallel, so the figure is drawn once, in its final state;
frame() keeps the step numbers only to say which parts are drawn.
"""
from tk import *

PW = 330          # panel width
SY = 0.8          # vertical scale of the panels
AH = 270 * SY     # axis height
OY = 102 + AH     # baseline of every panel's axes
H = OY + 44


def up(v):
    """Height of a point above the baseline; the lowest points keep theirs."""
    return 30 + SY * (v - 30)


def ox(k):
    return 18 + k * (PW + 17)


def analogy(k, a, b_, c, d, A, B, C, D, sides):
    """a -> b is the step; c + (b - a) lands near d. A..D are offsets from the
    panel origin (right, up). sides gives each label's (dx, dy, anchor)."""
    x0 = ox(k) + 10
    pos = {w: (x0 + p[0], OY - up(p[1])) for w, p in zip((a, b_, c, d), (A, B, C, D))}
    step = (pos[b_][0] - pos[a][0], pos[b_][1] - pos[a][1])
    ideal = (pos[c][0] + step[0], pos[c][1] + step[1])
    out = axes(x0, OY, PW - 20, AH)
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


CAPTIONS = ["德国到意大利的方向，也连接两国领导人", "日本到德国的方向，也连接两国的食物"]



def frame(n):
    b = []
    for k, caption in enumerate(CAPTIONS[:n]):
        b.append(label(ox(k) + PW / 2, OY + 34, caption, 17, INK))
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
    save("analogies", b, H, w=ox(2) + 14)


frame(2)
