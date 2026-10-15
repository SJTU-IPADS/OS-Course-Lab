#!/usr/bin/env python3
"""The perf stat numbers of the page as bars: instructions and elapsed time.
The figure sits under the perf output on the same page, so it is low: the
ratio follows each title on its line.

dot_scalar: 2,459,562,893 instructions, 0.150 s; dot_avx2: 310,762,907
instructions, 0.024 s (n = 4096, 100000 calls).
Run it to refresh ../assets/perf-bars.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, GREEN, INK, LINE, MUTED, line,
                    rect, save, text)

W, H = 1120, 118
X_BAR, BAR, ROW, GAP, TOP = 150, 280, 30, 10, 42


def group(x0, title, rows, top, note):
    """Two horizontal bars that share the axis x0 + X_BAR; the longer one is BAR
    wide. The note follows the title on the same line, so the figure stays low."""
    axis, bottom = x0 + X_BAR, TOP + len(rows) * ROW + (len(rows) - 1) * GAP
    out = [text(axis, 26, title, 18, INK, "bold", anchor="start"),
           text(axis + len(title) * 18 + 16, 26, note, 16, MUTED, anchor="start")]
    for k, (label, value, shown, color, fill) in enumerate(rows):
        y = TOP + k * (ROW + GAP)
        w = BAR * value / top
        out.append(text(axis - 12, y + ROW / 2 + 6, label, 16, INK, anchor="end"))
        out.append(rect(axis, y, w, ROW, fill, color, rx=3, width=1.6))
        out.append(text(axis + w + 12, y + ROW / 2 + 6, shown, 18, color, "bold",
                        anchor="start"))
    out.append(line(axis, TOP - 6, axis, bottom + 6, MUTED, 1.4))
    return out


def build():
    out = group(4, "指令数", [("dot_scalar", 2.4596, "2.46 G", BLUE, FILL_BLUE),
                               ("dot_avx2", 0.3108, "0.31 G", GREEN, FILL_GREEN)],
                2.4596, "约 1/8")
    out += group(554, "执行耗时", [("dot_scalar", 0.150, "0.150 s", BLUE, FILL_BLUE),
                                  ("dot_avx2", 0.024, "0.024 s", GREEN, FILL_GREEN)],
                 0.150, "约 1/6")
    out.append(line(W / 2, 8, W / 2, H - 8, LINE, 1, "4 4"))
    return out


if __name__ == "__main__":
    save("perf-bars", W, H, build())
