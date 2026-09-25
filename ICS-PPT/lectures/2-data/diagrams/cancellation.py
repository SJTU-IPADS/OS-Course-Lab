#!/usr/bin/env python3
"""The figure part-5.md asks for on its page 5: what a subtraction leaves.

Two 24-bit significands that agree in their first 21 bits and differ only in
the last 3, the bits that already carry rounding error.  The difference keeps
just those 3 bits; normalising shifts them to the front and fills the 21
places behind them with zeros that carry no information.  (part-5.md's
sketch says "e.g. the first 20 bits"; 21 is what makes 24 fall to 3 exactly.)
Run it to refresh ../assets/cancellation.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE,
                    brace, line, rect, svg, text)

W, H = 820, 222
RED = "#c0392b"

SHARED = "101101011001011100101"      # 21 bits, leading 1
A_TAIL, B_TAIL, D_TAIL = "111", "010", "101"
LABEL_X, X0, CW, CH = 150, 162, 18, 24
END = X0 + 24 * CW
NOTE_X = END + 16

ROW_A, ROW_B, ROW_D, ROW_N = 40, 70, 112, 172


def row(y, bits, fills, label):
    out = [text(LABEL_X, y + CH / 2 + 5, label, 14, INK, "bold", "end")]
    for i, (b, (fill, stroke, tone)) in enumerate(zip(bits, fills)):
        x = X0 + i * CW
        out.append(rect(x, y, CW, CH, fill, stroke))
        out.append(text(x + CW / 2, y + CH / 2 + 4.5, b, 12, tone,
                        font="SFMono-Regular, Menlo, Consolas, monospace"))
    return out


def build():
    blue = (FILL_BLUE, BLUE, INK)
    orange = (FILL_ORANGE, ORANGE, ORANGE)
    grey = (FILL_GREY, LINE, MUTED)
    out = []
    out += brace(X0, X0 + 21 * CW, ROW_A - 6, BLUE, "前 21 位完全相同", 12.5, below=False)
    out += brace(X0 + 21 * CW, END, ROW_A - 6, ORANGE, "含舍入误差", 12.5, below=False)
    out += row(ROW_A, SHARED + A_TAIL, [blue] * 21 + [orange] * 3, "a")
    out += row(ROW_B, SHARED + B_TAIL, [blue] * 21 + [orange] * 3, "b")
    rule = ROW_D - 8
    out.append(line(X0 - 6, rule, END, rule, INK, 1.4))
    out.append(text(LABEL_X - 26, ROW_B + CH / 2 + 6, "−", 18, INK, "bold"))
    out += row(ROW_D, "0" * 21 + D_TAIL, [grey] * 21 + [orange] * 3, "a − b")
    out.append(text(NOTE_X, ROW_D + CH / 2 + 5, "高 21 位全部抵消", 13, MUTED, "bold", "start"))

    # normalising: the 3 surviving bits move 21 places to the front
    x0, y0 = X0 + 22.5 * CW, ROW_D + CH
    x1, y1 = X0 + 1.5 * CW, ROW_N
    out.append(f'<path d="M {x0:.1f} {y0 + 2:.1f} C {x0:.1f} {y0 + 26:.1f}, '
               f'{x1:.1f} {y1 - 26:.1f}, {x1:.1f} {y1 - 7:.1f}" fill="none" '
               f'stroke="{ORANGE}" stroke-width="2"/>')
    out.append(f'<polygon points="{x1:.1f},{y1 - 1:.1f} {x1 - 5:.1f},{y1 - 9:.1f} '
               f'{x1 + 5:.1f},{y1 - 9:.1f}" fill="{ORANGE}"/>')
    out.append(text(NOTE_X, (y0 + y1) / 2 + 5, "规格化：左移 21 位", 13, ORANGE, "bold", "start"))
    out += row(ROW_N, D_TAIL + "0" * 21, [orange] * 3 + [grey] * 21, "规格化后")
    out.append(text(X0 + 13.5 * CW, ROW_N + CH + 18, "补入的 0，不含原数据的信息", 12, MUTED))
    out.append(text(NOTE_X, ROW_N + CH / 2 + 5, "有效精度：24 位 → 3 位", 14, RED, "bold", "start"))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "cancellation.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
