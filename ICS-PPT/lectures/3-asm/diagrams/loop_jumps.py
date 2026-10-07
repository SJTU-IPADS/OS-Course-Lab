#!/usr/bin/env python3
"""dot_product at gcc -Og, with its two jumps drawn beside the listing.

The listing is what `gcc -Og -fcf-protection=none -S dot.c` prints. The loop
body (.L3 up to the increment of i) is washed blue and bracketed on the right.
The forward jump `jmp .L2` runs down the right margin into the loop test; the
backward jump `jl .L3` runs up the left margin into the loop body; the dashed
hook is the fall-through when the test fails. The two jumps are on opposite
sides so that neither crosses the other. For the side column.
Run it to refresh ../assets/loop-jumps.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, INK, MUTED, ORANGE, elbow, listing, save,
                    text, vbrace)

W, H = 500, 390
LX, LY, SIZE, LH = 120, 14, 13, 24          # listing: left edge, top, font, pitch
RIGHT = LX + 34 * SIZE * 0.6 + 10           # right edge of the washed rows

ASM = ["dot_product:",
       "\tmovl\t$0, %eax",
       "\tmovl\t$0, %r9d",
       "\tjmp\t.L2",
       ".L3:",
       "\tmovslq\t%eax, %r8",
       "\tmovl\t(%rsi,%r8,4), %ecx",
       "\timull\t(%rdi,%r8,4), %ecx",
       "\taddl\t%ecx, %r9d",
       "\taddl\t$1, %eax",
       ".L2:",
       "\tcmpl\t%edx, %eax",
       "\tjl\t.L3",
       "\tmovl\t%r9d, %eax",
       "\tret"]
BODY = range(4, 10)                         # .L3: through addl $1, %eax
JMP, L2, JL, L3, NEXT = 3, 10, 12, 4, 13    # rows of the jumps and their targets


def mid(row):
    return LY + row * LH + LH / 2


def build():
    out = listing(LX, LY, ASM, SIZE, LH, marks={r: FILL_BLUE for r in BODY})
    out += vbrace(RIGHT + 6, LY + BODY.start * LH + 2, LY + BODY.stop * LH - 2, BLUE,
                  "循环体", 14)
    # jmp .L2: forward, down the right margin into the loop test
    x = RIGHT + 74
    out.append(elbow([(RIGHT + 4, mid(JMP)), (x, mid(JMP)), (x, mid(L2)),
                      (RIGHT + 8, mid(L2))], BLUE, 2.2))
    out.append(text((RIGHT + 4 + x) / 2, mid(JMP) - 7, "无条件跳转", 14, BLUE, "bold"))
    # jl .L3: backward, up the left margin into the loop body
    x = LX - 56
    out.append(elbow([(LX - 10, mid(JL)), (x, mid(JL)), (x, mid(L3)),
                      (LX - 14, mid(L3))], ORANGE, 2.2))
    out.append(text(LX - 14, mid(JL) + 19, "i < n 时跳回", 13, ORANGE, "bold", anchor="end"))
    # the fall-through when the test fails
    x = RIGHT + 16
    out.append(elbow([(RIGHT + 4, mid(JL) + 3), (x, mid(JL) + 3), (x, mid(NEXT)),
                      (RIGHT + 8, mid(NEXT))], MUTED, 1.8, dash="4 3"))
    out.append(text(x + 8, mid(NEXT) + 5, "否则顺序执行", 13, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("loop-jumps", W, H, build())
