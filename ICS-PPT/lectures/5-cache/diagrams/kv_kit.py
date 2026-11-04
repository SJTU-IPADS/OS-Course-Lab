"""Shared parts of the KV cache figures.

Every figure of the sixth part draws the same entities in the same colours:
an entry stored in the KV cache is light blue, the data computed in the current
step (the new token) is orange, an entry read from the cache instead of being
computed again is green, a repeated computation or a wasted slot is red, and a
slot that is reserved but empty is white with a dashed outline. A model parameter
(a weight matrix) is purple, and a vector that is neither cached nor new is grey.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_PURPLE, FILL_RED,
                    GREEN, INK, LINE, MUTED, ORANGE, PURPLE, RED, WHITE, arrow, brace, elbow,
                    line, mono, rect, save, text, vbrace)

# kind -> (fill, stroke)
KINDS = {
    "new": (FILL_ORANGE, ORANGE),
    "cache": (FILL_BLUE, BLUE),
    "reuse": (FILL_GREEN, GREEN),
    "waste": (FILL_RED, RED),
    "empty": (WHITE, LINE),
    "gone": (FILL_GREY, LINE),
    "mid": (FILL_GREY, MUTED),
    "param": (FILL_PURPLE, PURPLE),
}


def cell(x, y, w, h, label="", kind="cache", size=14, width=1.6, weight="bold"):
    """One entry: a rounded box in the colour of its kind, mono label centred."""
    fill, stroke = KINDS[kind]
    dash = "5 4" if kind in ("empty", "gone") else None
    out = [rect(x, y, w, h, fill, stroke, rx=4, width=width, dash=dash)]
    if label:
        tone = MUTED if kind in ("empty", "gone") else INK
        out.append(mono(x + w / 2, y + h / 2 + size * 0.35, label, size, tone, weight,
                        anchor="middle"))
    return out


def legend(x, y, items, size=15, gap=22):
    """A row of (kind, label) swatches starting at x; returns the shapes."""
    out = []
    for kind, label in items:
        fill, stroke = KINDS[kind]
        dash = "5 4" if kind in ("empty", "gone") else None
        out.append(rect(x, y - 13, 24, 17, fill, stroke, rx=3, width=1.4, dash=dash))
        out.append(text(x + 32, y + 1, label, size, MUTED, anchor="start"))
        x += 32 + sum(1.0 if ord(c) > 0x2e80 else 0.55 for c in label) * size + gap
    return out


def legend_width(items, size=15, gap=22):
    return sum(32 + sum(1.0 if ord(c) > 0x2e80 else 0.55 for c in label) * size + gap
               for _, label in items) - gap
