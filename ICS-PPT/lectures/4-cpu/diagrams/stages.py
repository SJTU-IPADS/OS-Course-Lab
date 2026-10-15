#!/usr/bin/env python3
"""One movl from the dot_product loop, split into the five stages.

The instruction is `movl (%rsi,%r8,4), %ecx` from gcc -Og: read x[i].
Run it to refresh ../assets/stages.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, FILL_BLUE, INK, box, save, text

W, H = 1120, 220
STAGES = [
    ("取指", "读 %rip 处的字节"),
    ("译码", "认出 movl 与寄存器"),
    ("执行", "算出 x[i] 的地址"),
    ("访存", "读出这个 int"),
    ("写回", "写入 %ecx"),
]


def build():
    bw, gap, bh = 196, 16, 120
    total = len(STAGES) * bw + (len(STAGES) - 1) * gap
    x0, y = (W - total) / 2, 58
    out = [text(W / 2, 32, "movl (%rsi,%r8,4), %ecx", 18, INK, "bold", font="monospace")]
    for i, (name, sub) in enumerate(STAGES):
        out += box(x0 + i * (bw + gap), y, bw, bh, name, FILL_BLUE, BLUE, 20, "bold",
                   INK, sub=sub, sub_size=15)
    return out


if __name__ == "__main__":
    save("stages", W, H, build())
