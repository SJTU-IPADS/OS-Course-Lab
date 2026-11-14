#!/usr/bin/env python3
"""context-kinds: context changes a word's meaning in many different ways.

1: "... when suddenly they crashed the car into a tree ...": crashed acts on
   car; an intact car beside a dented one.
2: "... wizard ... Hogwarts ... Hermione ... Harry": the embedding of Harry
   moves toward "Harry Potter" (a 2D view of the embedding space).
3: "... Queen ... Sussex ... William ... Harry": from the same start it moves
   toward "Harry 王子".

The examples are parallel, so the figure is drawn once, in its final state;
frame() keeps the step numbers only to say which parts are drawn.
"""
from tk import *

H = 420
DOTS_W = 26


def phrase(x, y, tokens, roles, size=19):
    """Chips for words, plain '...' between them. Returns (elements, spans)."""
    out, spans = [], []
    for tok, role in zip(tokens, roles):
        if tok == "...":
            out.append(text(x + DOTS_W / 2, y + 24, "...", size, MUTED))
            spans.append((x, x + DOTS_W / 2, x + DOTS_W))
            x += DOTS_W + 6
        else:
            els, w = chip(x, y, tok, role, size, pad=9, h=34)
            out += els
            spans.append((x, x + w / 2, x + w))
            x += w + 6
    return out, spans


def phrase_width(tokens, size=19):
    w = 0
    for tok in tokens:
        w += (DOTS_W if tok == "..." else width(tok, size) + 18) + 6
    return w - 6


def car(x, y, s=1.0, dented=False):
    """A side view of a car, x the rear bumper, y the ground line."""
    def P(px, py):
        return f"{x + px * s:.1f} {y - py * s:.1f}"
    if not dented:
        body = [(0, 10), (0, 30), (22, 34), (40, 54), (88, 54), (108, 34), (130, 30),
                (132, 10)]
        win = [(46, 36), (54, 49), (84, 49), (98, 36)]
    else:
        body = [(0, 10), (0, 30), (22, 34), (40, 54), (78, 54), (90, 40), (98, 46),
                (104, 30), (110, 36), (114, 22), (108, 14), (114, 10)]
        win = [(46, 36), (54, 49), (76, 49), (86, 37)]
    d = "M " + " L ".join(P(*p) for p in body) + " Z"
    w = "M " + " L ".join(P(*p) for p in win) + " Z"
    out = [path(d, INK, 1.8, fill=FILL_DATA, head=False),
           path(w, INK, 1.4, fill=WHITE, head=False)]
    for wx in ((28, 100) if not dented else (28, 92)):
        out.append(circle(x + wx * s, y - 10 * s, 10 * s, WHITE, INK, 1.8))
    if dented:
        for (ax, ay), (bx, by) in (((112, 46), (124, 52)), ((116, 36), (128, 38)),
                                    ((114, 26), (126, 22))):
            out.append(line(x + ax * s, y - ay * s, x + bx * s, y - by * s, RED, 1.8))
    return out


CRASH = "... when suddenly they crashed the car into a tree ...".split()
HP = "... wizard ... Hogwarts ... Hermione ... Harry".split()
PR = "... Queen ... Sussex ... William ... Harry".split()

O = (652, 404)                      # origin of the 2D view
H0 = (O[0] + 120, O[1] - 104)       # E(Harry)
POTTER = (O[0] + 286, O[1] - 196)
PRINCE = (O[0] + 300, O[1] - 46)


def context_arrows(spans, sources, target, y, color=MUTED):
    out = []
    tx = spans[target][1]
    for k, src in enumerate(sources):
        sx = spans[src][1]
        out.append(curve(sx, y - 2, tx - 16 + 12 * k, y - 3, bend=-22 - 8 * k, color=color,
                         width=1.6))
    return out


def frame(n):
    b = []
    # ---- 1: crashed -> car
    roles = ["hl" if t == "car" else "data" for t in CRASH]
    x0 = (W - phrase_width(CRASH)) / 2
    els, sp = phrase(x0, 40, CRASH, roles)
    b += els
    ci, ki = CRASH.index("crashed"), CRASH.index("car")
    b.append(curve(sp[ci][1], 38, sp[ki][1], 38, bend=-36, color=ORANGE, width=2))
    gx = 238
    b += car(gx, 172, 0.9)
    b.append(arrow(gx + 132, 148, gx + 178, 148, INK, 1.8))
    b += car(gx + 190, 172, 0.9, dented=True)
    b.append(text(gx + 330, 154, "动作改变 car 的形状与结构", 19, INK, anchor="start"))

    # ---- 2, 3: the two Harrys
    if n >= 2:
        b += axes(O[0], O[1], 360, 200)
        b.append(vec(*O, *H0, DATA, 2.4))
        b.append(dot(*H0, 5, DATA))
        b.append(text(H0[0] - 10, H0[1] - 10, "Harry", 19, INK, anchor="end"))
        roles = ["hl" if t == "Harry" else "data" for t in HP]
        els, sp = phrase(40, 252, HP, roles)
        b += els
        b += context_arrows(sp, [1, 3, 5], 7, 250, ORANGE)
        b.append(vec(*H0, *POTTER, UPD, 2.6))
        b += point(*POTTER, "Harry Potter", UPD, dx=-8, dy=-14, anchor="end", size=19)
    if n >= 3:
        roles = ["hl" if t == "Harry" else "data" for t in PR]
        els, sp = phrase(40, 360, PR, roles)
        b += els
        b += context_arrows(sp, [1, 3, 5], 7, 358, ORANGE)
        b.append(vec(*H0, *PRINCE, UPD, 2.6))
        b += point(*PRINCE, "Harry 王子", UPD, dx=0, dy=30, anchor="middle", size=19)
    save("context-kinds", b, H)


frame(3)
