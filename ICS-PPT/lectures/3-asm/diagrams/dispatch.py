#!/usr/bin/env python3
"""How ollama picks a CPU backend: cpuid feature bits, then one library.

The library list is /usr/local/lib/ollama/ from the page; the highlighted
row is the one named in the load_backend log line on page 73.
Run it to refresh ../assets/dispatch.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MONO, MUTED,
                    ORANGE, WHITE, arrow, box, line, mono, rect, save, text)

W, H = 1120, 200
LIBS = [("libggml-cpu-x64.so", "x86-64 基线"), ("libggml-cpu-sse42.so", "SSE4.2"),
        ("libggml-cpu-haswell.so", "AVX2 + FMA"),
        ("libggml-cpu-alderlake.so", "AVX2 + AVX_VNNI"),
        ("libggml-cpu-icelake.so", "AVX-512 + VBMI + VNNI"),
        ("libggml-cpu-zen4.so", "AVX-512")]
HIT = 4


def build():
    out = box(10, 72, 120, 56, "程序启动", FILL_GREY, MUTED, 16)
    out.append(arrow(130, 100, 160, 100, INK, 2))
    out += box(164, 66, 170, 68, "cpuid", FILL_BLUE, BLUE, 18, font=MONO,
               sub="读取特性位")
    out.append(arrow(334, 100, 364, 100, INK, 2))
    out += box(368, 66, 170, 68, "判定特性", FILL_ORANGE, ORANGE, 17,
               sub="AVX2 / AVX-512 …")
    lx = 640
    for k, (name, feats) in enumerate(LIBS):
        y = 4 + k * 32
        hit = k == HIT
        out.append(line(538, 100, lx - 2, y + 14, ORANGE if hit else LINE,
                        2.4 if hit else 1.2))
        out.append(rect(lx, y, 470, 28, FILL_ORANGE if hit else WHITE,
                        ORANGE if hit else LINE, rx=3, width=1.6 if hit else 1.2))
        out.append(mono(lx + 10, y + 19, name, 14, INK, "bold" if hit else "normal"))
        out.append(text(lx + 460, y + 19, feats, 13, INK if hit else MUTED,
                        "bold" if hit else "normal", anchor="end"))
    out.append(text(453, 160, "加载对应的动态库", 14, MUTED))
    return out


if __name__ == "__main__":
    save("dispatch", W, H, build())
