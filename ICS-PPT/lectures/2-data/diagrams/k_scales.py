#!/usr/bin/env python3
"""Sixteen 6-bit numbers packed into twelve bytes.

Q4_K stores eight sub-block scales and eight sub-block mins, 6 bits each:
16 x 6 = 96 bits = exactly 12 bytes, with nothing left over. The packing is
llama.cpp's: the first eight bytes each hold one whole 6-bit value in their
low six bits, and lend their two spare top bits to the high half of one of
the last four values, whose low nibbles live in the last four bytes.
Bit 7 is drawn on the left of each byte.
Run it to refresh ../assets/k-scales.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, MONO, MUTED, ORANGE,
                    rect, svg, text)

W, H = 928, 270
BW, BH = 130, 44                       # one byte
GAP = 2
X0, ROW_A, ROW_B = 88, 66, 158
BIT = BW / 8


def byte_box(x, y, fields):
    """`fields` is a list of (bits, label, colour) drawn from bit 7 down."""
    out = []
    cx = x
    for bits, label, fill, stroke in fields:
        w = bits * BIT
        out.append(rect(cx, y, w, BH, fill, stroke, rx=2, width=1.2))
        out.append(text(cx + w / 2, y + BH / 2 + 4, label, 11.5, INK, font=MONO))
        out.append(text(cx + w / 2, y + BH - 6, f"{bits} 位", 9.5, MUTED))
        cx += w
    return out


def row(y, first, boxes, caption):
    out = [text(X0 - 14, y + BH / 2 + 4, f"字节 {first}–{first + 5}", 12, MUTED,
                anchor="end")]
    for i, fields in enumerate(boxes):
        x = X0 + i * (BW + GAP)
        out.append(text(x + BW / 2, y - 8, str(first + i), 11, MUTED, font=MONO))
        out += byte_box(x, y, fields)
    out.append(text(X0 + 3 * (BW + GAP), y + BH + 22, caption, 12.5, MUTED))
    return out


def build():
    sc = (FILL_BLUE, BLUE)
    mn = (FILL_ORANGE, ORANGE)

    top = []
    for j in range(4):                                   # bytes 0..3
        top.append([(2, f"sc{j + 4}高", *sc), (6, f"sc{j}", *sc)])
    for j in range(2):                                   # bytes 4..5
        top.append([(2, f"m{j + 4}高", *mn), (6, f"m{j}", *mn)])

    bot = []
    for j in range(2, 4):                                # bytes 6..7
        bot.append([(2, f"m{j + 4}高", *mn), (6, f"m{j}", *mn)])
    for j in range(4, 8):                                # bytes 8..11
        bot.append([(4, f"m{j}低", *mn), (4, f"sc{j}低", *sc)])

    body = [text(W / 2, 30, "scales[12]：8 个 6 位 scale 与 8 个 6 位 min", 15.5, INK,
                 "bold")]
    body += row(ROW_A, 0, top, "前八个字节各放一个完整的 6 位值，空出的两位借给后四个")
    body += row(ROW_B, 6, bot, "后四个字节各放两个 4 位的低半段，与上面借出的两位拼成 6 位")
    body.append(text(W / 2, 258, "16 × 6 = 96 位 = 12 字节，没有一位剩余", 13, INK, "bold"))
    return svg(W, H, body)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "k-scales.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
