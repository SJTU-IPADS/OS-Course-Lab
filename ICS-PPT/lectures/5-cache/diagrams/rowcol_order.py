#!/usr/bin/env python3
"""Static 4 x 4 matrices coloured by row-wise or column-wise read order.

Colours denote read order, from pale to dark, rather than element values.
Run this script to refresh ../assets/rowcol-order-{row,col}.svg.
"""

from svgkit import MUTED, WHITE, rect, save, text
from traverse import M, N, colour, read_number


WIDTH, HEIGHT = 320, 180
CELL = 36
MATRIX_X, MATRIX_Y = (WIDTH - N * CELL) / 2, 4
KEY_X, KEY_Y, KEY_WIDTH, KEY_HEIGHT = MATRIX_X, 160, N * CELL, 10


def build(by_column):
    count = M * N
    out = []
    for row in range(M):
        for col in range(N):
            order = read_number(row, col, by_column)
            out.append(rect(MATRIX_X + col * CELL, MATRIX_Y + row * CELL,
                            CELL, CELL, colour(order, count), WHITE,
                            rx=2, width=1.5))

    for order in range(count):
        out.append(rect(KEY_X + order * KEY_WIDTH / count, KEY_Y,
                        KEY_WIDTH / count, KEY_HEIGHT, colour(order, count),
                        WHITE, rx=0, width=0.5))
    out.append(text(KEY_X - 12, KEY_Y + KEY_HEIGHT, "先", 14, MUTED,
                    anchor="end"))
    out.append(text(KEY_X + KEY_WIDTH + 12, KEY_Y + KEY_HEIGHT, "后", 14,
                    MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("rowcol-order-row", WIDTH, HEIGHT, build(False))
    save("rowcol-order-col", WIDTH, HEIGHT, build(True))
