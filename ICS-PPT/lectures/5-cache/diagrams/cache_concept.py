#!/usr/bin/env python3
"""Level k caches level k+1: blocks, and the copies of blocks the cache holds.

Level k+1 (grey) holds 16 numbered blocks, level k (blue) holds four of them
(3, 8, 9, 14), each slot a copy of one block of the level below. Data moves
between the two levels in blocks. cache_hit_miss.py imports the drawing code
and adds the request.
Run it to refresh ../assets/cache-concept.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE, arrow, line,
                    mono, rect, save, text)

W, H = 1000, 330
BX, BW, BGAP = 110, 51, 2.5               # level k+1: first block, block width, gap
LOW_Y, BH = 214, 44                       # level k+1: top of its blocks
HIGH_Y = 62                               # level k: top of its slots
SLOT_W = 70
SLOTS = [3, 8, 9, 14]
SLOT_X = [290 + i * (SLOT_W + 20) for i in range(4)]
BLOCK_CAPTION = "第 k+1 层：16 个块"


def low_x(n):
    return BX + n * (BW + BGAP)


def slot_mid(i):
    return SLOT_X[i] + SLOT_W / 2


def levels(slots=SLOTS, fills=None, strokes=None, hot_low=None, links=True):
    """Both levels. fills/strokes: {slot index: colour}; hot_low: block number in orange."""
    fills = fills or {}
    strokes = strokes or {}
    out = []
    out.append(text(BX - 14, HIGH_Y + 28, "第 k 层", 18, INK, "bold", "end"))
    out.append(text(BX - 14, HIGH_Y + 50, "缓存", 15, MUTED, "normal", "end"))
    out.append(text(BX - 14, LOW_Y + 30, "第 k+1 层", 18, INK, "bold", "end"))
    # links from each slot to the block it copies
    if links:
        for i, n in enumerate(slots):
            out.append(line(slot_mid(i), HIGH_Y + BH, low_x(n) + BW / 2, LOW_Y, LINE, 1.6, "5 4"))
    for n in range(16):
        hot = n == hot_low
        out.append(rect(low_x(n), LOW_Y, BW, BH, FILL_ORANGE if hot else FILL_GREY,
                        ORANGE if hot else MUTED, rx=3, width=2 if hot else 1.3))
        out.append(mono(low_x(n) + BW / 2, LOW_Y + BH / 2 + 6, str(n), 17, INK,
                        "bold" if hot else "normal", "middle"))
    for i, n in enumerate(slots):
        out.append(rect(SLOT_X[i], HIGH_Y, SLOT_W, BH, fills.get(i, FILL_BLUE),
                        strokes.get(i, INK), rx=3, width=2.2 if i in strokes else 1.6))
        out.append(mono(slot_mid(i), HIGH_Y + BH / 2 + 6, str(n), 17, INK, "bold", "middle"))
    return out


def build():
    out = levels()
    out.append(arrow(60, 220, 60, 124, MUTED, 2, both=True))
    out.append(text(80, 174, "以块为单位传输", 15, MUTED, "bold", "start"))
    out.append(text(SLOT_X[0] - 6, HIGH_Y - 14, "4 个块", 15, MUTED, "bold", "start"))
    out.append(text(BX, LOW_Y + BH + 28, "16 个块", 15, MUTED, "bold", "start"))
    return out


if __name__ == "__main__":
    save("cache-concept", W, 310, build())
