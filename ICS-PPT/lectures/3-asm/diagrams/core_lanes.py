#!/usr/bin/env python3
"""Eight cores times eight AVX2 lanes: 64 lanes working at once.

Each row is one core running one OpenMP thread over its share of the
iterations; each cell is one 32-bit lane of its %ymm registers.
Run it to refresh ../assets/core-lanes.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, GREEN, INK, LINE, MUTED, ORANGE,
                    WHITE, mono, rect, save, text, vbrace)

W, H = 1120, 430
X0, Y0, CW, CH = 150, 60, 62, 38


def build():
    out = [text(X0 + 4 * CW, 26, "每个核心内的 SIMD 通道（AVX2，8 × int32）", 16, INK, "bold")]
    for j in range(8):
        out.append(mono(X0 + j * CW + CW / 2, Y0 - 8, f"Lane {j}", 13, MUTED,
                        anchor="middle"))
    for i in range(8):
        y = Y0 + i * (CH + 6)
        out.append(mono(X0 - 14, y + 25, f"Core {i}", 15, INK, "bold", anchor="end"))
        for j in range(8):
            out.append(rect(X0 + j * CW + 2, y, CW - 4, CH, FILL_BLUE if i % 2 else FILL_GREEN,
                            BLUE if i % 2 else GREEN, rx=3, width=1.2))
    bottom = Y0 + 8 * (CH + 6) - 6
    right = X0 + 8 * CW
    out += vbrace(right + 10, Y0, bottom, INK, "", 15)
    tx = right + 40
    out.append(text(tx, 150, "OpenMP：8 个线程", 17, INK, "bold", anchor="start"))
    out.append(text(tx, 176, "迭代范围切成 8 段，每个核心一段", 15, MUTED, anchor="start"))
    out.append(text(tx, 230, "每个核心：AVX2 向量指令", 17, INK, "bold", anchor="start"))
    out.append(text(tx, 256, "一条指令处理 8 个通道", 15, MUTED, anchor="start"))
    out.append(text(tx, 320, "8 核 × 8 通道 = 64 通道", 22, ORANGE, "bold", anchor="start"))
    return out


if __name__ == "__main__":
    save("core-lanes", W, H, build())
