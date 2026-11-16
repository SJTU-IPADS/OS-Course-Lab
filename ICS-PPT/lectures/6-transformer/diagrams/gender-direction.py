#!/usr/bin/env python3
"""gender-direction: woman - man is close to queen - king.

Points are placed by hand in a 2D view: they show the relation the video
draws, not coordinates of a trained model.
1: the step woman - man, moved to king, lands near queen, but not on it.
2: the family pairs of the video (uncle/aunt, brother/sister, nephew/niece,
   father/mother, son/daughter): the same step joins each pair.
"""
from tk import *

H = 400
X0, Y0 = 90, 360   # origin of the view


def P(x, y):
    return X0 + x, Y0 - y


MAN, WOMAN = P(120, 90), P(200, 190)
KING, QUEEN = P(470, 110), P(570, 225)
STEP = (WOMAN[0] - MAN[0], WOMAN[1] - MAN[1])
IDEAL = (KING[0] + STEP[0], KING[1] + STEP[1])


def frame(n):
    b = []
    b += axes(X0, Y0, 900, 330)
    b += point(*MAN, "man", INK, dx=-12, dy=22, anchor="end")
    b += point(*WOMAN, "woman", INK, dx=-12, dy=-8, anchor="end")
    if n == 1:
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
    if n == 2:
        # the video's family pairs, each joined by the same step
        pairs = [("uncle", "aunt", P(246, 40)), ("brother", "sister", P(376, 40)),
                 ("nephew", "niece", P(506, 40)), ("father", "mother", P(636, 40)),
                 ("son", "daughter", P(766, 40))]
        for m, f, at in pairs:
            to = (at[0] + STEP[0], at[1] + STEP[1])
            b.append(vec(*at, *to, ORANGE))
            b += point(*at, m, INK, dx=10, dy=20)
            b += point(*to, f, INK, dx=10, dy=-6)
        b.append(vec(*MAN, *WOMAN, ORANGE))
        b.append(text(X0 + 20, 40, "同一个方向连接每一对词", 19, ORANGE, "bold",
                      anchor="start"))
    save(f"gender-direction-{n}", b, H)


for n in (1, 2):
    frame(n)
