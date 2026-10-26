#!/usr/bin/env python3
"""The instruction in the execute stage in each cycle, in-order against out-of-order.

Two rows on the columns of ooo.py, cycles 3-10: the upper row reads the
execute cells of the five instructions off ooo_inorder.py, the lower one off
ooo.py. One execution unit, oldest ready instruction first:

  in-order       movl  -  -  -  imull  addl %ecx  addl $1  cmpl
  out-of-order   movl  -  -  addl $1  imull  addl %ecx  cmpl

An empty execute stage is a dashed white cell, as a bubble. addl $1,%eax is
drawn with a heavy border in both rows, and an orange elbow runs from its
in-order cycle (9) to its out-of-order cycle (6). Right of each row, the
cycle in which the last of the five (cmpl) writes back: 12 and 11.
Run it to refresh ../assets/ooo-compare.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from ooo import LEFT, W, col, numbers
from svgkit import FILL_ORANGE, INK, MONO, MUTED, ORANGE, WHITE, elbow, rect, save, text
from timegrid import CH, CW, LX, TOP, cx

H = 236
COLS = 8                            # cycles 3..10
ROW_GAP = 52                        # room between the rows for the elbow and its label
TOPS = [TOP, TOP + CH + ROW_GAP]
NEXT = "addl $1"
ROWS = [
    ("顺序流水线", ["movl", None, None, None, "imull", "addl %ecx", NEXT, "cmpl"], 12),
    ("乱序执行", ["movl", None, None, NEXT, "imull", "addl %ecx", "cmpl"], 11),
]


def cell(k, y, name):
    x = LX + k * CW
    if name is None:
        return [rect(x + 3, y + 3, CW - 6, CH - 6, WHITE, MUTED, rx=3, width=1.4, dash="5 4"),
                text(x + CW / 2, y + CH / 2 + 6, "空闲", 17, MUTED, "bold")]
    heavy = 2.8 if name == NEXT else 1.4
    return [rect(x + 3, y + 3, CW - 6, CH - 6, FILL_ORANGE, ORANGE, rx=3, width=heavy),
            text(x + CW / 2, y + CH / 2 + 6, name, 15, ORANGE, "bold", font=MONO)]


def build():
    out = [text(LX + COLS * CW / 2, 22, "每个周期在执行级中的指令", 16, INK, "bold")]
    out += numbers(COLS)
    for top, (label, names, done) in zip(TOPS, ROWS):
        out.append(text(24, top + CH / 2 + 6, label, 16, INK, "bold", anchor="start"))
        for k, name in enumerate(names):
            out += cell(k, top, name)
        out.append(text(LX + COLS * CW + 14, top + CH / 2 + 6, f"五条全部写回：周期 {done}", 15,
                        INK, "bold", anchor="start"))
    mid = TOPS[0] + CH + ROW_GAP / 2
    x9, x6 = cx(col(9)), cx(col(6))
    out.append(elbow([(x9, TOPS[0] + CH - 2), (x9, mid), (x6, mid), (x6, TOPS[1] + 2)],
                     ORANGE, 2.4))
    out.append(text((x9 + x6) / 2, mid - 7, "提前 3 个周期", 15, ORANGE, "bold"))
    return out


if __name__ == "__main__":
    save("ooo-compare", W, H, build(), left=LEFT)
