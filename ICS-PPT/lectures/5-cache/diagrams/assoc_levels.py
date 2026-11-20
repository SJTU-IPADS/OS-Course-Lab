#!/usr/bin/env python3
"""The hit time of L1 and L3 split into comparing the tags and reading the data.

Two groups, L1 and L3, each with one bar for a small and one for a large
associativity E. A bar is the tag comparison (purple, the colour of the tag)
followed by reading out the data (orange, the colour of the requested data).
All four bars share one horizontal scale, so the share of the comparison is
drawn and never written. The legend sits in the blank space right of the
short L1 bars, which keeps the figure low. The cycle counts are teaching
values, not measurements: comparing takes 1 cycle for a small E and 2 for a
large E at every level, reading the data takes 3 cycles in L1 and 48 in L3.
Run it to refresh ../assets/assoc-levels.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import FILL_ORANGE, FILL_PURPLE, INK, ORANGE, PURPLE, rect, save, text

COMPARE = {"E 较小": 1, "E 较大": 2}       # cycles to compare the tags of one set
READ = {"L1": 3, "L3": 48}                 # cycles to read the data out of the line

W, H = 1000, 166
START, UNIT = 146, 15                      # bar origin; px per cycle, one scale
BAR_H, BAR_GAP, GROUP_GAP, TOP = 28, 8, 22, 8
LEGEND_W = 2 * (30 + 4 * 18) + 28          # two swatches with their labels


def legend(x, y):
    """Swatches of the two parts, laid left to right from x; y is the baseline."""
    out = []
    for label, fill, stroke in (("比较标记", FILL_PURPLE, PURPLE),
                                ("读出数据", FILL_ORANGE, ORANGE)):
        out.append(rect(x, y - 15, 22, 18, fill, stroke, rx=0, width=1.4))
        out.append(text(x + 30, y, label, 18, INK, anchor="start"))
        x += 30 + 4 * 18 + 28
    return out


def build():
    out = legend(START + 50 * UNIT - LEGEND_W, TOP + BAR_H + BAR_GAP / 2 + 7)
    top = TOP
    for level, read in READ.items():
        group_mid = top + BAR_H + BAR_GAP / 2
        out.append(text(24, group_mid + 7, level, 20, INK, "bold", "start"))
        for label, compare in COMPARE.items():
            mid = top + BAR_H / 2 + 6
            compare_w, read_w = compare * UNIT, read * UNIT
            out.append(text(START - 12, mid, label, 18, INK, anchor="end"))
            out.append(rect(START, top, compare_w, BAR_H, FILL_PURPLE, PURPLE, rx=0,
                            width=1.4))
            out.append(rect(START + compare_w, top, read_w, BAR_H, FILL_ORANGE, ORANGE,
                            rx=0, width=1.4))
            out.append(text(START + compare_w + read_w + 10, mid + 1,
                            f"{compare + read} 周期", 19, INK, "bold", "start"))
            top += BAR_H + BAR_GAP
        top += GROUP_GAP - BAR_GAP
    return out


if __name__ == "__main__":
    save("assoc-levels", W, H, build())
