#!/usr/bin/env python3
"""Ford's moving assembly line: four stations, four cars, one step per beat.

Each station does one job; the car moves on. The station working on car 1
is the last; car 4 has just entered. Run it to refresh
../assets/assembly-line.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE,
                    MUTED, ORANGE, arrow, box, rect, save, text)

W, H = 1120, 230
STATIONS = ["装车架", "装发动机", "装车轮", "装车身"]
CARS = ["第 4 辆", "第 3 辆", "第 2 辆", "第 1 辆"]
TONES = [(FILL_BLUE, BLUE), (FILL_ORANGE, ORANGE), (FILL_GREEN, GREEN), (FILL_GREY, INK)]
SW, SH, GAP = 190, 58, 60
X0 = (W - (4 * SW + 3 * GAP)) / 2
YS = 36                       # stations
YB = 130                      # the belt
BELT_H = 50


def build():
    out = [text(X0 - 12, YS + SH / 2 + 6, "工位", 15, MUTED, anchor="end"),
           text(X0 - 12, YB + BELT_H / 2 + 6, "传送带", 15, MUTED, anchor="end")]
    for i, (name, (fill, stroke)) in enumerate(zip(STATIONS, TONES)):
        x = X0 + i * (SW + GAP)
        out += box(x, YS, SW, SH, name, fill, stroke, 17, "bold", sub=f"工位 {i + 1}：只做这一道工序",
                   sub_size=12)
        out.append(arrow(x + SW / 2, YS + SH + 2, x + SW / 2, YB - 4, MUTED, 1.4))
    out.append(rect(X0 - 30, YB, 4 * SW + 3 * GAP + 60, BELT_H, "#f4f6f8", LINE, rx=6, width=1.2))
    for i, (car, (fill, stroke)) in enumerate(zip(CARS, TONES)):
        x = X0 + i * (SW + GAP)
        out += box(x + 30, YB + 8, SW - 60, BELT_H - 16, car, fill, stroke, 15, "bold")
        if i < 3:
            out.append(arrow(x + SW - 26, YB + BELT_H / 2, x + SW + GAP + 26, YB + BELT_H / 2, INK, 2))
    xr = X0 + 4 * SW + 3 * GAP
    out.append(arrow(xr - 26, YB + BELT_H / 2, xr + 28, YB + BELT_H / 2, INK, 2))
    out.append(text(xr + 34, YB + BELT_H / 2 + 5, "出厂", 15, INK, anchor="start"))
    out.append(text(W / 2, YB + BELT_H + 32, "每一拍传送带前进一格：四个工位同时在装四辆不同的车", 15, INK, "bold"))
    return out


if __name__ == "__main__":
    save("assembly-line", W, H, build())
