#!/usr/bin/env python3
"""Without forwarding: the next instruction decodes only after the write-back.

Two rows of the shared time-space grid (timegrid.py). The first instruction
runs its five stages in cycles 1-5 and writes its register at the clock edge
that ends cycle 5. The second one reads that register in decode, so it waits
in cycles 3-5 (red "停" cells) and decodes in cycle 6; execute, memory and
write-back follow in cycles 7-9. A dark elbow runs from the write-back cell
to the decode cell, coloured like the stage it leaves.

Two series with the same timing, one per page:
no-forward-k       imull -> addl through %ecx (the forward page);
no-forward-load-k  movl reads w[i] into %r10d -> imull (the load-use page).
An animation: frames 1-5 show cycles 1..2 up to 1..6, one cycle per frame;
the last frame adds cycles 7-9 and the bracket under the three stall cells.
Run it to refresh ../assets/no-forward-1.svg .. no-forward-load-6.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, RED, brace, elbow, save, text
from timegrid import CH, CW, LX, TOP, VIEW_LEFT, cx, cycle_numbers, row

W, H = 1120, 266
GAP = 56                            # room between the rows for the elbow and its label
R2 = TOP + CH + GAP                 # top of the second row (172)
MID = TOP + CH + GAP / 2            # the elbow's horizontal run (144)
FIRST = ["取指", "译码", "执行", "访存", "写回"]
SECOND = ["取指", "停", "停", "停", "译码", "执行", "访存", "写回"]
UPTO = [2, 3, 4, 5, 6, 9]           # cycles shown in each frame
CASES = {                           # name -> (first, second, register, value)
    "no-forward": ("imull", "addl", "%ecx", "乘积"),
    "no-forward-load": ("movl", "imull", "%r10d", " w[i] "),
}


def build(upto, first, second, reg, value):
    out = []
    if upto >= 3:
        out.append(text(cx(2), 22, f"周期 3：{second} 要读 {reg}，{first} 要到周期 5 才写回", 16,
                        RED, "bold"))
    out += cycle_numbers(9, emph=3 if upto >= 3 else None, color=RED)
    out += row(first, TOP, 0, FIRST, upto)
    out += row(second, R2, 1, SECOND, upto)
    if upto >= 5:
        out.append(text(LX + 5 * CW + 12, TOP + CH / 2 + 6, f"周期 5 结束时{value}写入 {reg}", 15,
                        INK, anchor="start"))
    if upto >= 6:
        out.append(elbow([(cx(4), TOP + CH - 1), (cx(4), MID), (cx(5), MID), (cx(5), R2 + 1)],
                         INK, 2.4))
        out.append(text(LX + 6 * CW + 8, MID + 5, f"周期 6 译码，读到写回后的 {reg}", 16, INK,
                        "bold", anchor="start"))
    if upto >= 9:
        out += brace(LX + 2 * CW + 4, LX + 5 * CW - 4, R2 + CH + 6, RED, "等待写回的 3 个周期",
                     16)
    return out


if __name__ == "__main__":
    for name, case in CASES.items():
        for k, upto in enumerate(UPTO, 1):
            save(f"{name}-{k}", W, H, build(upto, *case), left=VIEW_LEFT)
