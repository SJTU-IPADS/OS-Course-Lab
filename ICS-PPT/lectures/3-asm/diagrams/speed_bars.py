#!/usr/bin/env python3
"""Three generation speeds as bars on one linear scale.

The memory-bandwidth ceiling of the machine (51.2 GB/s / 0.968 GB = 53
tokens a second), ollama on one thread (10.7) and mini-ollama on one thread
(0.77). Linear, so the last bar is a sliver: that is the point of the page.
Measured 2026-10-05 on the i9-11900H; see measured_speed() in pages.py.
Run it to refresh ../assets/speed-bars.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_ORANGE, FILL_RED, GREEN, INK, MUTED, ORANGE,
                    RED, line, rect, save, text)

W, H = 1120, 158
X_AXIS, BAR, ROW, GAP, TOP = 262, 700, 30, 16, 22

ROWS = [("内存带宽上限", 53.0, "53 Token/s", RED, FILL_RED),
        ("ollama，1 个线程", 10.7, "10.7 Token/s", GREEN, FILL_GREEN),
        ("mini-ollama，1 个线程", 0.77, "0.77 Token/s（ollama 的 1/14）", ORANGE, FILL_ORANGE)]


def build():
    out = []
    for k, (label, value, shown, color, fill) in enumerate(ROWS):
        y = TOP + k * (ROW + GAP)
        w = BAR * value / ROWS[0][1]
        out.append(text(X_AXIS - 12, y + ROW / 2 + 6, label, 16, INK, anchor="end"))
        out.append(rect(X_AXIS, y, w, ROW, fill, color, rx=3, width=1.6))
        out.append(text(X_AXIS + w + 12, y + ROW / 2 + 6, shown, 17, color, "bold",
                        anchor="start"))
    bottom = TOP + len(ROWS) * ROW + (len(ROWS) - 1) * GAP
    out.append(line(X_AXIS, TOP - 6, X_AXIS, bottom + 6, MUTED, 1.4))
    return out


if __name__ == "__main__":
    save("speed-bars", W, H, build())
