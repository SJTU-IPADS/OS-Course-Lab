"""Shapes shared by the figures of the fourth and fifth parts.

A cache line is a row of cells [valid | (dirty) | tag | bytes] in the colours of
the cache-organisation figures: the line light blue with a dark outline, the tag
purple, the requested data orange, main memory green, a hit green, a miss red.
A module, not a figure: render.sh runs it and it writes nothing.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_PURPLE,
                    FILL_RED, GREEN, INK, LINE, MONO, MUTED, ORANGE, PURPLE, RED, WHITE,
                    arrow, box, brace, line, mono, rect, save, text, FONT)


def cells_row(x, y, spec, h=34, size=16):
    """A row of cells; spec is a list of (width, label, fill, stroke, colour, font).

    Returns (shapes, right edge)."""
    out = []
    cx = x
    for width, label, fill, stroke, colour, font in spec:
        out.append(rect(cx, y, width, h, fill, stroke, rx=0, width=1.4))
        if label:
            out.append(text(cx + width / 2, y + h / 2 + size * 0.36, label, size, colour,
                            "bold", font=font))
        cx += width
    return out, cx


def cache_line(x, y, nbytes=4, bw=46, dirty=None, valid=1, tag="标记", hot=None,
               hot_label="x", h=34, size=16):
    """A line: V, optional D, tag, nbytes byte cells; byte `hot` is the requested one.

    Returns (shapes, right edge, {name: (x0, x1)})."""
    spec = [(40, f"V={valid}", FILL_BLUE, INK, INK, MONO)]
    names = ["v"]
    if dirty is not None:
        spec.append((40, f"D={dirty}", FILL_BLUE, INK, RED if dirty else INK, MONO))
        names.append("d")
    spec.append((56, tag, FILL_PURPLE, PURPLE, PURPLE, FONT))
    names.append("tag")
    for k in range(nbytes):
        if k == hot:
            spec.append((bw, hot_label, FILL_ORANGE, ORANGE, INK, MONO))
        else:
            spec.append((bw, "", FILL_BLUE, INK, INK, MONO))
        names.append(f"b{k}")
    out, right = cells_row(x, y, spec, h, size)
    pos = {}
    cx = x
    for (width, *_), name in zip(spec, names):
        pos[name] = (cx, cx + width)
        cx += width
    out.append(rect(x, y, right - x, h, "none", INK, rx=0, width=2))
    return out, right, pos
