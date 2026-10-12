#!/usr/bin/env python3
"""int W[3][4] in memory: three rows of four ints, one row after another.

A band of 12 cells, 4 bytes each, low addresses on the left. The brackets
above name the three rows, each an array of 4 ints; the numbers below are
offsets from the start of W, the bold ones the start of a row. One element,
W[1][2], is outlined and its offset written out with the formula of the page:
16 * 1 + 4 * 2 = 24.

The cells are orange, the colour of array data in the lecture's stack figures.
Run it to refresh ../assets/nested-array.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_ORANGE, FONT, INK, MONO, MUTED, ORANGE, _advance, arrow,
                    label_line, line, mono, rect, save, text)

W, H = 1120, 214
CELL = 80                           # one int
CX = (W - 12 * CELL) / 2            # the band is centred on the canvas
CY, CH = 62, 54
PICK = (1, 2)                       # the element whose address is worked out


def centred(cx, y, parts, size=16):
    """A line of (text, font, fill, weight) pieces centred on cx."""
    width = sum(_advance(s, size, f) for s, f, _, _ in parts)
    return label_line(cx - width / 2, y, parts, size)


def build():
    out = []
    for k in range(12):
        r, j = divmod(k, 4)
        x = CX + k * CELL
        out.append(rect(x, CY, CELL, CH, FILL_ORANGE, ORANGE, rx=2, width=1.4))
        out.append(mono(x + CELL / 2, CY + CH / 2 + 5, f"W[{r}][{j}]", 15, INK, "bold",
                        anchor="middle"))
        start = j == 0
        out.append(mono(x, CY + CH + 22, str(4 * k), 15, INK if start else MUTED,
                        "bold" if start else "normal", anchor="middle"))
    out.append(mono(CX + 12 * CELL, CY + CH + 22, "48", 15, MUTED, anchor="middle"))
    out.append(text(CX - 26, CY + CH + 22, "偏移", 14, MUTED, anchor="end"))
    # the three rows
    for r in range(3):
        x0, x1 = CX + 4 * r * CELL, CX + 4 * (r + 1) * CELL
        out.append(line(x0, CY, x0, CY + CH, INK, 2.4))
        d = f"M {x0 + 4:.1f} {CY - 6} L {x0 + 4:.1f} {CY - 14} L {x1 - 4:.1f} {CY - 14} L {x1 - 4:.1f} {CY - 6}"
        out.append(f'<path d="{d}" fill="none" stroke="{INK}" stroke-width="1.6"/>')
        out += centred((x0 + x1) / 2, CY - 24,
                       [(f"W[{r}]", MONO, INK, "bold"), ("，16 字节", FONT, INK, "bold")])
    out.append(line(CX + 12 * CELL, CY, CX + 12 * CELL, CY + CH, INK, 2.4))
    # low and high addresses
    out.append(text(CX - 14, CY + CH / 2 + 5, "低地址", 14, MUTED, anchor="end"))
    out.append(text(CX + 12 * CELL + 14, CY + CH / 2 + 5, "高地址", 14, MUTED,
                    anchor="start"))
    # one element and its offset
    r, j = PICK
    x = CX + (4 * r + j) * CELL
    out.append(rect(x, CY, CELL, CH, "none", INK, rx=2, width=3))
    out.append(arrow(x + CELL / 2, CY + CH + 62, x + CELL / 2, CY + CH + 6, INK, 2))
    out += centred(x + CELL / 2, CY + CH + 84,
                   [(f"W[{r}][{j}]", MONO, INK, "bold"), (" 的偏移：", FONT, INK, "bold"),
                    (f"16 × {r} + 4 × {j} = {16 * r + 4 * j}", MONO, INK, "bold")])
    return out


if __name__ == "__main__":
    save("nested-array", W, H, build())
