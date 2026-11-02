#!/usr/bin/env python3
"""The same 8 lines of cache arranged as 8 sets of 1 line (direct mapped), 4 sets of 2
lines (2-way) and 1 set of 8 lines (fully associative): S x E = C / B stays 8.
Run it to refresh ../assets/cache-compare.svg.
"""

from cachekit import *

W, H = 1000, 290
COMPACT_H = 226
LH, LW = 20, 190
PANELS = [("直接映射", 8, 1, 60), ("2 路组相联", 4, 2, 400), ("全相联", 1, 8, 740)]


def build(panels=None, *, compact=False):
    out = []
    line_height = 16 if compact else LH
    for title, S, E, x in PANELS if panels is None else panels:
        out.append(text(x + LW / 2 + 10, 22 if compact else 26, title,
                        19 if compact else 20, INK, "bold"))
        y = 38 if compact else 46
        for s in range(S):
            top = y
            for e in range(E):
                out.append(rect(x + 10, y, LW, line_height, FILL_BLUE, INK, rx=0, width=1.4))
                out.append(text(x + 10 + LW / 2, y + (12 if compact else 14.5),
                                "v  tag  块", 13, MUTED, font=MONO))
                y += line_height
            if E > 1:
                out.append(rect(x + 4, top - 4, LW + 12, y - top + 8, "none", LINE, rx=5,
                                width=1.4, dash="5 3"))
            if S > 1:
                out.append(text(x - 4, (top + y) / 2 + 5, f"组 {s}", 15, IDX, "bold", "end"))
            y += (8 if E > 1 else 3) if compact else (10 if E > 1 else 4)
        out.append(text(x + LW / 2 + 10, 217 if compact else 276,
                        f"S = {S}，E = {E}", 19, INK, "bold"))
    return out


if __name__ == "__main__":
    save("cache-compare", W, COMPACT_H, build(compact=True))
