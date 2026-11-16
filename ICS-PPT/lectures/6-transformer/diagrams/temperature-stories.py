#!/usr/bin/env python3
"""temperature-stories: GPT-3 continuing one seed at two temperatures.

The stories themselves are not recorded, so their lines are drawn as bars of
text; only the phrase the narration quotes is written out.
1: the seed, then T = 0: always the most likely word, a trite story close
   to Goldilocks.
2: T = 5: the story opens with an unusual phrase and later lines break up.

The examples are parallel, so the figure is drawn once, in its final state;
frame() keeps the step numbers only to say which parts are drawn.
"""
import random

from tk import *

H = 264                   # panel bottom + the 12 px top margin
SEED = "Once upon a time, there was a"
PHRASE = "a young web artist from South Korea"
PY, PH = 96, 156          # panels: top, height
LH = 24                   # line pitch inside a panel


def bars_line(x, y, x_end, rng, h=10):
    """Words as grey bars from x to about x_end."""
    out = []
    while True:
        w = rng.uniform(18, 66)
        if x + w > x_end:
            break
        out.append(rect(x, y - h, w, h, LINE, "none", rx=5, width=0))
        x += w + 9
    return out


def scribble(x, y, x_end, rng, amp):
    pts = [(x, y - 5)]
    while pts[-1][0] < x_end:
        px = pts[-1][0] + rng.uniform(6, 14)
        pts.append((min(px, x_end), y - 5 + rng.uniform(-amp, amp)))
    return path("M " + " L ".join(f"{a:.1f} {c:.1f}" for a, c in pts), MUTED, 1.6, head=False)


def panel_box(x, w, t, title, color=MUTED):
    out = [rect(x, PY, w, PH, WHITE, LINE, rx=8, width=1.4)]
    out.append(text(x + 16, PY + 28, title, 18, color, "bold", "start"))
    return out


def tag(x, y, t, anchor):
    """T = t with T italic red, as in the temperature figure."""
    return [f'<text x="{x:.1f}" y="{y:.1f}" font-family="{MATH}" font-size="21" fill="{RED}" '
            f'text-anchor="{anchor}"><tspan font-style="italic">T</tspan> = {t}</text>']


def frame(n):
    b = []
    sw = width(SEED, 20) + 28
    b.append(rect(520 - sw / 2, 12, sw, 38, FILL_DATA, DATA, rx=6, width=1.4))
    b.append(text(520, 38, SEED, 20, INK))
    # T = 0
    lx, w = 40, 450
    b.append(arrow(470, 54, 300, PY - 4, INK, 1.8))
    b += tag(368, 72, 0, "end")
    b += panel_box(lx, w, 0, "近似 Goldilocks 的老套故事")
    rng = random.Random(3)
    for k in range(4):
        end = lx + w - 18 if k < 3 else lx + w - 160
        b += bars_line(lx + 18, PY + 60 + k * LH, end, rng)
    if n == 2:
        rx = 550
        b.append(arrow(570, 54, 740, PY - 4, INK, 1.8))
        b += tag(672, 72, 5, "start")
        b += panel_box(rx, w, 5, "随后变得混乱", RED)
        pw = width(PHRASE, 17) + 10
        y = PY + 60
        b.append(rect(rx + 14, y - 17, pw, 24, FILL_ORANGE, ORANGE, rx=4, width=1.2))
        b.append(text(rx + 19, y, PHRASE, 17, INK, anchor="start"))
        rng = random.Random(5)
        b += bars_line(rx + 24 + pw, y, rx + w - 18, rng)
        b += bars_line(rx + 18, y + LH, rx + w - 120, rng)
        b.append(scribble(rx + w - 110, y + LH, rx + w - 20, rng, 4))
        b += bars_line(rx + 18, y + 2 * LH, rx + 150, rng)
        b.append(scribble(rx + 160, y + 2 * LH, rx + w - 20, rng, 7))
        b.append(scribble(rx + 18, y + 3 * LH, rx + w - 100, rng, 9))
    save("temperature-stories", b, H)


frame(2)
