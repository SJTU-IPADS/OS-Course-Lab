#!/usr/bin/env python3
"""The dot_product prototype with an arrow from each part to its register.

Token positions are computed from the monospace advance, so the arrows start
under the text they belong to.
Run it to refresh ../assets/param-binding.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, MONO_EM, MUTED, ORANGE,
                    WHITE, arrow, box, brace, mono, save, text)

W, H = 880, 154
SIZE = 22
PROTO = "int dot_product(const int *w, const int *x, int n)"
X0 = (W - len(PROTO) * SIZE * MONO_EM) / 2

PARTS = [("int", "%eax", "返回值：32 位整数", ORANGE, FILL_ORANGE, 20),
         ("const int *w", "%rdi", "参数 1：64 位指针", BLUE, FILL_BLUE, 240),
         ("const int *x", "%rsi", "参数 2：64 位指针", BLUE, FILL_BLUE, 460),
         ("int n", "%edx", "参数 3：32 位整数", BLUE, FILL_BLUE, 680)]


def span(token, start=0):
    i = PROTO.index(token, start)
    a = X0 + i * SIZE * MONO_EM
    return a, a + len(token) * SIZE * MONO_EM, i + len(token)


def build():
    out = [mono(X0, 40, PROTO, SIZE, INK, "bold")]
    pos = 0
    for token, reg, what, color, fill, bx in PARTS:
        a, b, pos = span(token, pos)
        out += brace(a + 1, b - 1, 50, color, "", depth=7)
        out.append(arrow((a + b) / 2, 60, bx + 90, 90, color, 2))
        out += box(bx, 94, 180, 52, reg, fill, color, 20, font="monospace", sub=what)
    return out


if __name__ == "__main__":
    save("param-binding", W, H, build())
