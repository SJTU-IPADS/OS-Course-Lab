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

W, H = 1120, 360
BASE = 300


def group(x0, title, rows, top, note):
    out = [text(x0 + 200, 30, title, 18, INK, "bold")]
    for k, (label, value, shown, color, fill) in enumerate(rows):
        x = x0 + 60 + k * 190
        h = 230 * value / top
        out.append(rect(x, BASE - h, 110, h, fill, color, rx=3, width=1.6))
        out.append(text(x + 55, BASE - h - 10, shown, 18, color, "bold"))
        out.append(text(x + 55, BASE + 26, label, 16, INK))
    out.append(line(x0 + 20, BASE, x0 + 400, BASE, MUTED, 1.4))
    out.append(text(x0 + 200, BASE + 54, note, 16, MUTED))
    return out


def build():
    out = group(40, "指令数", [("dot_scalar", 2.4596, "2.46 G", BLUE, FILL_BLUE),
                               ("dot_avx2", 0.3108, "0.31 G", GREEN, FILL_GREEN)],
                2.4596, "约 1/8（比值 7.91）")
    out += group(640, "执行耗时", [("dot_scalar", 0.150, "0.150 s", BLUE, FILL_BLUE),
                                  ("dot_avx2", 0.024, "0.024 s", GREEN, FILL_GREEN)],
                 0.150, "约 1/6.25")
    out.append(line(560, 20, 560, 340, LINE, 1, "4 4"))
    return out


if __name__ == "__main__":
    save("perf-bars", W, H, build())
