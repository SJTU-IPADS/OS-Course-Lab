#!/usr/bin/env python3
"""Compact storage hierarchy for a slide with software-cache examples.

Adapted from this lecture's storage_levels.py and hierarchy.py, which match
the sources under ../ics-github-main/ICS-PPT/lectures/5-cache/diagrams/.
Retain their widening rows, shared palette and dashed remote-storage border.
Omit numerical latency and capacity estimates so the figure describes the
general trend without assigning mechanisms to the operating system.

Run this script to refresh ../assets/cache-software-hierarchy.svg.
"""

from svgkit import (
    BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, GREEN, INK, MUTED,
    arrow, rect, save, text,
)


W, H = 460, 380
CENTER_X = 274
TOP, ROW_HEIGHT, ROW_PITCH = 18, 50, 65
LEVELS = [
    ("寄存器", 140, FILL_GREY, INK, None),
    ("硬件缓存", 188, FILL_BLUE, BLUE, None),
    ("内存", 236, FILL_GREEN, GREEN, None),
    ("SSD / 本地文件", 284, FILL_GREY, MUTED, None),
    ("远程存储", 332, FILL_GREY, MUTED, "7 5"),
]


def build():
    out = []
    for index, (label, width, fill, stroke, dash) in enumerate(LEVELS):
        y = TOP + index * ROW_PITCH
        out.append(rect(CENTER_X - width / 2, y, width, ROW_HEIGHT,
                        fill, stroke, rx=6, width=1.8, dash=dash))
        out.append(text(CENTER_X, y + ROW_HEIGHT / 2 + 7,
                        label, 20, INK, "bold"))

    axis_x = 52
    out.append(arrow(axis_x, 68, axis_x, 327, MUTED, 2.2))
    out.append(text(axis_x, 26, "访问更快", 16, MUTED, "bold"))
    out.append(text(axis_x, 47, "容量更小", 16, MUTED, "bold"))
    out.append(text(axis_x, 350, "访问更慢", 16, MUTED, "bold"))
    out.append(text(axis_x, 371, "容量更大", 16, MUTED, "bold"))
    out.append(text(CENTER_X, 360, "容量与访问时间的总体趋势", 16, MUTED))
    return out


if __name__ == "__main__":
    save("cache-software-hierarchy", W, H, build())
