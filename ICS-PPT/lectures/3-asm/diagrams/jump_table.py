#!/usr/bin/env python3
"""A six-case switch compiled to a jump table, with the path for op = 2.

The listing is what `gcc -O1 -fno-pie -fcf-protection=none -S calc.c` prints
for `int calc(int op, int a, int b)` whose cases 0..5 return a+b, a-b, a*b,
a&b, a|b, a^b and which returns 0 after the switch. Left to right: the entry
(the indirect jump washed orange), the table .L4 of six 8-byte slots, and the
six jump targets. A target is shown as its line of calc.c: the code at a
label is several instructions, and the C line says what they compute. The
orange path is the worked case op = 2: the entry computes .L4 + 8 x 2, reads
.L7 from slot 2 and jumps there. The dashed path is `ja .L10`: op above 5 as
an unsigned number goes to the `return 0` after the switch.
Run it to refresh ../assets/jump-table.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE,
                    arrow, elbow, listing, mono, rect, save, text)

W, H = 1120, 262
LX, LY, LH = 24, 10, 24             # entry listing: left edge, top, pitch
ENTRY_RIGHT = LX - 6 + 30 * 15 * 0.6 + 16   # right edge of the washed rows
TX, TW = 400, 140                   # jump table: left edge, slot width
CX, CW = 740, 310                   # case boxes: left edge, width
Y0, PITCH, RH = 44, 30, 27          # first slot top, row pitch, slot height
CASE = 2                            # the worked case

ENTRY = ["calc:",
         "\tcmpl\t$5, %edi",
         "\tja\t.L10",
         "\tmovl\t%edi, %edi",
         "\tjmp\t*.L4(,%rdi,8)"]
JA, JMP = 2, 4                      # rows of the two jumps

# slot content, the C statement of the case, op
CASES = [(".L9", "return a + b;", 0), (".L8", "return a - b;", 1),
         (".L7", "return a * b;", 2), (".L6", "return a & b;", 3),
         (".L5", "return a | b;", 4), (".L3", "return a ^ b;", 5)]


def row_mid(i):
    return Y0 + i * PITCH + RH / 2


def entry():
    return listing(LX, LY, ENTRY, 15, LH, marks={JMP: FILL_ORANGE})


def table():
    out = []
    for i, (label, _, _) in enumerate(CASES):
        y = Y0 + i * PITCH
        picked = i == CASE
        out.append(rect(TX, y, TW, RH, FILL_BLUE, ORANGE if picked else BLUE, rx=2,
                        width=2.4 if picked else 1.4))
        out.append(mono(TX + 10, y + RH / 2 + 5, str(i), 13, MUTED))
        out.append(mono(TX + TW / 2 + 8, y + RH / 2 + 5, label, 15, INK, "bold",
                        anchor="middle"))
    out.append(text(TX + TW / 2, Y0 + 6 * PITCH + 18, "跳转表 .L4：每项 8 字节，一个目标地址",
                    14, INK))
    return out


def cases():
    out = []
    for i, (label, stmt, op) in enumerate(CASES):
        y = Y0 + i * PITCH
        picked = i == CASE
        out.append(rect(CX, y, CW, RH, WHITE, ORANGE if picked else BLUE, rx=2,
                        width=2.4 if picked else 1.4))
        out.append(mono(CX + 10, y + RH / 2 + 5, label, 13, MUTED))
        out.append(mono(CX + 48, y + RH / 2 + 5, f"case {op}: {stmt}", 14, INK, "bold"))
    # the return after the switch, reached by ja
    out.append(rect(CX, 8, CW, RH, WHITE, MUTED, rx=2, width=1.4, dash="5 4"))
    out.append(mono(CX + 10, 8 + RH / 2 + 5, ".L10", 13, MUTED))
    out.append(mono(CX + 48, 8 + RH / 2 + 5, "return 0;", 14, INK, "bold"))
    return out


def paths():
    out = []
    # every slot holds the address of its case body
    for i in range(len(CASES)):
        if i != CASE:
            out.append(arrow(TX + TW + 4, row_mid(i), CX - 6, row_mid(i), LINE, 1.2))
    # the worked case: entry -> slot 2 -> .L7
    jy = LY + JMP * LH + LH / 2
    out.append(arrow(ENTRY_RIGHT + 4, jy, TX - 6, jy, ORANGE, 2.4))
    out.append(text((ENTRY_RIGHT + TX) / 2, jy - 10, ".L4 + 8 × op", 14, ORANGE, "bold"))
    out.append(arrow(TX + TW + 4, row_mid(CASE), CX - 6, row_mid(CASE), ORANGE, 2.4))
    out.append(text((TX + TW + CX) / 2, row_mid(CASE) - 9, "读出 .L7 的地址，写入 %rip", 14,
                    ORANGE, "bold"))
    # ja .L10: the return after the switch, over the top of the table
    ay = LY + JA * LH + LH / 2
    out.append(elbow([(ENTRY_RIGHT + 4, ay), (332, ay), (332, 22), (CX - 6, 22)], MUTED,
                     1.8, dash="6 4"))
    out.append(text((332 + CX) / 2, 16, "op 作为无符号数大于 5：跳到 .L10", 13, MUTED))
    return out


def build():
    return entry() + table() + cases() + paths()


if __name__ == "__main__":
    save("jump-table", W, H, build(), left=-25)    # centres the ink on the canvas
