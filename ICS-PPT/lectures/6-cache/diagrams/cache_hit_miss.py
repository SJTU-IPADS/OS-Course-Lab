#!/usr/bin/env python3
"""A hit and a miss in a cache of four blocks, in three frames.

Same two levels as cache_concept.py (imported). The cache holds blocks 3, 8, 9
and 14 of the 16 blocks of the level below.
  1. read block 14: the block is in the cache, a hit (green);
  2. read block 12: not in the cache, a miss (red); block 12 is fetched from
     the level below (orange);
  3. block 12 is placed in the cache and replaces block 8.
Run it to refresh ../assets/cache-hit-miss-1.svg .. cache-hit-miss-3.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from cache_concept import (BH, BW, HIGH_Y, LOW_Y, SLOT_W, SLOT_X, SLOTS, low_x, levels,
                           slot_mid)
from svgkit import (FILL_GREEN, FILL_ORANGE, FILL_RED, GREEN, INK, MUTED, ORANGE, RED, arrow,
                    path, rect, save, text)

W, H = 1000, 340
CAPTION_Y = 316
REQ_X = slot_mid(3)


def request(label, color):
    return [arrow(REQ_X, 14, REQ_X, HIGH_Y - 6, color, 3),
            text(REQ_X + 14, 30, label, 17, color, "bold", "start")]


def frame(k):
    out = []
    if k == 1:
        out += levels(fills={3: FILL_GREEN}, strokes={3: GREEN})
        out += request("读块 14", ORANGE)
        cap, color = "命中：块 14 在第 k 层，直接从第 k 层读出", GREEN
    elif k == 2:
        out += levels(hot_low=12)
        out.append(rect(SLOT_X[0] - 10, HIGH_Y - 8, SLOT_X[3] + SLOT_W - SLOT_X[0] + 20, BH + 16,
                        "none", RED, rx=6, width=2.2, dash="7 5"))
        out += request("读块 12", ORANGE)
        out.append(path(f"M {low_x(12) + BW / 2:.1f} {LOW_Y - 2} L {low_x(12) + BW / 2:.1f} "
                        f"{HIGH_Y + BH + 14}", ORANGE, 3))
        cap, color = "不命中：块 12 不在第 k 层，从第 k+1 层取出", RED
    else:
        slots = [3, 12, 9, 14]
        out += levels(slots=slots, fills={1: FILL_ORANGE}, strokes={1: ORANGE}, hot_low=12)
        out.append(text(slot_mid(1), HIGH_Y - 12, "替换块 8", 15, ORANGE, "bold"))
        cap, color = "块 12 放入第 k 层，替换块 8", ORANGE
    out.append(text(W / 2, CAPTION_Y, cap, 18, INK if k == 3 else color, "bold"))
    return out


if __name__ == "__main__":
    for k in (1, 2, 3):
        save(f"cache-hit-miss-{k}", W, H, frame(k))
