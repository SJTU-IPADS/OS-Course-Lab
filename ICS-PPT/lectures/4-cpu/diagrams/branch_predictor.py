#!/usr/bin/env python3
"""The hardware branch predictor: a table of 2-bit counters, one per jump.

Left: the predictor table. A row belongs to one jump instruction and is
found by that instruction's address; its cell holds the jump's counter. The
row shown is jl .L3 at 0x34 (objdump -d dot.o, the output that puts movslq
at 0x20 on stage-units), holding 3 while the loop runs. The fetch stage looks
the row up with the PC (blue box, the fetch colour of timegrid.py); after jl
has executed, its outcome updates the counter (orange box, the execute
colour, also the colour of the decision line in branch.py).
Right: the four values of a 2-bit counter. A taken jump adds 1 (arrows on
top), a not-taken jump subtracts 1 (arrows below), and the counter stays at
3 or 0 at the ends; the brackets group the values that predict not taken
(0, 1) and taken (2, 3). The cell of jl .L3 and the box of its value 3 carry
the same thick frame. Blue and orange mean the two stages only, so the
counter and the brackets are ink.
Run it to refresh ../assets/branch-predictor.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, FONT, INK, MONO, MUTED,
                    ORANGE, WHITE, arrow, brace, label_line, mono, path, rect, save, text)

W, H = 1120, 176

# the table: a header, one row standing for the other jumps, then jl .L3
TX, TY = 200, 4                     # top-left corner
AW, CW = 170, 110                   # address column, counter column
RH = 36                             # row height
JL_Y = TY + 2 * RH                  # top of the jl .L3 row
CX = TX + AW                        # left edge of the counter column
FRAME = 2.6                         # the thick frame: jl's cell, the value 3

# the counter: four values in a row
SX, SY = 604, 30                    # left edge of value 0, top of the boxes
BW, BH, GAP = 68, 56, 56
CY = SY + BH / 2


def cell(x, y, w, s, fill=WHITE, tone=INK, weight="normal", font=FONT):
    return [rect(x, y, w, RH, fill, INK, rx=0, width=1.4),
            text(x + w / 2, y + RH / 2 + 6, s, 16, tone, weight, font=font)]


def table():
    out = cell(TX, TY, AW, "跳转指令的地址", FILL_GREY, weight="bold")
    out += cell(CX, TY, CW, "2 位计数器", FILL_GREY, weight="bold")
    out += cell(TX, TY + RH, AW, "…", tone=MUTED)
    out += cell(CX, TY + RH, CW, "…", tone=MUTED)
    out += cell(TX, JL_Y, AW, "0x34  jl .L3", font=MONO)
    out += cell(CX, JL_Y, CW, "")
    out.append(rect(CX + 4, JL_Y + 4, CW - 8, RH - 8, WHITE, INK, rx=3, width=FRAME))
    out.append(mono(CX + CW / 2, JL_Y + RH / 2 + 7, "3", 20, INK, "bold", anchor="middle"))
    return out


def lookup():
    """Fetch looks the row up with the PC; execute updates the counter."""
    out = []
    bx, bw, bh = 20, 132, 52
    by = JL_Y + RH / 2 - bh / 2
    out.append(rect(bx, by, bw, bh, FILL_BLUE, BLUE, rx=6, width=1.6))
    out.append(text(bx + bw / 2, by + 21, "取指", 15, BLUE, "bold"))
    out.append(mono(bx + bw / 2, by + 42, "PC = 0x34", 16, INK, anchor="middle"))
    out.append(arrow(bx + bw + 2, JL_Y + RH / 2, TX - 4, JL_Y + RH / 2, BLUE))
    out.append(text((bx + bw + TX) / 2, JL_Y + RH / 2 - 10, "查表", 15, BLUE, "bold"))

    uw, uh = 200, 34
    ux, uy = CX + CW / 2 - uw / 2, H - uh - 4
    out.append(rect(ux, uy, uw, uh, FILL_ORANGE, ORANGE, rx=6, width=1.6))
    out += label_line(ux + 22, uy + 23, [("jl", MONO, INK, "normal"),
                                         (" 执行完：跳或不跳", FONT, ORANGE, "bold")],
                      size=16)
    out.append(arrow(CX + CW / 2, uy - 2, CX + CW / 2, JL_Y + RH + 4, ORANGE))
    out.append(text(CX + CW / 2 + 10, (uy + JL_Y + RH) / 2 + 6, "更新", 15, ORANGE,
                    "bold", anchor="start"))
    return out


def counter():
    out = []
    xs = [SX + k * (BW + GAP) for k in range(4)]
    for k, x in enumerate(xs):
        out.append(rect(x, SY, BW, BH, WHITE, INK, rx=6, width=FRAME if k == 3 else 1.6))
        out.append(mono(x + BW / 2, CY + 8, str(k), 22, INK, "bold", anchor="middle"))
    # between neighbours: taken goes right (top), not taken goes left (bottom)
    for k in range(3):
        a, b = xs[k] + BW, xs[k + 1]
        mid = (a + b) / 2
        out.append(arrow(a + 4, CY - 12, b - 4, CY - 12, INK))
        out.append(text(mid, CY - 20, "跳", 15, INK, "bold"))
        out.append(arrow(b - 4, CY + 12, a + 4, CY + 12, INK))
        out.append(text(mid, CY + 34, "不跳", 15, INK, "bold"))
    # at the ends the counter stays: 3 on taken, 0 on not taken
    r = xs[3] + BW
    out.append(path(f"M {r + 2} {CY - 12} C {r + 42} {CY - 36}, {r + 42} {CY + 36}, "
                    f"{r + 4} {CY + 12}", INK, 2))
    out.append(text(r + 38, CY + 6, "跳", 15, INK, "bold", anchor="start"))
    l0 = xs[0]
    out.append(path(f"M {l0 - 2} {CY + 12} C {l0 - 42} {CY + 36}, {l0 - 42} {CY - 36}, "
                    f"{l0 - 4} {CY - 12}", INK, 2))
    out.append(text(l0 - 38, CY + 6, "不跳", 15, INK, "bold", anchor="end"))
    # what each value predicts
    yb = SY + BH + 34
    out += brace(xs[0], xs[1] + BW, yb, INK, "预测不跳转", size=16)
    out += brace(xs[2], xs[3] + BW, yb, INK, "预测跳转", size=16)
    return out


def build():
    return table() + lookup() + counter()


if __name__ == "__main__":
    save("branch-predictor", W, H, build())
