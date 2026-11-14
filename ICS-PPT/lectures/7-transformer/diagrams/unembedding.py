#!/usr/bin/env python3
"""unembedding: from the last vector to a distribution over the vocabulary.

The text, the candidate words and their probabilities are the video's.
1: the text, its last token Professor in orange, and the final layer's
   vectors, the last one in orange.
2: W_U (one row per token: 50,257 rows, 12,288 columns) times that last
   vector gives one number per token, a column of 50,257 logits.
3: softmax turns the column into probabilities: Snape 0.78, Trelawney 0.16,
   Quirrell 0.06, their neighbours in the vocabulary 0.00.
"""
from tk import *

H = 400
ROWS = [None, "Snake", "Snape", "Snare", None, "Treks", "Trelawney", "Trellis", None,
        "Quirky", "Quirrell", "Quirt", None]
PROB = {"Snape": 0.78, "Trelawney": 0.16, "Quirrell": 0.06}
RY, RH = 96, 22                 # top of the vocabulary rows, row height
MID = RY + len(ROWS) * RH / 2   # their vertical middle
GX, GC = 352, 5                 # W_U: left edge, columns drawn
VX = GX + GC * RH + 18          # the vector W_U multiplies
LX = VX + RH + 42               # the logits column


def sentence(cx, y, size=19):
    head, word = "Harry Potter was a highly unusual boy ... least favourite teacher,", "Professor"
    kw = width(word, size) + 12
    total = width(head, size) * 0.92 + 10 + kw
    left = cx - total / 2 + width(head, size) * 0.92 + 10
    return [text(left - 10, y, head, size, INK, anchor="end"),
            rect(left, y - size - 3, kw, size + 12, FILL_ORANGE, ORANGE, rx=4, width=1.6),
            text(left + kw / 2, y, word, size, ORANGE, "bold")]


def grey(seed, i, j, lo=0.15, hi=0.75):
    s = lo + (hi - lo) * (pseudo(seed, i, j) + 1) / 2
    g = round(255 - s * 80)
    return f"#{g - 8:02x}{g - 3:02x}{g:02x}"


def frame(n):
    b = sentence(520, 34)
    # the final layer: one column per token, the last one orange
    cw, pitch, nr, ch = 18, 24, 8, 15
    ay = MID - nr * ch / 2
    cols = [0, 1, 2, 3, None, 4, 5, 6]
    for k, c in enumerate(cols):
        x = 34 + k * pitch
        if c is None:
            b.append(text(x + cw / 2, ay + nr * ch / 2 + 6, "⋯", 18, MUTED))
            continue
        for i in range(nr):
            b.append(rect(x, ay + i * ch, cw, ch, grey("fin", i, c), WHITE, rx=0, width=0.8))
        last = c == 6
        b.append(rect(x - 2, ay - 2, cw + 4, nr * ch + 4, "none", ORANGE if last else DATA,
                      rx=2, width=2.2 if last else 1.2))
    lastx = 34 + 7 * pitch + cw / 2
    b.append(note(34 + (7 * pitch + cw) / 2, ay + nr * ch + 30, "最后一层的向量"))

    if n >= 2:
        # W_U: one row per token of the vocabulary
        for i, word in enumerate(ROWS):
            y = RY + i * RH
            if word is None:
                b.append(text(GX + GC * RH / 2, y + RH / 2 + 6, "⋮", 17, MUTED))
                continue
            for j in range(GC):
                b.append(rect(GX + j * RH, y, RH, RH, weight_fill(pseudo("WU", word, j)), WHITE,
                              rx=0, width=1))
        b.append(rect(GX, RY, GC * RH, len(ROWS) * RH, "none", INK, rx=2, width=1.6))
        b += math(GX - 50, MID - 18, "W_U", 28)
        b.append(text(GX - 50, MID + 16, "50,257 行", 16, MUTED, font=FONT))
        b.append(text(GX - 50, MID + 40, "12,288 列", 16, MUTED, font=FONT))
        # the last vector, carried over
        vy = MID - GC * RH / 2
        for i in range(GC):
            b.append(rect(VX, vy + i * RH, RH, RH, grey("fin", i, 6), WHITE, rx=0, width=0.8))
        b.append(rect(VX - 2, vy - 2, RH + 4, GC * RH + 4, "none", ORANGE, rx=2, width=2.2))
        b.append(elbow([(lastx, ay - 6), (lastx, 72), (VX + RH / 2, 72), (VX + RH / 2, vy - 8)],
                       ORANGE, 2))
        b.append(label(VX + RH + 20, MID + 8, "=", 26))
        # the logits: one number per token
        for i, word in enumerate(ROWS):
            y = RY + i * RH
            if word is None:
                b.append(text(LX + 20, y + RH / 2 + 6, "⋮", 17, MUTED))
                continue
            p = PROB.get(word, 0.0)
            g = round(250 - 150 * p ** 0.5)
            b.append(rect(LX, y, 40, RH, f"#{g - 8:02x}{g - 3:02x}{g:02x}", LINE, rx=0,
                          width=1))
            b.append(text(LX + 50, y + RH / 2 + 6, word, 17, INK, anchor="start"))
        b.append(rect(LX - 2, RY - 2, 44, len(ROWS) * RH + 4, "none", DATA, rx=3, width=1.6))
        b.append(note(LX + 20, RY - 12, "logits"))
    if n >= 3:
        ax = LX + 150
        b.append(arrow(ax, MID, ax + 74, MID, INK, 2))
        b.append(text(ax + 37, MID - 12, "softmax", 19, INK, font=MATH))
        px = ax + 130
        for i, word in enumerate(ROWS):
            y = RY + i * RH
            if word is None:
                b.append(text(px - 20, y + RH / 2 + 6, "⋮", 17, MUTED))
                continue
            p = PROB.get(word, 0.0)
            pick = word == "Snape"
            c, f = (GREEN, FILL_GREEN) if pick else (BLUE, FILL_BLUE)
            b.append(text(px, y + RH / 2 + 6, f"{p:.2f}", 17, c, "bold" if pick else "normal",
                          anchor="end", font=MONO))
            b.append(rect(px + 10, y + 3, max(1.5, p * 160), RH - 6, f, c, rx=2, width=1.2))
        b.append(note(px - 20, RY - 12, "概率"))
    save(f"unembedding-{n}", b, H, left=-15)


for n in (1, 2, 3):
    frame(n)
