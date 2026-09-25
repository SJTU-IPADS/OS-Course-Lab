#!/usr/bin/env python3
"""Where quantization moves single-request decode on the RTX 5090 roofline.

The bandwidth slope and the compute ceiling meet at 209.5 TFLOP/s / 1.792 TB/s
= 117 FLOP/Byte.  Decode reads every weight once per token, so its arithmetic
intensity is 2 FLOP over the bytes per weight: BF16 at 1, 8-bit at 2 (the
paper's GPT-2), 4-bit at 4.  The arrow is BF16 -> 4-bit; all three points stay
on the slope, left of the ridge.  Sized for the empty band under the text of
memory-wall-cont.  Run it to refresh ../assets/roofline-shift.svg.
"""

import math
import pathlib

import svgkit as k

W, H = 760, 208

RED, GREEN, GOLD = "#c0392b", "#196b24", "#b8860b"

PEAK, BW = 209.5, 1.792                     # TFLOP/s and TB/s -> ridge at 117
I_MIN, DECADES = 0.5, 3.0                   # x axis: 0.5 .. 500 FLOP/Byte
LOG_LO, LOG_HI = -0.3, 2.5                  # y axis: attainable TFLOP/s, log10
X0, X1 = 76, 740
Y0, Y1 = 32, 166
AXIS_Y = 170


def px(i):
    return X0 + (math.log10(i) - math.log10(I_MIN)) / DECADES * (X1 - X0)


def py(v):
    return Y1 - (math.log10(v) - LOG_LO) / (LOG_HI - LOG_LO) * (Y1 - Y0)


def attainable(i):
    return min(PEAK, BW * i)


def arrow_head(x, y, color, dx, dy, size=9.0):
    """A small filled triangle pointing along (dx, dy)."""
    n = math.hypot(dx, dy)
    dx, dy = dx / n, dy / n
    ax, ay = -dy, dx
    pts = [(x, y),
           (x - dx * size + ax * size * 0.5, y - dy * size + ay * size * 0.5),
           (x - dx * size - ax * size * 0.5, y - dy * size - ay * size * 0.5)]
    d = " ".join(f"{a:.1f},{b:.1f}" for a, b in pts)
    return f'<polygon points="{d}" fill="{color}"/>'


def axes():
    out = [k.line(X0, AXIS_Y, X1, AXIS_Y, k.MUTED, 1.3),
           k.line(X0, AXIS_Y, X0, Y0 - 12, k.MUTED, 1.3)]
    for i in (1, 2, 4, 10, 100):
        x = px(i)
        out.append(k.line(x, AXIS_Y, x, AXIS_Y + 5, k.MUTED, 1.2))
        out.append(k.text(x, AXIS_Y + 19, str(i), 12, k.MUTED))
    for v in (1, 10, 100):
        y = py(v)
        out.append(k.line(X0 - 5, y, X0, y, k.MUTED, 1.2))
        out.append(k.text(X0 - 9, y + 4, str(v), 12, k.MUTED, anchor="end"))
    out.append(k.text((X0 + X1) / 2, AXIS_Y + 36, "算术强度 I (FLOP/Byte，对数)", 12.5, k.MUTED))
    out.append(k.text(X0 - 14, Y0 - 18, "可达算力 (TFLOP/s，对数)", 12.5, k.MUTED, anchor="start"))
    return out


def roof():
    ridge = PEAK / BW
    rx, ry = px(ridge), py(PEAK)
    out = [k.line(px(I_MIN), py(attainable(I_MIN)), rx, ry, k.BLUE, 2.6),
           k.line(rx, ry, X1, ry, k.BLUE, 2.6),
           k.line(rx, ry, rx, AXIS_Y, k.MUTED, 1.1, dash="4 4"),
           k.text(rx, ry - 12, "平衡点 ≈ 117 FLOP/Byte（RTX 5090）", 13, k.MUTED)]
    out.append(k.text(px(25), py(attainable(25)) + 34, "访存受限区", 13.5, k.BLUE, "bold"))
    out.append(k.text((rx + X1) / 2, ry + 28, "算力受限区", 13.5, k.BLUE, "bold"))
    return out


def shift():
    """Decode at three widths, and the BF16 -> 4-bit arrow above the slope."""
    points = ((1.0, "BF16", RED), (2.0, "8 位", GOLD), (4.0, "4 位", GREEN))
    out = []
    for i, lab, color in points:
        x, y = px(i), py(attainable(i))
        out.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="6" fill="{color}"/>')
        out.append(k.text(x + 4, y + 24, lab, 13, color, "bold"))

    ax, ay = px(1.0), py(attainable(1.0))
    bx, by = px(4.0), py(attainable(4.0))
    dx, dy = bx - ax, by - ay
    n = math.hypot(dx, dy)
    ox, oy = dy / n * 18, -dx / n * 18          # 18 px off the slope, above it
    out.append(k.line(ax + ox, ay + oy, bx + ox - dx / n * 8, by + oy - dy / n * 8,
                      GREEN, 2.8))
    out.append(arrow_head(bx + ox, by + oy, GREEN, dx, dy))
    mx, my = (ax + bx) / 2 + ox, (ay + by) / 2 + oy
    out.append(k.text(mx - 34, my - 36, "BF16 → 4 位", 13.5, GREEN, "bold"))
    out.append(k.text(mx - 34, my - 19, "访存量 ÷ 4，I × 4", 13, GREEN))
    return out


def build():
    return k.svg(W, H, axes() + roof() + shift())


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "roofline-shift.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
