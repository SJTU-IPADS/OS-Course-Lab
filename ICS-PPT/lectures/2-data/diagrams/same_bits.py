#!/usr/bin/env python3
"""One bit pattern, two declared types, two values.

The strip is drawn once on purpose: the claim is that nothing about the bits
changes, so there must not be two strips to compare. Only the reading above
and the reading below differ. Writes ../assets/same-bits.svg.
"""

import pathlib

import svgkit as k

W, H = 760, 232
BITS = "1100111111000111"          # 0xcfc7
CW, CH = 32.0, 36.0
X0 = (W - CW * len(BITS)) / 2
Y = 96


def reading(y, fill, stroke, head, note):
    box_w, box_h = 360, 50
    x = (W - box_w) / 2
    out = [k.rect(x, y, box_w, box_h, fill, stroke, rx=5),
           k.text(W / 2, y + 21, head, 15, k.INK, "bold"),
           k.text(W / 2, y + 40, note, 12.5, k.MUTED)]
    return out


def build():
    out = []
    out += reading(20, k.FILL_BLUE, k.BLUE, "short y  =  −12345", "最高位的权重 −2¹⁵")
    out.append(k.line(W / 2, 70, W / 2, Y, k.BLUE, 1.4))

    body, right = k.cells(X0, Y, list(BITS), CW, CH, "#ffffff", k.LINE, size=13)
    out += body
    # the one cell whose weight differs between the two readings
    out.append(k.rect(X0, Y, CW, CH, "none", k.INK, width=2.6))
    out.append(k.text(right + 12, Y + CH / 2 + 5, "0xcfc7", 14, k.INK, "bold",
                      "start", k.MONO))

    out.append(k.line(W / 2, Y + CH, W / 2, Y + CH + 26, k.ORANGE, 1.4))
    out += reading(Y + CH + 26, k.FILL_ORANGE, k.ORANGE,
                   "unsigned short uy  =  53191", "最高位的权重 +2¹⁵")
    return k.svg(W, H, out)


if __name__ == "__main__":
    p = pathlib.Path(__file__).resolve().parent.parent / "assets" / "same-bits.svg"
    p.write_text(build(), encoding="utf-8")
    print(p)
