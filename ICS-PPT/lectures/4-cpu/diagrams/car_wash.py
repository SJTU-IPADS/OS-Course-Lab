#!/usr/bin/env python3
"""A pipelined car wash: three stations, one step each.

Drawn for the page "现实中的流水线：洗车" (ICS1 lecture 17, page 3, "Real-World
Pipelines: Car Washes"), where it sits right of the three-line idea, under the
photographs of that page. The wash is three steps (soap, rinse, dry); each
station does one step, and the cars pass through them in order, so every
station holds a car: car 1 is drying while car 3 is being soaped. The step
colours come from timegrid.TONE, as the stage chips of insn_flow.py do.
Run it to refresh ../assets/car-wash.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, WHITE, arrow, rect, save, text
from timegrid import TONE

STEPS = [("打泡沫", "取指"), ("冲洗", "执行"), ("擦干", "访存")]   # step -> tone
CAR_W, CAR_H = 72, 42
STAGE_W, STAGE_H, STAGE_GAP = 124, 112, 34   # a one-step station
PAD = 8
W = 2 * PAD + 3 * STAGE_W + 2 * STAGE_GAP
H = 2 * PAD + STAGE_H


def car(x, y, n):
    return [rect(x, y, CAR_W, CAR_H, WHITE, INK, rx=5, width=1.8),
            text(x + CAR_W / 2, y + CAR_H / 2 + 6, f"车 {n}", 18, INK, "bold")]


def build():
    out = []
    y = PAD
    for k, (name, tone) in enumerate(STEPS):
        x = PAD + k * (STAGE_W + STAGE_GAP)
        fill, stroke, ink = TONE[tone]
        out.append(rect(x, y, STAGE_W, STAGE_H, fill, stroke, rx=8, width=1.8))
        out.append(text(x + STAGE_W / 2, y + 32, name, 20, ink, "bold"))
        out += car(x + (STAGE_W - CAR_W) / 2, y + STAGE_H - CAR_H - 12, 3 - k)
        if k:
            out.append(arrow(x - STAGE_GAP + 4, y + STAGE_H / 2, x - 4, y + STAGE_H / 2,
                             INK, 2))
    return out


if __name__ == "__main__":
    save("car-wash", W, H, build())
