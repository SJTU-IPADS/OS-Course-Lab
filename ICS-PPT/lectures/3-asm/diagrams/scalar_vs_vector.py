#!/usr/bin/env python3
"""How far one loop iteration moves: 4 bytes (scalar) vs 32 bytes (AVX2).

Both loop bodies are six instructions. The scalar -O2 loop adds 4 to %rax
and covers one int; the AVX2 loop adds 32 and covers eight. Cells are the
first 16 ints of w, one colour per iteration.
Run it to refresh ../assets/scalar-vs-vector.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, GREEN, INK, LINE, MUTED, WHITE,
                    mono, path, rect, save, text)

W, H = 1120, 280
CW = 32


def strip(x0, per, color, fill, title, step, rounds):
    out = [mono(x0, 28, title, 17, INK, "bold")]
    for k in range(16):
        r = k // per
        f = fill if r % 2 == 0 else WHITE
        out.append(rect(x0 + k * CW, 110, CW, 40, f, color, rx=1, width=1.2))
        out.append(mono(x0 + k * CW + CW / 2, 136, str(k), 13, INK, anchor="middle"))
    hops = 3 if per == 1 else 1
    for h in range(hops):
        a, b = x0 + h * per * CW + 4, x0 + (h + 1) * per * CW + 4
        top = 100 - (34 if per == 1 else 44)
        out.append(path(f"M {a} 106 C {a} {top} {b} {top} {b} 104", color, 2))
        out.append(mono((a + b) / 2, top + 4, f"+{step}", 14, color, "bold",
                        anchor="middle"))
    out.append(text(x0, 182, f"每轮 {per} 个元素（{step} 字节）", 17, color, "bold",
                    anchor="start"))
    out.append(text(x0, 210, f"n = 4096：{rounds} 轮", 15, MUTED, anchor="start"))
    return out


def build():
    out = strip(40, 1, BLUE, FILL_BLUE, "标量：addq $4, %rax", 4, 4096)
    out += strip(600, 8, GREEN, FILL_GREEN, "AVX2：addq $32, %rax", 32, 512)
    out.append(text(560, 262, "两种循环体都是 6 条指令；每轮处理的元素数是 8 倍", 17, INK,
                    "bold"))
    return out


if __name__ == "__main__":
    save("scalar-vs-vector", W, H, build())
