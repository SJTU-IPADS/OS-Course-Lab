#!/usr/bin/env python3
"""Eight ints in a row, four bytes each, with the address of every element.

w[3] is picked out, with the 3 × 4 = 12 bytes between it and w_base.
Run it to refresh ../assets/array-address.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, MUTED, ORANGE, WHITE,
                    brace, line, mono, rect, save, text)

W, H = 1120, 190
X0, CW = 80, 120


def build():
    out = []
    for i in range(8):
        x = X0 + i * CW
        hit = i == 3
        out.append(rect(x, 50, CW, 56, FILL_ORANGE if hit else FILL_BLUE,
                        ORANGE if hit else BLUE, rx=2, width=1.6))
        for b in range(1, 4):                       # byte boundaries inside the int
            out.append(line(x + b * CW / 4, 94, x + b * CW / 4, 106, MUTED, 1))
        out.append(mono(x + CW / 2, 84, f"w[{i}]", 18, INK, "bold" if hit else "normal",
                        anchor="middle"))
        off = "w_base" if i == 0 else f"+{4 * i}"
        out.append(mono(x, 130, off, 15, ORANGE if hit else MUTED,
                        "bold" if hit else "normal", anchor="middle"))
        out.append(line(x, 106, x, 116, MUTED, 1.2))
    out.append(text(X0 + CW / 2, 36, "每个 int 占 4 字节", 15, MUTED))
    out += brace(X0 + 2, X0 + 3 * CW - 2, 146, ORANGE, "3 × 4 = 12 字节")
    return out


if __name__ == "__main__":
    save("array-address", W, H, build())
