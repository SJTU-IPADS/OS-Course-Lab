#!/usr/bin/env python3
"""machine-learning: a function with tunable parameters, set from examples.

1: two tasks as functions: an image to the word "cat", a passage of text to
   its next word.
2: + f as a box of knobs (its parameters); many input/output examples come
   in and set the knobs.
3: + the simplest case, linear regression: house area against price, the
   line of best fit and its two parameters, slope and intercept.
"""
import hashlib
import math as m

from tk import *

H = 400


def rnd(seed, i):
    return hashlib.md5(f"{seed}:{i}".encode()).digest()[0] / 255.0


def cat(x, y, w=96, h=84):
    """A framed picture of a cat's head."""
    cx, cy = x + w / 2, y + h / 2 + 6
    out = [rect(x, y, w, h, FILL_DATA, DATA, rx=4, width=1.6)]
    ears = (f"M {cx - 26:.1f} {cy - 12:.1f} L {cx - 22:.1f} {cy - 38:.1f} L {cx - 6:.1f} {cy - 22:.1f} Z "
            f"M {cx + 26:.1f} {cy - 12:.1f} L {cx + 22:.1f} {cy - 38:.1f} L {cx + 6:.1f} {cy - 22:.1f} Z")
    out.append(path(ears, INK, 1.8, fill=WHITE, head=False))
    out.append(circle(cx, cy, 26, WHITE, INK, 1.8))
    for ex in (cx - 10, cx + 10):
        out.append(circle(ex, cy - 4, 3, INK, INK, 0.5))
    out.append(path(f"M {cx - 4:.1f} {cy + 5:.1f} L {cx + 4:.1f} {cy + 5:.1f} L {cx:.1f} {cy + 9:.1f} Z",
                    INK, 1.2, fill=INK, head=False))
    for s in (-1, 1):
        for dy in (8, 13):
            out.append(line(cx + s * 8, cy + dy, cx + s * 34, cy + dy + (dy - 10) * 1.2, INK, 1.1))
    return out


def fbox(x, y, s=56):
    return [rect(x, y, s, s, FILL_BLUE, BLUE, rx=6, width=1.6)] + math(x + s / 2, y + s / 2 + 9, "f", 28)


def knob(cx, cy, a, r=13, hl=False):
    """A dial: a circle and a pointer at angle a (degrees from straight up)."""
    c = ORANGE if hl else BLUE
    tx = cx + (r - 3) * m.sin(m.radians(a))
    ty = cy - (r - 3) * m.cos(m.radians(a))
    out = [circle(cx, cy, r, WHITE, INK if not hl else ORANGE, 1.6 if not hl else 2.2),
           line(cx, cy, tx, ty, c, 2.6)]
    return out


def frame(n):
    b = []
    # 1: two tasks
    b.append(label(170, 34, "输入 → 输出", 20, INK, "bold"))
    b += cat(24, 74)
    b.append(arrow(126, 116, 152, 116, INK, 2))
    b += fbox(158, 88)
    b.append(arrow(220, 116, 246, 116, INK, 2))
    b.append(label(254, 123, "cat", 21, INK, anchor="start"))

    lines = ["Behold, a wild", "pi creature,", "foraging in its", "native"]
    b.append(rect(14, 214, 132, 94, FILL_DATA, DATA, rx=6, width=1.4))
    for i, s in enumerate(lines):
        b.append(label(24, 236 + i * 20, s, 15, INK, anchor="start"))
    b.append(arrow(150, 261, 176, 261, INK, 2))
    b += fbox(182, 233)
    b.append(arrow(244, 261, 270, 261, INK, 2))
    b.append(label(278, 268, "land", 21, INK, anchor="start"))

    if n >= 2:
        bx, by, bw, bh = 380, 142, 290, 196
        b.append(label(bx + bw / 2, 34, "输入/输出样本", 20, INK, "bold"))
        for k in range(3):
            cx = bx + 54 + k * 82
            b.append(rect(cx - 36, 54, 72, 32, FILL_DATA, DATA, rx=4, width=1.4))
            b += math(cx, 77, f"(x_{k + 1}, y_{k + 1})", 19)
            b.append(arrow(cx, 90, cx, by - 6, INK, 1.8))
        b.append(label(bx + 54 + 3 * 82 - 8, 76, "⋯", 20, MUTED))
        b.append(rect(bx, by, bw, bh, FILL_BLUE, BLUE, rx=8, width=1.6))
        b += math(bx + 18, by + 30, "f", 26)
        tuned = {(0, 2), (1, 4), (2, 1), (3, 3), (1, 0)}
        for i in range(4):
            for j in range(6):
                cx, cy = bx + 58 + j * 40, by + 34 + i * 44
                a = -120 + 240 * rnd("knob", i * 6 + j)
                b += knob(cx, cy, a, hl=(i, j) in tuned)
        b.append(label(bx + bw / 2, by + bh + 28, "可调参数", 20, BLUE, "bold"))

    if n >= 3:
        x0, y0, w, h = 744, 352, 270, 268
        b.append(label(x0 + w / 2, 34, "线性回归", 20, INK, "bold"))
        b += axes(x0, y0, w, h)
        b.append(text(x0 + w, y0 + 28, "面积", 18, MUTED, anchor="end"))
        b.append(text(x0 + 10, y0 - h + 8, "价格", 18, MUTED, anchor="start"))
        a, k = 0.22, 0.62          # intercept, slope in unit coordinates

        def P(u, v):
            return x0 + u * (w - 20), y0 - v * (h - 30)

        for i in range(25):
            u = 0.15 + 0.8 * rnd("sq", i)
            v = a + k * u + 0.16 * (rnd("pr", i) - 0.5) * 2
            b.append(dot(*P(u, v), 4.5, DATA))
        b.append(line(*P(0, a), *P(1.0, a + k), BLUE, 2.6))
        # slope triangle
        u1, u2 = 0.40, 0.60
        p1, p2 = P(u1, a + k * u1), P(u2, a + k * u2)
        b.append(line(p1[0], p1[1], p2[0], p1[1], ORANGE, 1.8, dash="5 4"))
        b.append(line(p2[0], p1[1], p2[0], p2[1], ORANGE, 1.8, dash="5 4"))
        b.append(text(p2[0] + 8, (p1[1] + p2[1]) / 2 + 22, "斜率", 18, ORANGE, "bold", "start"))
        # intercept
        pi = P(0, a)
        b.append(circle(*pi, 5.5, WHITE, ORANGE, 2.2))
        b.append(text(pi[0] + 12, pi[1] + 26, "截距", 18, ORANGE, "bold", "start"))
    save(f"machine-learning-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
