#!/usr/bin/env python3
"""A moving assembly line: three stations along one conveyor.

Drawn for the page "流水线这个词从哪来：装配线" in the manner of car_wash.py
(the same cars, the same station colours from timegrid.TONE): station 1
fits the engine, station 2 the wheels, station 3 the body; car 3 is at
station 1 while car 2 is at station 2 and car 1 at station 3. The conveyor
under the stations carries each car to the next one.
Run it to refresh ../assets/assembly-line.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from car_wash import CAR_H, CAR_W, car
from svgkit import INK, MUTED, WHITE, arrow, circle, line, rect, save, text
from timegrid import TONE

STEPS = [("装发动机", "取指"), ("装车轮", "执行"), ("装车身", "访存")]   # step -> tone
SW, SH, SGAP = 176, 112, 56          # a station and the gap between two
END = 84                             # room for 上线 / 下线 at either end
W = 1120
X0 = (W - 3 * SW - 2 * SGAP) / 2
SY = 10
BELT = SY + SH + 22                  # the conveyor line
H = BELT + 46


def build():
    out = []
    for k, (name, tone) in enumerate(STEPS):
        x = X0 + k * (SW + SGAP)
        fill, stroke, ink = TONE[tone]
        out.append(rect(x, SY, SW, SH, fill, stroke, rx=8, width=1.8))
        out.append(text(x + SW / 2, SY + 28, f"工位 {k + 1}：{name}", 18, ink, "bold"))
        out += car(x + (SW - CAR_W) / 2, SY + SH - CAR_H - 14, 3 - k)
        out.append(line(x + SW / 2, SY + SH, x + SW / 2, BELT - 6, MUTED, 1.4))
    left, right = X0 - END, X0 + 3 * SW + 2 * SGAP + END
    out.append(arrow(left, BELT, right, BELT, INK, 4))
    for k in range(int((right - left - 20) // 40) + 1):
        out.append(circle(left + 10 + k * 40, BELT + 9, 4, WHITE, MUTED, 1.2))
    out.append(text(left, BELT - 10, "上线", 16, INK, "bold", anchor="start"))
    out.append(text(right - 30, BELT - 10, "下线", 16, INK, "bold", anchor="end"))
    out.append(text(W / 2, BELT + 38, "传送带：每过一个节拍，每辆车前进一个工位", 15, MUTED))
    return out


if __name__ == "__main__":
    save("assembly-line", W, H, build())
