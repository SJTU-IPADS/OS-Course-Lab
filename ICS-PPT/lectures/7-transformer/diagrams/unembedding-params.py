#!/usr/bin/env python3
"""unembedding-params: the weights of W_U, entered into GPT-3's running tally.

One frame. Left: W_U drawn to its proportions (50,257 rows, 12,288 columns,
the shape of W_E turned over) and the product of its sides. Right: the
tally with the embedding and unembedding rows filled, unembedding in focus.
"""
from tk import *

H = 340
ROWS, COLS = 50_257, 12_288


def main():
    b = []
    cell, nr, nc = 7, 36, 10
    gx, gy = 200, 58
    b += weights(gx, gy, nr, nc, cell, cell, seed="WU-params")
    gw, gh = nc * cell, nr * cell
    b += brace(gx, gx + gw, gy - 6, INK, f"{COLS:,}", size=19, below=False)
    b += vbrace(gx - 8, gy, gy + gh, INK, f"{ROWS:,}", size=19, right=False)
    b += math(gx + gw + 24, gy + gh / 2 - 40, "W_U", 30, anchor="start")
    b.append(text(gx + gw + 24, gy + gh / 2 + 6, f"{ROWS:,} × {COLS:,}", 21, INK, "bold",
                  anchor="start", font=MONO))
    b.append(text(gx + gw + 24, gy + gh / 2 + 38, f"= {ROWS * COLS:,}", 21, INK, "bold",
                  anchor="start", font=MONO))
    els, th = tally(590, 11, filled={0, 7}, w=420, focus={7})
    b += els
    save("unembedding-params", b, H)


main()
