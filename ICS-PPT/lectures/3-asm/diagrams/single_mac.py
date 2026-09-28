#!/usr/bin/env python3
"""dot_product greyed out except the one statement, beside DRAM and the CPU.

Only i = 0 is followed: w[0] and x[0] sit in DRAM, the ALU sits in the CPU,
and the link between them is the open question of the page.
Run it to refresh ../assets/single-mac.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED,
                    ORANGE, RED, WHITE, arrow, box, cells, mono, rect, save, text)

W, H = 1120, 210

DOT_C = ["int dot_product(const int *w, const int *x, int n) {",
         "    int sum = 0;",
         "    for (int i = 0; i < n; i++) {",
         "        sum += w[i] * x[i];",
         "    }",
         "    return sum;",
         "}"]


def code():
    out = [rect(20, 10, 540, 190, WHITE, LINE, rx=6)]
    for i, s in enumerate(DOT_C):
        y = 36 + i * 24
        if i == 3:
            out.append(rect(28, y - 17, 524, 24, FILL_ORANGE, "none", rx=3, width=0))
        out.append(mono(34, y, s, 15, INK if i == 3 else "#a9b6bf",
                        "bold" if i == 3 else "normal"))
    out.append(text(380, 188, "i = 0：w[0] * x[0]", 16, ORANGE, "bold", anchor="start"))
    return out


def memory():
    out = [rect(610, 20, 220, 170, FILL_GREY, MUTED, rx=8, width=1.6),
           text(720, 48, "DRAM", 18, INK, "bold")]
    for k, name in enumerate("wx"):
        y = 70 + k * 56
        out.append(mono(626, y + 25, name, 16, MUTED))
        shapes, _ = cells(646, y, [f"{name}[0]", f"{name}[1]", "…"], 58, 38,
                          [FILL_ORANGE, WHITE, WHITE], MUTED, 14)
        out += shapes
    return out


def cpu():
    out = [rect(900, 20, 200, 170, FILL_BLUE, BLUE, rx=8, width=1.6),
           text(1000, 48, "CPU", 18, INK, "bold")]
    out += box(930, 70, 140, 40, "寄存器", WHITE, BLUE, 16)
    out += box(930, 126, 140, 44, "ALU：× +", WHITE, ORANGE, 16)
    return out


def build():
    out = code() + memory() + cpu()
    out.append(arrow(834, 105, 896, 105, RED, 2.4, dash="6 4"))
    out.append(text(865, 92, "?", 22, RED, "bold"))
    return out


if __name__ == "__main__":
    save("single-mac", W, H, build())
