#!/usr/bin/env python3
"""cross-attention: keys and queries come from two different sequences.

Keys are rows, queries are columns, as in every attention grid of the deck;
each column is a softmax over the keys, drawn as dots; nothing is masked.
1: translation, the video's pair: the French source "Je ne veux pas le
   caresser" gives the keys, the English output "I do not want to pet it"
   gives the queries; the large dots pair corresponding words (the video's
   scores).
2: speech recognition beside it: chunks of an audio waveform give the keys,
   the words of the transcript give the queries; each word attends to the
   chunks where it is spoken.
"""
import math as m

from tk import *
from attn_data import softmax

H = 410
CELL = 40
GY = 96

FR = "Je ne veux pas le caresser".split()
EN = "I do not want to pet it".split()
# the video's unnormalised scores, one list per English word over the French words
TR = [[3, 0, 0, 0, 0, 0], [0, 1, 1.3, 1, 0, 0], [0, 3, 0, 3, 0, 0], [0, 0, 3, 0, 0, 0],
      [0, 0, 0, 0, 0, 3], [0, 0, 0, 0, 0, 3], [0, 0, 0, 0, 3, 0]]

WORDS = "or maybe audio input of speech".split()
CHUNKS = 6
# chunks where each word is spoken: (chunk, score)
SP = [[(0, 3)], [(0, 1.5), (1, 3)], [(2, 3)], [(3, 3), (2, 1)], [(4, 3)],
      [(4, 1), (5, 3)]]

GXA = 214
GXB = 718


def pattern_from(cols, rows):
    """cols[q] = per-key scores; returns weights[k][q] (each column sums to 1)."""
    ws = [softmax(c) for c in cols]
    return [[ws[q][k] for q in range(len(cols))] for k in range(rows)]


def dots(x, y, values, rmax=CELL * 0.42):
    """One dot per cell, its radius proportional to the weight."""
    out = []
    for i, row in enumerate(values):
        for j, v in enumerate(row):
            out.append(circle(x + (j + .5) * CELL, y + (i + .5) * CELL,
                              max(1.6, rmax * v), INK, INK, 1.2))
    return out


def header(x, words):
    """Query words rotated above the columns, q_j under them."""
    out = []
    for j, w in enumerate(words):
        cx = x + (j + .5) * CELL
        out.append(f'<text x="{cx - 4:.1f}" y="{GY - 36:.1f}" font-family="{FONT}" '
                   f'font-size="18" fill="{INK}" text-anchor="start" '
                   f'transform="rotate(-35 {cx - 4:.1f} {GY - 36:.1f})">{w}</text>')
        out += math(cx, GY - 12, f"q_{j + 1}", 19, Q)
    return out


def waveform(x, y, w, h, t0, t1, n=60):
    """The piece [t0, t1] of one speech-like signal, drawn in a w x h box."""
    pts = []
    for s in range(n + 1):
        t = t0 + (t1 - t0) * s / n
        env = 0.25 + 0.75 * abs(m.sin(m.pi * t * 1.0 + 0.4)) ** 1.5
        v = env * (0.6 * m.sin(31 * t) + 0.4 * m.sin(57 * t + 1.3))
        pts.append((x + w * s / n, y + h / 2 - v * h / 2))
    d = "M " + " L ".join(f"{px:.1f} {py:.1f}" for px, py in pts)
    return [rect(x - 4, y - 2, w + 8, h + 4, FILL_DATA, DATA, rx=4, width=1.4),
            path(d, DATA, 1.4, head=False)]


def panel_a():
    b = []
    b += header(GXA, EN)
    for i, w in enumerate(FR):
        cy = GY + (i + .5) * CELL + 7
        b.append(text(GXA - 48, cy, w, 18, INK, anchor="end"))
        b += math(GXA - 22, cy, f"k_{i + 1}", 19, K)
    b += grid(GXA, GY, len(FR), len(EN), CELL)
    b += dots(GXA, GY, pattern_from(TR, len(FR)))
    y = GY + len(FR) * CELL
    b.append(text(GXA + len(EN) * CELL / 2 - 60, y + 30, "翻译", 19, INK, "bold"))
    b.append(text(GXA + len(EN) * CELL / 2 - 60, y + 56, "键来自法语原文，查询来自英语译文", 17,
                  MUTED))
    return b


def panel_b():
    b = []
    b += header(GXB, WORDS)
    for i in range(CHUNKS):
        cy = GY + (i + .5) * CELL
        b += waveform(GXB - 128, cy - 14, 84, 28, i / CHUNKS * 3, (i + 1) / CHUNKS * 3)
        b += math(GXB - 22, cy + 7, f"k_{i + 1}", 19, K)
    cols = [[0.0] * CHUNKS for _ in WORDS]
    for q, hits in enumerate(SP):
        for k, s in hits:
            cols[q][k] = s
    b += grid(GXB, GY, CHUNKS, len(WORDS), CELL)
    b += dots(GXB, GY, pattern_from(cols, CHUNKS))
    y = GY + CHUNKS * CELL
    b.append(text(GXB + len(WORDS) * CELL / 2 - 50, y + 30, "语音转写", 19, INK, "bold"))
    b.append(text(GXB + len(WORDS) * CELL / 2 - 50, y + 56, "键来自音频片段，查询来自转写文本", 17,
                  MUTED))
    return b


save("cross-attention", panel_a() + panel_b(), H)
