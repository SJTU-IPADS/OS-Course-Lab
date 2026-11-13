#!/usr/bin/env python3
"""Text generation, one token per step, in three frames.

Each row is one run of the model: the tokens so far go in, the model outputs one token,
and that token is appended to the input of the next row (orange elbow arrow). Row 1 is
the prefill, rows 2 and 3 are decoding steps; frame k draws the first k rows. A column
header marks the output tokens. The sentence is the one of the attention figures. The
page text says where generation ends.
Run it to refresh ../assets/kv-generate-1..3.svg.
"""

from kv_kit import *
from kv_matrix import WORDS
from svgkit import box

W, H = 809, 254
N = 4                                    # prompt tokens
LX = 117                                # right edge of the row names
X0, CW, PITCH = 129, 58, 63              # token cells: left edge, width, pitch
MX, MW = 555, 96                         # the model box
OX, OW = 701, 62                         # the output token
Y0, RP, RH = 40, 78, 32                  # first row, row pitch, cell height

NAMES = ["预填充", "解码第 1 步", "解码第 2 步"]


def centre(row):
    return Y0 + row * RP + RH / 2


def step(row):
    """Row `row`: the first N + row words go in, the next word comes out."""
    n = N + row
    y, cy = Y0 + row * RP, centre(row)
    out = [text(LX, cy + 6, NAMES[row], 17, INK, "bold", "end")]
    for i in range(n):
        kind = "new" if row and i == n - 1 else "mid"
        out += cell(X0 + i * PITCH, y, CW, RH, "", kind)
        out.append(text(X0 + i * PITCH + CW / 2, cy + 6, WORDS[i], 16, INK, "bold"))
    out.append(arrow(X0 + (n - 1) * PITCH + CW + 5, cy, MX - 6, cy, INK, 2))
    out += box(MX, y - 4, MW, RH + 8, "模型", fill=FILL_GREY, stroke=INK, size=17)
    out.append(arrow(MX + MW, cy, OX - 6, cy, INK, 2))
    out += cell(OX, y, OW, RH, "", "new")
    out.append(text(OX + OW / 2, cy + 6, WORDS[n], 16, INK, "bold"))
    return out


def append(row):
    """The token output in row `row` becomes the last input of the next row."""
    cy = centre(row)
    ox = OX + OW / 2
    tx = X0 + (N + row) * PITCH + CW / 2
    mid = cy + RP / 2
    return [elbow([(ox, cy + RH / 2 + 2), (ox, mid), (tx, mid),
                   (tx, cy + RP - RH / 2 - 4)], ORANGE, 2)]


def figure(rows):
    out = [text(OX + OW / 2, Y0 - 21, "输出", 13, MUTED, "bold")]
    for row in range(rows):
        if row:
            out += append(row - 1)
        out += step(row)
    return out


if __name__ == "__main__":
    for rows in (1, 2, 3):
        save(f"kv-generate-{rows}", W, H, figure(rows))
