#!/usr/bin/env python3
"""The figure part-5.md asks for on its page 8: FP32, FP16 and BF16 side by side.

The three formats drawn to one bit scale, sign in blue, exponent in orange,
mantissa in green.  FP16's 5-bit exponent is boxed in red; BF16 lines up
with FP32's upper 16 bits (bit 31 down to bit 16), which the dashed guides
make visible.  Run it to refresh ../assets/fp16-bf16-fields.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import INK, MUTED, rect, svg, text

W, H = 920, 230
RED = "#c0392b"
SIGN = ("#cfe3ec", "#156082")
EXP = ("#f8d5c0", "#e97132")
MAN = ("#cfe8d2", "#196b24")

BIT, X0, RH = 22, 96, 30
ROWS = [("FP32", 8, 23, 44), ("FP16", 5, 10, 110), ("BF16", 8, 7, 170)]


def seg(x, y, bits, colors, label):
    fill, stroke = colors
    w = bits * BIT
    out = [rect(x, y, w, RH, fill, stroke, rx=3, width=1.5)]
    out.append(text(x + w / 2, y + RH / 2 + 5, label, 13 if w > 40 else 12, INK,
                    "bold" if w > 40 else "normal"))
    return out


def build():
    out = []
    for name, exp, man, y in ROWS:
        out.append(text(X0 - 14, y + RH / 2 + 5, name, 16, INK, "bold", "end"))
        out += seg(X0, y, 1, SIGN, "1")
        out += seg(X0 + BIT, y, exp, EXP, f"阶码 {exp}")
        out += seg(X0 + (1 + exp) * BIT, y, man, MAN, f"尾数 {man}")
        end = X0 + (1 + exp + man) * BIT
        out.append(text(end + 12, y + RH / 2 + 5, f"{1 + exp + man} 位", 13, MUTED,
                        anchor="start"))

    # bit numbers over FP32
    y32 = ROWS[0][3]
    for bit in (31, 23, 16, 0):
        x = X0 + (31 - bit) * BIT + BIT / 2
        out.append(text(x, y32 - 8, f"{bit}", 11.5, MUTED))

    # FP16's short exponent
    y16 = ROWS[1][3]
    out.append(rect(X0 + BIT - 4, y16 - 4, 5 * BIT + 8, RH + 8, "none", RED, rx=4, width=2.4))
    out.append(text(X0 + BIT + 5 * BIT + 12, y16 - 8, "阶码仅 5 位：最大值 65504", 13, RED,
                    "bold", "start"))

    # BF16 is FP32's upper 16 bits
    cut = X0 + 16 * BIT
    yb = ROWS[2][3]
    for x in (X0, cut):
        out.append(f'<line x1="{x:.1f}" y1="{y32 + RH:.1f}" x2="{x:.1f}" y2="{yb:.1f}" '
                   f'stroke="#156082" stroke-width="1.6" stroke-dasharray="5 4"/>')
    out.append(text(X0 + 8 * BIT, yb + RH + 22, "与 FP32 的高 16 位（位 31 至位 16）逐位对齐",
                    13, "#156082", "bold"))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "fp16-bf16-fields.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
