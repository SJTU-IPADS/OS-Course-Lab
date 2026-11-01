#!/usr/bin/env python3
"""The lea of phase_1 in .text and the string it points to in .rodata: the
address is the address of the next instruction plus the displacement, and
objdump writes the sum after #. The bytes are those of examples/bomb. Run it
to refresh ../assets/bomb_lea.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, MONO,
                    MUTED, ORANGE, WHITE, arrow, cells, elbow, label_line, mono,
                    rect, save, text)

W, H = 1040, 300
LX, LW = 20, 490                    # .text
RX, RW = 650, 370                   # .rodata
TOP, BH = 40, 200


def build():
    out = []
    # .text with the two instructions
    out.append(rect(LX, TOP, LW, BH, FILL_BLUE, BLUE, rx=6, width=1.6))
    out.append(text(LX + 14, TOP + 24, "代码段 .text", 16, BLUE, "bold", anchor="start"))
    y1, y2 = TOP + 66, TOP + 96
    out.append(rect(LX + 10, y1 - 18, LW - 20, 26, FILL_ORANGE, "none", rx=3, width=0))
    out.append(mono(LX + 18, y1, "ab99:  48 8d 15 90 04 0c 00  lea    0xc0490(%rip),%rdx",
                    14, INK))
    out.append(mono(LX + 18, y2, "aba0:  48 8b 45 f8           mov    -0x8(%rbp),%rax",
                    14, INK))
    # the two numbers the sum takes
    out.append(mono(LX + 18, TOP + 150, "%rip", 14, ORANGE, "bold"))
    out.append(text(LX + 60, TOP + 150, "= 下一条指令的地址 = 0xaba0", 14, INK, anchor="start"))
    out.append(mono(LX + 18, TOP + 176, "0xc0490", 14, ORANGE, "bold"))
    out.append(text(LX + 90, TOP + 176, "= 指令中的偏移量（机器码的后 4 个字节 90 04 0c 00，小端）",
                    14, INK, anchor="start"))
    # .rodata with the string
    out.append(rect(RX, TOP, RW, BH, FILL_GREEN, GREEN, rx=6, width=1.6))
    out.append(text(RX + 14, TOP + 24, "只读数据 .rodata", 16, GREEN, "bold", anchor="start"))
    cw, ch = 30, 24
    bx, by = RX + 70, y1 - 18
    out.append(mono(bx - 8, by + 17, "cb030:", 14, INK, "bold", anchor="end"))
    shapes, _ = cells(bx, by, ["54", "68", "65", "20", "73", "74", "61", "63"],
                      cw, ch, WHITE, GREEN, 13)
    out += shapes
    out.append(mono(bx + 8 * cw + 6, by + 17, "…", 14, INK))
    shapes, _ = cells(bx, by + ch, ["T", "h", "e", "␣", "s", "t", "a", "c"],
                      cw, ch, WHITE, GREEN, 13, tone=GREEN)
    out += shapes
    by2 = by + 2 * ch + 20
    out.append(mono(bx - 8, by2 + 17, "cb056:", 14, INK, "bold", anchor="end"))
    shapes, _ = cells(bx, by2, ["2e", "00"], cw, ch, WHITE, GREEN, 13)
    out += shapes
    shapes, _ = cells(bx, by2 + ch, [".", "\\0"], cw, ch, WHITE, GREEN, 13, tone=GREEN)
    out += shapes
    out.append(text(bx + 2 * cw + 10, by2 + ch + 4, "值为 0 的字节结束字符串", 13, MUTED,
                    anchor="start"))
    out.append(text(RX + RW / 2, TOP + BH - 14, "The stack grows toward lower addresses.",
                    14, INK, font=MONO))
    # the sum, from the lea to the first byte: the byte row sits on the line
    # of the lea, so the arrow is straight
    ay = by + ch / 2
    out.append(arrow(LX + LW + 2, ay, bx - 66, ay, INK, 2))
    out.append(mono((LX + LW + RX) / 2, ay - 12, "0xaba0 + 0xc0490", 13, INK, "bold",
                    anchor="middle"))
    out.append(mono((LX + LW + RX) / 2, ay + 22, "= 0xcb030", 13, INK, "bold",
                    anchor="middle"))
    out.append(text(W / 2, H - 14, "objdump 在行尾的 # cb030 写出了这个和；lea 把它写入 %rdx，不读内存",
                    14, MUTED))
    return out


if __name__ == "__main__":
    save("bomb_lea", W, H, build())
