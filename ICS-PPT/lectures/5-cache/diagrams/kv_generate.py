#!/usr/bin/env python3
"""Autoregressive generation in two phases. The prefill takes the whole prompt at once and
outputs the first new token; each decoding step appends the token just produced to the
input and outputs one more. One row per step, the same sentence as the attention figures.
Run it to refresh ../assets/kv-generate.svg.
"""

from kv_kit import *
from kv_matrix import WORDS
from svgkit import box

W, H = 1000, 236
N = 4                                    # prompt tokens
LX = 150                                 # right edge of the row names
X0, CW, PITCH = 166, 62, 68              # token cells: left edge, width, pitch
MX, MW = 640, 150                        # the model box
OX = 848                                 # the output token
Y0, RP, RH = 22, 72, 34                  # first row, row pitch, cell height


def step(row, name, n):
    """Row `row`: the first n words go in, word n comes out."""
    y = Y0 + row * RP
    out = [text(LX, y + RH / 2 + 6, name, 17, INK, "bold", "end")]
    for i in range(n):
        kind = "new" if row and i == n - 1 else "mid"
        out += cell(X0 + i * PITCH, y, CW, RH, "", kind)
        out.append(text(X0 + i * PITCH + CW / 2, y + RH / 2 + 6, WORDS[i], 16, INK, "bold"))
    out.append(arrow(X0 + n * PITCH, y + RH / 2, MX - 6, y + RH / 2, INK, 2))
    out += box(MX, y - 4, MW, RH + 8, "模型", fill=FILL_GREY, stroke=INK, size=17)
    out.append(arrow(MX + MW, y + RH / 2, OX - 6, y + RH / 2, INK, 2))
    out += cell(OX, y, CW, RH, "", "new")
    out.append(text(OX + CW / 2, y + RH / 2 + 6, WORDS[n], 16, INK, "bold"))
    return out


def append(row, n):
    """The output of row `row` becomes the last input of the next row."""
    y = Y0 + row * RP + RH
    ox = OX + CW / 2
    tx = X0 + n * PITCH + CW / 2
    mid = y + (RP - RH) / 2
    return [elbow([(ox, y + 2), (ox, mid), (tx, mid), (tx, y + RP - RH - 4)], ORANGE, 2)]


def figure():
    out = []
    out += step(0, "预填充", N)
    out += append(0, N)
    out += step(1, "解码第 1 步", N + 1)
    out += append(1, N + 1)
    out += step(2, "解码第 2 步", N + 2)
    out.append(text(OX + CW / 2, Y0 - 4 + 3 * RP, "⋯ 直到输出结束符", 15, MUTED, "bold"))
    return out


if __name__ == "__main__":
    save("kv-generate", W, H, figure())
