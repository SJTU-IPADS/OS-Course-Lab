#!/usr/bin/env python3
"""Peak single-precision throughput of consumer GPUs and CPUs, 2006 to 2025.

Orange: the GeForce flagship of each generation, CUDA cores x 2 operations per
FMA x boost clock. Blue: the Intel Core flagship of the mainstream desktop
platform, cores x base clock x FP32 operations per cycle per core (8 with SSE,
16 with AVX, 32 with AVX2 and two FMA units; the E-cores of Alder Lake to
Raptor Lake do 16, those of Arrow Lake 32). The vertical axis is logarithmic.
Run it to refresh ../assets/peak-trend.svg.
"""

import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE,
                    circle, line, save, text)

# (year, model, GFLOP/s)
GPU = [(2006, "8800 GTX", 345.6), (2008, "GTX 280", 622), (2010, "GTX 480", 1345),
       (2012, "GTX 680", 3250), (2013, "GTX 780 Ti", 5345), (2015, "GTX 980 Ti", 6060),
       (2017, "GTX 1080 Ti", 11340), (2018, "RTX 2080 Ti", 13450),
       (2020, "RTX 3090", 35580), (2022, "RTX 4090", 82580), (2025, "RTX 5090", 104760)]
CPU = [(2006, "Core 2 Extreme QX6700", 4 * 2.66 * 8), (2008, "Core i7-965", 4 * 3.2 * 8),
       (2010, "Core i7-980X", 6 * 3.33 * 8), (2011, "Core i7-2600K", 4 * 3.4 * 16),
       (2013, "Core i7-4770K", 4 * 3.5 * 32), (2015, "Core i7-6700K", 4 * 4.0 * 32),
       (2017, "Core i7-7700K", 4 * 4.2 * 32), (2018, "Core i9-9900K", 8 * 3.6 * 32),
       (2020, "Core i9-10900K", 10 * 3.7 * 32),
       (2021, "Core i9-12900K", 8 * 3.2 * 32 + 8 * 2.4 * 16),
       (2022, "Core i9-13900K", 8 * 3.0 * 32 + 16 * 2.2 * 16),
       (2023, "Core i9-14900K", 8 * 3.2 * 32 + 16 * 2.4 * 16),
       (2024, "Core Ultra 9 285K", 8 * 3.7 * 32 + 16 * 3.2 * 32)]

W, H = 600, 470
LEFT, RIGHT, TOP, BOTTOM = 70, 500, 40, 420
YEAR0, YEAR1 = 2005, 2026
LOW, HIGH = 1.5, 5.3          # log10 of the axis ends


def px(year):
    return LEFT + (year - YEAR0) / (YEAR1 - YEAR0) * (RIGHT - LEFT)


def py(value):
    return BOTTOM - (math.log10(value) - LOW) / (HIGH - LOW) * (BOTTOM - TOP)


def series(points, color, fill):
    out = []
    coords = [(px(year), py(value)) for year, _, value in points]
    path = " ".join(f"{x:.1f},{y:.1f}" for x, y in coords)
    out.append(f'<polyline points="{path}" fill="none" stroke="{color}" stroke-width="3" '
               f'stroke-linejoin="round"/>')
    out += [circle(x, y, 4.5, fill, color, 2) for x, y in coords]
    return out


def build():
    out = [text(LEFT, 22, "单精度峰值算力（GFLOP/s，对数坐标）", 16, INK, "bold", "start")]
    for exponent in (2, 3, 4, 5):
        y = py(10 ** exponent)
        out.append(line(LEFT, y, RIGHT, y, LINE, 1, "4 4"))
        out.append(text(LEFT - 8, y + 5, f"{10 ** exponent:,}".replace(",", " "), 14, MUTED,
                        "normal", "end"))
    out.append(line(LEFT, BOTTOM, RIGHT, BOTTOM, INK, 1.6))
    for year in range(2006, 2026, 3):
        out.append(line(px(year), BOTTOM, px(year), BOTTOM + 5, INK, 1.4))
        out.append(text(px(year), BOTTOM + 24, str(year), 14, MUTED))
    out += series(CPU, BLUE, FILL_BLUE)
    out += series(GPU, ORANGE, FILL_ORANGE)

    first_gpu, last_gpu = GPU[0], GPU[-1]
    first_cpu, last_cpu = CPU[0], CPU[-1]
    out.append(text(px(last_gpu[0]), py(last_gpu[2]) - 14,
                    f"{last_gpu[1]}　{last_gpu[2] / 1000:.1f} TFLOP/s", 15, ORANGE, "bold", "end"))
    out.append(text(px(last_cpu[0]) - 14, py(last_cpu[2]) + 5,
                    f"{last_cpu[1]}　{last_cpu[2] / 1000:.1f} TFLOP/s", 15, BLUE, "bold", "end"))
    out.append(text(px(first_gpu[0]) - 14, py(first_gpu[2]) + 28, first_gpu[1], 14, ORANGE,
                    "normal", "start"))
    out.append(text(px(first_cpu[0]) - 14, py(first_cpu[2]) + 26, "Core 2 Extreme", 14, BLUE,
                    "normal", "start"))

    # the gap of the last generation, drawn as a bracket on its right
    x = px(2025) + 14
    y_top, y_bot = py(last_gpu[2]), py(last_cpu[2])
    out.append(line(x, y_top, x, y_bot, INK, 1.8))
    out.append(line(x - 5, y_top, x, y_top, INK, 1.8))
    out.append(line(x - 5, y_bot, x, y_bot, INK, 1.8))
    ratio = last_gpu[2] / last_cpu[2]
    out.append(text(x + 8, (y_top + y_bot) / 2 + 5, f"约 {ratio:.0f} 倍", 15, INK, "bold", "start"))

    # legend
    lx, ly = LEFT + 190, BOTTOM - 62
    out.append(line(lx, ly, lx + 30, ly, ORANGE, 3))
    out.append(text(lx + 38, ly + 5, "GeForce 旗舰 GPU", 15, INK, "normal", "start"))
    out.append(line(lx, ly + 26, lx + 30, ly + 26, BLUE, 3))
    out.append(text(lx + 38, ly + 31, "Intel Core 旗舰 CPU", 15, INK, "normal",
                    "start"))
    return out


if __name__ == "__main__":
    save("peak-trend", W, H, build())
