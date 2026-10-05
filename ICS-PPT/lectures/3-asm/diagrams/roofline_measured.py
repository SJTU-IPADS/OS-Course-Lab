#!/usr/bin/env python3
"""The roofline of roofline.py with two measured programs on it.

Both ran the same Q4_0 file on one thread of the i9-11900H (2026-10-05, on
mains power): ollama 0.33.2 with num_gpu 0 and num_thread 1, and mini-ollama
from ../examples/mini-ollama. A rate in tokens a second times 3.44e9 FLOP a
token is the rate in GFLOPS.

One thread works on one of the 8 cores, so the dashed line is the compute
roof of one core, 320 / 8 = 40 GFLOPS. The memory roof of roofline.py lies
above it over the whole axis: a run on one thread is bound by compute.
Run it to refresh ../assets/roofline-measured.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from roofline import AI, H, PEAK, PX0, PX1, W, frame, gflops, gx, gy
from svgkit import BLUE, GREEN, INK, ORANGE, arrow, circle, line, save, text

CORES = 8
# program, colour, eval rate in tokens a second
MEASURED = [("ollama", GREEN, 10.7), ("mini-ollama", ORANGE, 0.77)]


def build():
    px, core = gx(AI), gy(PEAK / CORES)
    ys = [gy(gflops(rate)) for _, _, rate in MEASURED]
    # ollama sits just under the line of one core: its label goes below the line
    label_y = [core + 22, ys[1] + 5]
    out = frame(ridge_gaps=[(y - 16, y + 6) for y in label_y])
    out.append(line(PX0, core, PX1, core, BLUE, 2.4, "8 5"))
    out.append(text(PX1 - 8, core - 8, f"单核算力上限 P ÷ {CORES} = {PEAK / CORES:g} GFLOPS", 15,
                    BLUE, "bold", anchor="end"))
    for (name, color, rate), y, ly in zip(MEASURED, ys, label_y):
        out.append(circle(px, y, 8, color, color))
        out.append(text(px + 16, ly,
                        f"{name} 单线程实测 {rate:g} Token/s（约 {gflops(rate):.2g} GFLOPS）",
                        15, color, "bold", anchor="start"))
    fast, slow = MEASURED[0][2], MEASURED[1][2]
    out.append(arrow(px - 24, ys[0] + 4, px - 24, ys[1] - 4, INK, 1.8, both=True))
    out.append(text(px - 36, (ys[0] + ys[1]) / 2 + 5, f"相差 {fast / slow:.0f} 倍", 16, INK,
                    "bold", anchor="end"))
    return out


if __name__ == "__main__":
    save("roofline-measured", W, H, build())
