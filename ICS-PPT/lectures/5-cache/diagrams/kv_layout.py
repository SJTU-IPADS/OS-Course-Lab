#!/usr/bin/env python3
"""The layout of the KV cache, cache[layer][kv][head][position][dim]: each row is the zoom
into one cell of the row above, down to the d elements of one position. The positions that
hold a token are light blue, the position the current step appends is orange, the rest of the
preallocated positions are empty. Run it to refresh ../assets/kv-layout.svg."""

from kv_kit import *

W, H = 1000, 366
X0, X1 = 130, 970                        # the extent of every row
RH, PITCH, Y0 = 38, 76, 20


def row(level, labels, selected, kinds=None, size=16):
    """One row of equal cells; returns (shapes, extent of the selected cell)."""
    out = []
    n = len(labels)
    cw = (X1 - X0) / n
    y = Y0 + level * PITCH
    names = ["[layer]", "[kv]", "[head]", "[pos]", "[dim]"]
    out.append(mono(X0 - 16, y + RH / 2 + 5, names[level], 16, MUTED, "bold", "end"))
    sel = None
    for k, lab in enumerate(labels):
        x = X0 + k * cw
        kind = (kinds or {}).get(k, "cache")
        out += cell(x + 2, y, cw - 4, RH, lab, kind, size=size, weight="normal")
        if k == selected:
            out.append(rect(x + 2, y, cw - 4, RH, "none", INK, rx=4, width=2.8))
            sel = (x + 2, x + cw - 2)
    return out, sel


def figure():
    rows = [
        (["layer 0", "...", "layer l", "...", "layer L-1"], 2, None),
        (["K", "V"], 0, None),
        (["head 0", "head 1", "...", "head n-1"], 1, None),
        (["pos 0", "pos 1", "...", "pos t-1", "pos t", "empty", "empty", "...", "MAX-1"], 4,
         {4: "new", 5: "empty", 6: "empty", 7: "empty", 8: "empty"}),
        (["d0", "d1", "d2", "...", "d127"], 1, None),
    ]
    out = []
    prev = None
    for level, (labels, selected, kinds) in enumerate(rows):
        shapes, sel = row(level, labels, selected, kinds)
        if prev is not None:
            y0 = Y0 + (level - 1) * PITCH + RH
            y1 = Y0 + level * PITCH
            out.append(line(prev[0], y0 + 2, X0, y1 - 2, LINE, 1.4))
            out.append(line(prev[1], y0 + 2, X1, y1 - 2, LINE, 1.4))
        out += shapes
        prev = sel
    out.append(text(X1, Y0 + 3 * PITCH + RH + 17, "已占用的位置", 14, MUTED, "normal", "end"))
    return out


if __name__ == "__main__":
    save("kv-layout", W, H, figure())
