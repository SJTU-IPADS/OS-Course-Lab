#!/usr/bin/env python3
"""Widening keeps the value; narrowing may not.

Two rows of byte cells: -12345 as a short widened to an int, and 53191 as an
int narrowed to a short. The cells that appear and the cells that vanish are
the whole content of the rules.
Run it to refresh ../assets/expand-truncate.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED,
                    ORANGE, brace, cells, line, svg, text)

W, H = 866, 246
CW, CH = 46, 32
LX, X0 = 168, 180          # LX: right edge of the row label
TONE = {"hot": (FILL_ORANGE, ORANGE, INK), "gone": (FILL_GREY, LINE, MUTED)}


def strip(x, y, values, marked=(), how="hot"):
    """A run of byte cells; the marked ones are drawn in the accent tone."""
    out = []
    for i, v in enumerate(values):
        fill, stroke, tone = TONE[how] if i in marked else (FILL_BLUE, BLUE, INK)
        out += cells(x + i * CW, y, [v], CW, CH, fill, stroke, 13, tone=tone)[0]
    return out


def arrow(x, y):
    return [line(x, y, x + 37, y, MUTED, 1.6),
            f'<path d="M {x + 44:.1f} {y:.1f} L {x + 36:.1f} {y - 4.5:.1f} '
            f'L {x + 36:.1f} {y + 4.5:.1f} Z" fill="{MUTED}"/>']


def row(y, label, left, right, span, note, how="hot", tail=""):
    """One conversion, drawn left to right, with span=(lo, hi) braced below."""
    out = [text(LX, y + CH / 2 + 5, label, 13.5, INK, "bold", anchor="end")]
    marked = set(range(*span))
    on_left = how == "gone"
    out += strip(X0, y, left, marked if on_left else (), how)
    edge = X0 + len(left) * CW
    out += arrow(edge + 14, y + CH / 2)
    out += strip(edge + 72, y, right, () if on_left else marked, how)
    if tail:
        out.append(text(edge + 86 + len(right) * CW, y + CH / 2 + 5, tail, 13.5,
                        ORANGE, "bold", anchor="start"))
    bx = X0 if on_left else edge + 72
    out += brace(bx + span[0] * CW, bx + span[1] * CW, y + CH + 4,
                 MUTED if on_left else ORANGE, note)
    return out


def build():
    out = [text(W / 2, 30, "改变宽度：补入的位与丢弃的位", 15.5, INK, "bold")]
    out += row(52, "short −12345  →  int",
               ["cf", "c7"], ["ff", "ff", "cf", "c7"], (0, 2),
               "补入的两个字节是符号位", tail="数值不变，仍是 −12345")
    out += row(146, "int 53191  →  short",
               ["00", "00", "cf", "c7"], ["cf", "c7"], (0, 2),
               "高位直接丢弃", how="gone", tail="数值变成 −12345")
    out.append(text(W / 2, H - 14,
                    "扩展补入的位由原类型的符号性决定；截断丢弃的位不参与新的数值",
                    13, MUTED))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "expand-truncate.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
