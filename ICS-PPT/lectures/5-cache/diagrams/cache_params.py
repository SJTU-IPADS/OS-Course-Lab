#!/usr/bin/env python3
"""What the parameters printed by cacheinfo.sh mean: a cache is a grid of lines, `sets` rows
by `ways` columns, and every line holds one block of `line size` bytes, so the capacity is
sets x ways x line size. The grid is schematic (the ellipsis columns and rows stand for the
rest); the numbers are those of the L1 data cache in the demo output above it (64 sets,
12 ways, 64 B lines, 48 KB). The tag and the valid bit are not drawn.
Drawn at the width it is shown on the slide (1000 px).
Run it to refresh ../assets/cache-params.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, MUTED, ORANGE, WHITE, brace, line, rect,
                    save, text, vbrace)

W, H = 1000, 236
GX, GY = 150, 38                          # top left of the grid
CW, DOT, RH = 74, 40, 34                  # line width, ellipsis column width, row height
COLS = ["路 0", "路 1", "…", "路 E−1"]
ROWS = ["组 0", "组 1", "⋮", "组 S−1"]


def col_x(c):
    return GX + sum(DOT if COLS[k] == "…" else CW for k in range(c))


def frame():
    out = []
    right = col_x(len(COLS))
    for c, name in enumerate(COLS):
        width = DOT if name == "…" else CW
        out.append(text(col_x(c) + width / 2, GY - 10, name, 16, MUTED, "bold"))
    for r, name in enumerate(ROWS):
        y = GY + r * RH
        out.append(text(GX - 12, y + RH / 2 + 6, name, 16, MUTED, "bold", "end"))
        for c, cname in enumerate(COLS):
            x = col_x(c)
            if name == "⋮" or cname == "…":
                if name == "⋮" and cname != "…":
                    out.append(text(x + CW / 2, y + RH / 2 + 6, "⋮", 18, MUTED))
                elif cname == "…" and name != "⋮":
                    out.append(text(x + DOT / 2, y + RH / 2 + 6, "…", 18, MUTED))
                continue
            first = r == 0 and c == len(COLS) - 1
            out.append(rect(x + 2, y + 2, CW - 4, RH - 4, FILL_ORANGE if first else FILL_BLUE,
                            ORANGE if first else BLUE, rx=3, width=2.4 if first else 1.4))
            out.append(text(x + CW / 2, y + RH / 2 + 6, "行", 16, INK))
    bottom = GY + len(ROWS) * RH
    out += brace(col_x(0) + 2, right - 2, bottom + 6, MUTED, "路数：每组的行数", 16, 8)
    out += vbrace(GX - 56, GY + 2, bottom - 2, MUTED, "组数", 16, 8, right=False)

    zx, zy, zw, zh = 540, 38, 330, 44          # one line, zoomed
    out.append(line(right - 2, GY + 2, zx, zy, ORANGE, 1.6, dash="5 4"))
    out.append(line(right - 2, GY + RH - 2, zx, zy + zh, ORANGE, 1.6, dash="5 4"))
    out.append(rect(zx, zy, zw, zh, FILL_BLUE, ORANGE, rx=4, width=2.4))
    out.append(text(zx + zw / 2, zy + zh / 2 + 6, "一个块", 18, INK, "bold"))
    out += brace(zx + 2, zx + zw - 2, zy + zh + 6, MUTED, "行长：块的大小，64 B", 16, 8)
    out.append(text(zx, zy - 12, "一行保存内存中的一个块", 16, ORANGE, "bold", "start"))

    out.append(rect(zx, 150, zw, 66, WHITE, MUTED, rx=6, width=1.4))
    out.append(text(zx + zw / 2, 176, "容量 = 组数 × 路数 × 行长", 18, INK, "bold"))
    out.append(text(zx + zw / 2, 204, "L1 数据缓存：64 × 12 × 64 B = 48 KB", 17, MUTED))
    return out


if __name__ == "__main__":
    save("cache-params", W, H, frame())
