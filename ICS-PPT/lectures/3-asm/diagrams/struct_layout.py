#!/usr/bin/env python3
"""Structs as bands of bytes: where each field starts and how long it is.

Every band is drawn to scale, one byte a fixed number of pixels, offset 0 on
the left. The numbers under a band are offsets from the first byte of the
struct. A field is orange, the colour of data in the lecture's stack figures;
bytes no field uses (a gap between two fields, padding after the last one) are
white with a dashed edge and say how many they are.

  tensor-layout.svg   struct tensor of examples/tensor.h: 24 bytes, no gap
  align-layout.svg    the two structs of examples/align.c that add a 1-byte
                      field: 7 bytes after it, either as a gap or as padding
  node-layout.svg     struct Node of examples/node.c, the exercise: a 4-byte
                      gap before the pointer, 7 bytes of padding at the end

Sizes and offsets are those examples/align.c prints and gcc uses in the
listings of node.c.
Run it to refresh the three files in ../assets/.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE, arrow, line, mono,
                    rect, save, text)

BH = 42                             # height of a band
POINTER = 66                        # width a pointer adds on the left of a band


def band(x, y, px, fields):
    """One struct. fields are (size, name); a name that is not ASCII labels a
    run of unused bytes (间隙, 填充).

    A field name too long for its cell is written above the cell.
    """
    out, off = [], 0
    for size, name in fields:
        w = size * px
        if not name.isascii():
            out.append(rect(x, y, w, BH, WHITE, LINE, rx=2, width=1.6, dash="5 4"))
            out.append(text(x + w / 2, y + BH / 2 + 5, f"{name}，{size} 字节", 14, MUTED))
        else:
            out.append(rect(x, y, w, BH, FILL_ORANGE, ORANGE, rx=2, width=1.4))
            if len(name) * 17 * 0.6 + 8 <= w:
                out.append(mono(x + w / 2, y + BH / 2 + 6, name, 17, INK, "bold",
                                anchor="middle"))
            else:
                out.append(mono(x + w / 2, y - 16, name, 17, INK, "bold", anchor="middle"))
                out.append(line(x + w / 2, y - 11, x + w / 2, y - 2, INK, 1.4))
        out.append(mono(x, y + BH + 20, str(off), 15, MUTED, anchor="middle"))
        x += w
        off += size
    out.append(mono(x, y + BH + 20, str(off), 15, INK, "bold", anchor="middle"))
    return out


def pointer(x, y, name):
    """A pointer to the first byte of the band at (x, y)."""
    return [arrow(x - 56, y + BH / 2, x - 5, y + BH / 2, INK, 2.2),
            mono(x - 64, y + BH / 2 + 5, name, 17, INK, "bold", anchor="end")]


def title(x, y, name):
    """The name of a struct, ending at x: the left end is where `kind` is labelled."""
    return mono(x, y, name, 16, INK, "bold", anchor="end")


def tensor():
    w, h, px = 900, 88, 30
    x = (w - 24 * px) / 2 + POINTER / 2
    out = band(x, 14, px, [(8, "rows"), (8, "cols"), (8, "data")])
    out += pointer(x, 14, "w")
    return w, h, out


def align():
    w, h, px = 1120, 214, 30
    x = (w - 32 * px) / 2
    right = x + 32 * px
    out = [title(right, 24, "struct kind_first")]
    out += band(x, 36, px, [(1, "kind"), (7, "间隙"), (8, "rows"), (8, "cols"),
                            (8, "data")])
    out.append(title(right, 134, "struct kind_last"))
    out += band(x, 146, px, [(8, "rows"), (8, "cols"), (8, "data"), (1, "kind"),
                             (7, "填充")])
    return w, h, out


def node():
    # 40 bytes: at 24 pixels a byte the band and its pointer fit the page width
    w, h, px = 1120, 108, 24
    x = (w - 40 * px) / 2 + POINTER / 2
    out = band(x, 36, px, [(8, "name"), (4, "val[0]"), (4, "val[1]"), (4, "val[2]"),
                           (4, "间隙"), (8, "next"), (1, "flags"), (7, "填充")])
    out += pointer(x, 36, "p")
    return w, h, out


if __name__ == "__main__":
    for name, fig in (("tensor-layout", tensor), ("align-layout", align),
                      ("node-layout", node)):
        w, h, body = fig()
        save(name, w, h, body)
