#!/usr/bin/env python3
"""embedding-params: the weights of W_E, entered into GPT-3's running tally.

One frame. Left: W_E drawn to its proportions (12,288 rows, 50,257 columns)
and the product of its sides. Right: the tally with the embedding row
filled and in focus.
"""
from tk import *

H = 340
ROWS, COLS = 12_288, 50_257


def main():
    b = []
    cell, nr, nc = 10, 10, 38
    gx, gy = 96, 120
    b += weights(gx, gy, nr, nc, cell, cell, seed="WE-params")
    gw, gh = nc * cell, nr * cell
    b += brace(gx, gx + gw, gy - 8, INK, f"{COLS:,}", size=19, below=False)
    b += vbrace(gx + gw + 8, gy, gy + gh, INK, f"{ROWS:,}", size=19)
    b += math(gx - 14, gy + gh / 2 + 10, "W_E", 30, anchor="end")
    b.append(text(gx + gw / 2, gy + gh + 62, f"{ROWS:,} × {COLS:,} = {ROWS * COLS:,}", 21,
                  INK, "bold", font=MONO))
    els, th = tally(590, 11, filled={0}, w=420, focus={0})
    b += els
    save("embedding-params", b, H)


main()
