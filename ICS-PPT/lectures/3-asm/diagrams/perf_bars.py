#!/usr/bin/env python3
"""The perf stat numbers of the page as bars: instructions and elapsed time.

dot_scalar: 2,459,562,893 instructions, 0.150 s; dot_avx2: 310,762,907
instructions, 0.024 s (n = 4096, 100000 calls).
Run it to refresh ../assets/perf-bars.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, GREEN, INK, LINE, MUTED, line,
                    rect, save, text)

W, H = 1120, 196
X_BAR, BAR, ROW, GAP, TOP = 150, 280, 40, 18, 46


def group(x0, title, rows, top, note):
    """Two horizontal bars that share the axis x0 + X_BAR; the longer one is BAR wide."""
    axis, bottom = x0 + X_BAR, TOP + len(rows) * ROW + (len(rows) - 1) * GAP
    out = [text(axis, 28, title, 18, INK, "bold", anchor="start")]
    for k, (label, value, shown, color, fill) in enumerate(rows):
        y = TOP + k * (ROW + GAP)
        w = BAR * value / top
        out.append(text(axis - 12, y + ROW / 2 + 6, label, 16, INK, anchor="end"))
        out.append(rect(axis, y, w, ROW, fill, color, rx=3, width=1.6))
        out.append(text(axis + w + 12, y + ROW / 2 + 6, shown, 18, color, "bold",
                        anchor="start"))
    out.append(line(axis, TOP - 8, axis, bottom + 8, MUTED, 1.4))
    out.append(text(axis, bottom + 36, note, 16, MUTED, anchor="start"))
    return out


def build():
    out = group(40, "指令数", [("dot_scalar", 2.4596, "2.46 G", BLUE, FILL_BLUE),
                               ("dot_avx2", 0.3108, "0.31 G", GREEN, FILL_GREEN)],
                2.4596, "约 1/8（比值 7.91）")
    out += group(590, "执行耗时", [("dot_scalar", 0.150, "0.150 s", BLUE, FILL_BLUE),
                                  ("dot_avx2", 0.024, "0.024 s", GREEN, FILL_GREEN)],
                 0.150, "约 1/6.25")
    out.append(line(570, 14, 570, H - 14, LINE, 1, "4 4"))
    return out


if __name__ == "__main__":
    save("perf-bars", W, H, build())
