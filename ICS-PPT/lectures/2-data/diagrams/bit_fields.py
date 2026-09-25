#!/usr/bin/env python3
"""One concrete FP32 value, decoded field by field.

The slide used to carry this as a monospace block with the three fields lined
up by spaces; at projector size the boundaries were invisible. Drawing the 32
cells puts the segment edges where the eye can find them, and leaves room for
the arithmetic that turns the fields into 3.1416. Writes ../assets/bit-fields.svg.
"""

import pathlib

import svgkit as k

W, H = 820, 172
CW, CH = 21.0, 32.0
SIGN, EXP, MAN = "0", "10000000", "10010010000111111011011"
X0 = (W - CW * 32) / 2
Y = 44


def build():
    out = []
    x = X0
    out.append(k.text(x + CW / 2, Y - 12, "符号 1 位", 11.5, k.MUTED))

    body, x = k.cells(x, Y, list(SIGN), CW, CH, k.FILL_GREY, k.MUTED)
    out += body
    x_exp = x
    body, x = k.cells(x, Y, list(EXP), CW, CH, k.FILL_BLUE, k.BLUE)
    out += body
    x_man = x
    body, x = k.cells(x, Y, list(MAN), CW, CH, k.FILL_ORANGE, k.ORANGE)
    out += body

    out += k.brace(x_exp, x_man, Y + CH + 6, k.BLUE, "阶码 8 位 · e = 128")
    out += k.brace(x_man, x, Y + CH + 6, k.ORANGE, "尾数 23 位 · M = 1.f = 1.5708")

    # two <text> elements: XML collapses runs of spaces, so a gap must be geometry
    out.append(k.text(W / 2 - 24, H - 22, "E = e − 127 = 1", 14.5, k.INK, "bold",
                      anchor="end"))
    out.append(k.text(W / 2 + 24, H - 22, "V = (−1)⁰ × 1.5708 × 2¹ = 3.1416",
                      14.5, k.INK, "bold", anchor="start"))
    return k.svg(W, H, out)


if __name__ == "__main__":
    p = pathlib.Path(__file__).resolve().parent.parent / "assets" / "bit-fields.svg"
    p.write_text(build(), encoding="utf-8")
    print(p)
