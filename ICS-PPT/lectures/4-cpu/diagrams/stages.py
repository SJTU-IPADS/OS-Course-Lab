#!/usr/bin/env python3
"""The five stages of the classic MIPS model, for any instruction.

No particular instruction: the next page (stage_units.py) shows the
hardware each stage uses. Five boxes in the stage colours the time-space figures use (fetch blue,
decode grey, execute orange, memory green, write-back solid ink) and the
hand-over between stages written above each gap. The figure sits under the
stage list on the `stages` page, so it is kept short; the single-cycle
bracket is left out, since the next page introduces that implementation.
Run it to refresh ../assets/stages.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK,
                    ORANGE, WHITE, arrow, rect, save, text)

W, H = 1120, 132
# (stage, what this step produces, fill, stroke, text colour)
STAGES = [
    ("取指", "按 PC 读出指令", FILL_BLUE, BLUE, INK),
    ("译码", "识别操作，读寄存器", FILL_GREY, INK, INK),
    ("执行", "运算或算地址", FILL_ORANGE, ORANGE, INK),
    ("访存", "读或写内存", FILL_GREEN, GREEN, INK),
    ("写回", "结果写入寄存器", INK, INK, WHITE),
]
PRODUCTS = ["指令", "源操作数", "结果或地址", "结果或数据"]   # handed over in each gap
BW, GAP, BH = 160, 60, 88
X0 = (W - 5 * BW - 4 * GAP) / 2                           # 40
Y0 = 34


def build():
    out = []
    for i, (name, sub, fill, stroke, tone) in enumerate(STAGES):
        x = X0 + i * (BW + GAP)
        out.append(rect(x, Y0, BW, BH, fill, stroke, rx=6, width=1.6))
        out.append(text(x + BW / 2, Y0 + BH / 2 - 6, name, 20, tone, "bold"))
        out.append(text(x + BW / 2, Y0 + BH / 2 + 22, sub, 15, tone))
        if i < 4:
            mid = Y0 + BH / 2
            out.append(arrow(x + BW + 4, mid, x + BW + GAP - 4, mid, INK, 2))
            out.append(text(x + BW + GAP / 2, Y0 - 12, PRODUCTS[i], 15, INK))
    return out


if __name__ == "__main__":
    save("stages", W, H, build())
