#!/usr/bin/env python3
"""The 144 bytes that hold 256 quantized weights.

Above: the super-block is eight ordinary blocks of thirty-two, each with its
own scale and min, so the levels still follow the data closely.
Below: those sixteen numbers are not stored as sixteen fp16 values (32 bytes)
but as sixteen 6-bit integers against two fp16 super-block scales (16 bytes),
which is what brings the format back to Q4_0's 4.5 bits per weight.
The field widths are drawn to scale, so `qs` really is 128 of the 144 bytes.
Run it to refresh ../assets/q4-k-block.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MONO,
                    MUTED, ORANGE, line, rect, svg, text)

W, H = 1000, 276
SUB_X, SUB_Y, SUB_W, SUB_H = 66, 58, 106, 44    # eight sub-blocks
BYTE = 6.0                                     # px per byte in the bottom strip
BAR_X, BAR_Y, BAR_H = 66, 182, 38


def sub_blocks():
    out = []
    for j in range(8):
        x = SUB_X + j * (SUB_W + 4)
        out.append(rect(x, SUB_Y, SUB_W, SUB_H, FILL_BLUE, BLUE))
        out.append(text(x + SUB_W / 2, SUB_Y + 16, f"子块 {j}", 13.5, INK))
        out.append(text(x + SUB_W / 2, SUB_Y + 32, "32 个权重", 13, MUTED))
        out.append(rect(x + 5, SUB_Y + SUB_H + 8, 46, 18, FILL_ORANGE, ORANGE, rx=2,
                        width=1.1))
        out.append(text(x + 28, SUB_Y + SUB_H + 21, "sc 6位", 11.5, ORANGE, font=MONO))
        out.append(rect(x + 55, SUB_Y + SUB_H + 8, 46, 18, FILL_ORANGE, ORANGE, rx=2,
                        width=1.1))
        out.append(text(x + 78, SUB_Y + SUB_H + 21, "m 6位", 11.5, ORANGE, font=MONO))
    return out


def field(x, nbytes, label, fill, stroke, inside=True):
    w = nbytes * BYTE
    out = [rect(x, BAR_Y, w, BAR_H, fill, stroke)]
    if inside:
        out.append(text(x + w / 2, BAR_Y + BAR_H / 2 + 5, label, 14, INK, font=MONO))
    return out, x + w


def build():
    body = [text(W / 2, 30, "block_q4_K：一组 256 个权重占 144 字节", 17.5, INK, "bold")]
    body += sub_blocks()

    x = BAR_X
    a, x = field(x, 2, "", FILL_ORANGE, ORANGE, inside=False)
    b, x = field(x, 2, "", FILL_ORANGE, ORANGE, inside=False)
    c, x = field(x, 12, "scales", FILL_ORANGE, ORANGE)
    d, x = field(x, 128, "qs　128 字节　256 个 4 位编码", FILL_BLUE, BLUE)
    body += a + b + c + d

    # d and dmin are 11 px wide each: name them above with leader lines
    for i, name in enumerate(("d", "dmin")):
        cx = BAR_X + (i + 0.5) * 2 * BYTE
        body.append(line(cx, BAR_Y - 4, cx, BAR_Y - 14 - i * 14, ORANGE, 1.1))
        body.append(text(cx - 4 if i == 0 else cx, BAR_Y - 18 - i * 14,
                         f"{name}　2 字节", 13, ORANGE, "bold",
                         anchor="end" if i == 0 else "start"))
    body.append(text(BAR_X + 16 * BYTE + 6 * BYTE, BAR_Y + BAR_H + 18,
                     "12 字节　8 个 6 位 scale 与 8 个 6 位 min", 13, MUTED,
                     anchor="start"))

    for j in (0, 7):                        # two of the eight arrows, for clarity
        x0 = SUB_X + j * (SUB_W + 4) + SUB_W / 2
        body.append(line(x0, SUB_Y + SUB_H + 28, BAR_X + 16 * BYTE + 6 * BYTE,
                         BAR_Y - 4, LINE, 1.2, "4 4"))

    body.append(text(W / 2, 264,
                     "144 字节 = 2 + 2 + 12 + 128",
                     15, INK, "bold"))
    return svg(W, H, body)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "q4-k-block.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
