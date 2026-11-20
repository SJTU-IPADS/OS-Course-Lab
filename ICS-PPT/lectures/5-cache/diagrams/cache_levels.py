#!/usr/bin/env python3
"""Access time of L1, L2, L3 and memory, with the ratio between neighbours.

Four boxes in a row, caches light blue and memory green, each wider than the
one before it to hint at the larger capacity. An arrow between two neighbours
carries the ratio of their access times, and a long arrow under the row
carries the ratio of memory to L1. Adjacent levels differ by less than one
order of magnitude, L1 and memory by about two.

The numbers are AMD Zen 4 (Ryzen 9 7950X, DDR5-6000) measured by Chips and
Cheese, "AMD's Zen 4, Part 2: Memory Subsystem and Conclusion",
https://chipsandcheese.com/p/amds-zen-4-part-2-memory-subsystem-and-conclusion
The ratios are computed here from the constants and rounded to one decimal.
Run it to refresh ../assets/cache-levels.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import FILL_BLUE, FILL_GREEN, GREEN, INK, arrow, elbow, rect, save, text

L1_NS = 0.7       # L1D: 4 cycles, "just 0.7 ns"
L2_NS = 2.44      # L2: 14 cycles, the article prints 2.44 ns
L3_NS = 8.5       # L3: "the 8-9 ns range"; printed as 9 ns
DRAM_NS = 73.35   # memory: 73.35 ns with a 1 GB test, DDR5-6000

# name, printed time, time for the ratio, fill, outline, box width
LEVELS = [
    ("L1", "约 0.7 ns", L1_NS, FILL_BLUE, INK, 120),
    ("L2", "约 2.4 ns", L2_NS, FILL_BLUE, INK, 140),
    ("L3", "约 9 ns", L3_NS, FILL_BLUE, INK, 165),
    ("内存", "约 73 ns", DRAM_NS, FILL_GREEN, GREEN, 195),
]

W, H = 1000, 168
GAP = 106                                  # between two boxes, holds one arrow
TOP, BOX_H = 14, 72
BELOW = TOP + BOX_H + 34                   # height of the long arrow under the row


def build():
    left = (W - sum(level[5] for level in LEVELS) - GAP * (len(LEVELS) - 1)) / 2
    out, edges = [], []
    x = left
    for name, shown, _, fill, stroke, width in LEVELS:
        out.append(rect(x, TOP, width, BOX_H, fill, stroke, rx=6, width=1.8))
        out.append(text(x + width / 2, TOP + 30, name, 20, INK, "bold"))
        out.append(text(x + width / 2, TOP + 58, shown, 18, INK))
        edges.append((x, x + width))
        x += width + GAP
    mid_y = TOP + BOX_H / 2
    for k in range(len(LEVELS) - 1):
        x0, x1 = edges[k][1] + 10, edges[k + 1][0] - 10
        ratio = LEVELS[k + 1][2] / LEVELS[k][2]
        out.append(arrow(x0, mid_y, x1, mid_y, INK, 2))
        out.append(text((x0 + x1) / 2, mid_y - 12, f"×{ratio:.1f}", 19, INK, "bold"))
    first, last = sum(edges[0]) / 2, sum(edges[-1]) / 2
    bottom = TOP + BOX_H
    out.append(elbow([(first, bottom), (first, BELOW), (last, BELOW), (last, bottom + 2)],
                     INK, 2))
    total = round(LEVELS[-1][2] / LEVELS[0][2], -2)
    out.append(text((first + last) / 2, BELOW + 26, f"约 ×{total:.0f}", 19, INK, "bold"))
    return out


if __name__ == "__main__":
    save("cache-levels", W, H, build())
