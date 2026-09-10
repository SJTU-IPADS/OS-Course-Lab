#!/usr/bin/env python3
"""How finely the scale is shared, and what it costs.

The same 256 weights three times, drawn as eight rows of thirty-two. What
changes is how many of them share one scale: the whole tensor, one super-block,
or one row of thirty-two. The number under each panel is the extra storage
that choice costs per weight; the last line is the error measured on real
weights by ../examples/quant_compare.c.
Run it to refresh ../assets/granularity.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, MONO, MUTED, ORANGE,
                    line, rect, svg, text)

W, H = 1000, 284
CELL = 8                                   # one weight
COLS, ROWS = 32, 8                         # 256 weights per panel
GRID_W, GRID_H = COLS * CELL, ROWS * CELL
PANEL_X = (40, 360, 680)
GRID_Y = 96
CHIP_W, CHIP_H = 30, 15


def chip(x, y):
    """One stored scale."""
    return [rect(x, y, CHIP_W, CHIP_H, FILL_ORANGE, ORANGE, rx=2, width=1.2),
            text(x + CHIP_W / 2, y + CHIP_H - 3, "d", 12, ORANGE, font=MONO)]


def panel(x, title, groups, cost, err):
    """One grid; `groups` is how many rows share a scale (0 = the whole tensor)."""
    out = [text(x + GRID_W / 2, 62, title, 15.5, INK, "bold")]
    for r in range(ROWS):
        for c in range(COLS):
            out.append(rect(x + c * CELL, GRID_Y + r * CELL, CELL, CELL,
                            FILL_BLUE, "#c8dae4", rx=0.8, width=0.5))

    if groups == 0:                        # one scale, shared beyond this panel
        out.append(rect(x - 3, GRID_Y - 3, GRID_W + 6, GRID_H + 6, "none", ORANGE,
                        rx=3, width=1.6, dash="4 3"))
        out += chip(x + GRID_W + 14, GRID_Y + GRID_H / 2 - CHIP_H / 2)
        out.append(line(x + GRID_W + 4, GRID_Y + GRID_H / 2, x + GRID_W + 12,
                        GRID_Y + GRID_H / 2, ORANGE, 1.2))
        out.append(text(x + GRID_W / 2, GRID_Y + GRID_H + 22,
                        "整个张量共用一个", 13.5, MUTED))
    elif groups == 1:                      # one scale for these 256
        out.append(rect(x - 3, GRID_Y - 3, GRID_W + 6, GRID_H + 6, "none", ORANGE,
                        rx=3, width=1.6))
        out += chip(x + GRID_W + 14, GRID_Y + GRID_H / 2 - CHIP_H / 2)
        out.append(text(x + GRID_W / 2, GRID_Y + GRID_H + 22, "每 256 个一个", 13.5, MUTED))
    else:                                  # one scale per row of 32
        for r in range(ROWS):
            out.append(rect(x - 2, GRID_Y + r * CELL - 1, GRID_W + 4, CELL + 2,
                            "none", ORANGE, rx=2, width=1.1))
            out.append(rect(x + GRID_W + 14, GRID_Y + r * CELL, 20, CELL,
                            FILL_ORANGE, ORANGE, rx=1.5, width=1))
        out.append(text(x + GRID_W + 24, GRID_Y - 10, "8 个 d", 12.5, ORANGE, "bold"))
        out.append(text(x + GRID_W / 2, GRID_Y + GRID_H + 22, "每 32 个一个", 13.5, MUTED))

    out.append(text(x + GRID_W / 2, GRID_Y + GRID_H + 52, cost, 15, BLUE, "bold"))
    out.append(text(x + GRID_W / 2, GRID_Y + GRID_H + 74, err, 15, ORANGE, "bold"))
    return out


def build():
    body = [text(W / 2, 30, "同样的 256 个权重，三种共用缩放系数的方式", 17.5, INK, "bold"),
            text(W / 2, 266, "误差为 quant_compare 在 4096 个真实权重上测得的相对 RMSE",
                 13.5, MUTED)]
    body += panel(PANEL_X[0], "per-tensor", 0, "+0.00 位/权重", "13.50%")
    body += panel(PANEL_X[1], "per-256", 1, "+0.06 位/权重", "11.15%")
    body += panel(PANEL_X[2], "per-32（Q4_0）", 8, "+0.50 位/权重", "8.42%")
    return svg(W, H, body)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "granularity.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
