#!/usr/bin/env python3
"""The three shifts, on one 8-bit pattern.

Each row shows what moved and, in the accent colour, what was fed in at the
open end. Logical and arithmetic right shift differ only in those two cells.
Run it to refresh ../assets/shifts.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, MONO, MUTED, ORANGE,
                    cells, svg, text)

W, H = 800, 262
CW, CH = 36, 30
LX, X0 = 198, 210
X = "10110010"

ROWS = [
    ("x",                 X,          (),     "0xb2"),
    ("x << 2",            "11001000", (6, 8), "0xc8"),
    ("x >> 2  逻辑右移",   "00101100", (0, 2), "0x2c"),
    ("x >> 2  算术右移",   "11101100", (0, 2), "0xec"),
]


def build():
    out = [text(W / 2, 30, "移位：移出的位丢弃，开口的一端补入新位", 15.5, INK, "bold")]
    for i, (label, bits, fed, hexv) in enumerate(ROWS):
        y = 54 + i * 44
        out.append(text(LX, y + CH / 2 + 5, label, 13.5,
                        INK, "bold", anchor="end"))
        lo = range(*fed) if fed else ()
        for j, b in enumerate(bits):
            hot = j in lo
            out += cells(X0 + j * CW, y, [b], CW, CH,
                         FILL_ORANGE if hot else FILL_BLUE,
                         ORANGE if hot else BLUE, 13.5)[0]
        out.append(text(X0 + 8 * CW + 16, y + CH / 2 + 5, hexv, 13, MUTED,
                        anchor="start", font=MONO))
    out.append(text(W / 2, H - 14,
                    "补入的位由类型决定：无符号数补 0，补码补符号位", 13, MUTED))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "shifts.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
