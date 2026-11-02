#!/usr/bin/env python3
"""Fetch adjacent matrix elements together, then read them from the cache.

The four-element block is illustrative. Both steps share the same storage
layout and colour code. Run this script to refresh ../assets/spatial-cache.svg.
"""

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK,
                    LINE, MUTED, ORANGE, arrow, line, mono, rect, save, text)


WIDTH, HEIGHT = 1000, 220
CELL_WIDTH, CELL_HEIGHT = 82, 40
CACHE_Y, MEMORY_Y = 72, 170
LEFT_X, RIGHT_X = 124, 642


def block(x, y, cache, requested=None):
    out = []
    fill, stroke = (FILL_BLUE, BLUE) if cache else (FILL_GREEN, GREEN)
    for col in range(4):
        current = col == requested
        out.append(rect(x + col * CELL_WIDTH, y, CELL_WIDTH, CELL_HEIGHT,
                        FILL_ORANGE if current else fill,
                        ORANGE if current else stroke, rx=2,
                        width=2 if current else 1.4))
        out.append(mono(x + (col + 0.5) * CELL_WIDTH,
                        y + CELL_HEIGHT / 2 + 6, f"W[i][{col}]", 17,
                        INK, "bold" if current else "normal", "middle"))
    out.append(text(x - 12, y + CELL_HEIGHT / 2 + 6,
                    "缓存（快）" if cache else "内存（慢）",
                    17, MUTED, anchor="end"))
    return out


def build():
    out = [text(250, 22, "① 读 W[i][0]，取入相邻数据", 19, INK, "bold"),
           text(750, 22, "② 访问相邻数据", 19, INK, "bold"),
           line(500, 12, 500, 216, LINE, 1.2)]
    out += block(LEFT_X, CACHE_Y, cache=True, requested=0)
    out += block(LEFT_X, MEMORY_Y, cache=False, requested=0)
    middle = LEFT_X + 2 * CELL_WIDTH
    out.append(arrow(middle, MEMORY_Y - 4, middle, CACHE_Y + CELL_HEIGHT + 5,
                     ORANGE, 3))
    out.append(text(middle + 18, 147, "整块复制", 18, ORANGE, "bold", "start"))

    out += block(RIGHT_X, CACHE_Y, cache=True, requested=1)
    out += block(RIGHT_X, MEMORY_Y, cache=False)
    requested_x = RIGHT_X + 1.5 * CELL_WIDTH
    out.append(arrow(requested_x, CACHE_Y - 4, requested_x, 38, ORANGE, 3))
    out.append(text(requested_x + 18, 55, "返回 W[i][1]", 17, ORANGE,
                    "bold", "start"))
    out.append(text(RIGHT_X + 2 * CELL_WIDTH, 147, "相邻元素已在缓存中", 18,
                    BLUE, "bold"))
    return out


if __name__ == "__main__":
    save("spatial-cache", WIDTH, HEIGHT, build())
