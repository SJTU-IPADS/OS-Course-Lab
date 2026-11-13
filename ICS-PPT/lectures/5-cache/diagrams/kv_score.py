#!/usr/bin/env python3
"""Two ways to score how relevant each token of the sentence is to the last token.

Left panel: the inner product of the embedding of the last token with the embedding of every
token, x4 . xj. The values are schematic (a vector has the largest inner product with itself),
they are not computed from a model. Right panel: the inner product of the query of the last
token with the key of every token, q4 . kj. These values are SCORES of kv_qkv.py. Each panel is
scaled to its own maximum and the largest bar is outlined. Run it to refresh
../assets/kv-score.svg.
"""

from kv_kit import *
from kv_matrix import WORDS
from svgkit import MONO_EM
from kv_qkv import SCORES

W, H = 860, 168
N = 4
RY, RP, BH = 42, 24, 14                   # top of the first row, row pitch, bar height
BAR = 220                                 # bar length of the panel maximum
LABEL = 44                                # row label: right edge, from the panel's left edge
BX = LABEL + 10                           # bars start here, from the panel's left edge
LEFT, RIGHT = 70, 480                     # left edges of the two panels
EMBED = [0.2, 0.1, 0.4, 1.0]              # schematic: x4 . x1..x4


def pair(x, y, left, right, size=15):
    """The label `left` . `right` with subscripts, such as x4 . xj, in mono; returns the shapes."""
    small = size * 0.74
    out = []
    for i, (name, sub) in enumerate((left, right)):
        if i:
            out.append(mono(x, y, "·", size, INK, "bold"))
            x += size * MONO_EM
        out.append(mono(x, y, name, size, INK, "bold"))
        x += size * MONO_EM
        out.append(mono(x, y + 4, sub, small, INK, "bold"))
        x += small * MONO_EM
    return out


def panel(px, title, math, values, caption):
    out = [text(px, 22, title, 15, INK, "bold", "start")]
    out += pair(px + 15 * 7 + 12, 22, *math)
    top = max(range(N), key=lambda j: values[j])
    for j in range(N):
        cy = RY + j * RP + BH / 2
        best = j == top
        length = BAR * values[j] / values[top]
        out.append(text(px + LABEL, cy + 5, WORDS[j], 15, INK, "bold" if best else "normal",
                        "end"))
        out.append(rect(px + BX, cy - BH / 2, length, BH, MUTED if best else FILL_GREY,
                        INK if best else MUTED, rx=2, width=2 if best else 1.2))
        out.append(mono(px + BX + length + 8, cy + 4.6, f"{values[j]:.1f}", 14, INK,
                        "bold" if best else "normal"))
    out.append(text(px + BX + BAR / 2, RY + N * RP + 16, caption, 14, MUTED, "bold"))
    return out


def figure():
    out = panel(LEFT, "嵌入向量的内积", (("x", "4"), ("x", "j")), EMBED, "与自己的内积最大")
    out += panel(RIGHT, "查询与键的内积", (("q", "4"), ("k", "j")), SCORES,
                 f"与「{WORDS[max(range(N), key=lambda j: SCORES[j])]}」的键最匹配")
    out.append(line((LEFT + RIGHT) / 2 + 157, 10, (LEFT + RIGHT) / 2 + 157, H - 10, LINE, 1.2))
    return out


if __name__ == "__main__":
    save("kv-score", W, H, figure())
