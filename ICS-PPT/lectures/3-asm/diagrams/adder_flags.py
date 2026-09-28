#!/usr/bin/env python3
"""One adder, one bit string, two readings of the same result.

0xffffffff + 1 in addl: as unsigned it is 4294967295 + 1 and carries out,
so CF = 1; as int it is -1 + 1 = 0 with no overflow, so OF = 0 and SF = 0.
The adder does not know which reading the program meant; the branch that
reads CF (jb) or SF/OF (jl) decides that later.
Run it to refresh ../assets/adder-flags.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK,
                    MUTED, ORANGE, WHITE, arrow, box, elbow, mono, rect, save, text)

W, H = 1120, 490
AX, Y0, Y1 = 420, 100, 410          # adder: left edge, top, bottom
MID = (Y0 + Y1) / 2


def operand(y, name, bits, u, s):
    return [rect(20, y, 330, 112, WHITE, INK, rx=6, width=1.6),
            mono(40, y + 36, f"{name} = {bits}", 22, INK, "bold"),
            text(40, y + 70, "unsigned：" + u, 19, ORANGE, anchor="start"),
            text(40, y + 98, "int：" + s, 19, BLUE, anchor="start")]


def build():
    out = operand(119, "a", "0xffffffff", "4294967295", "-1")
    out += operand(279, "b", "0x00000001", "1", "1")
    d = (f"M {AX} {Y0} L {AX + 100} {Y0 + 70} L {AX + 100} {Y1 - 70} L {AX} {Y1} "
         f"L {AX} {MID + 24} L {AX + 22} {MID} L {AX} {MID - 24} Z")
    out.append(f'<path d="{d}" fill="{FILL_GREY}" stroke="{INK}" stroke-width="2"/>')
    out.append(text(AX + 60, MID + 12, "+", 34, INK, "bold"))
    out.append(mono(AX + 50, Y1 + 36, "addl", 20, INK, "bold", anchor="middle"))
    out.append(text(AX + 50, Y1 + 66, "只有一个加法器", 18, MUTED))
    out.append(arrow(350, 175, AX - 4, 175, INK, 2))
    out.append(arrow(350, 335, AX - 4, 335, INK, 2))
    # result
    out += box(600, MID - 32, 260, 64, "0x00000000", FILL_GREY, INK, 22,
               font="SFMono-Regular, Menlo, DejaVu Sans Mono, Consolas, monospace")
    out.append(arrow(AX + 100, MID, 596, MID, INK, 2.4))
    out.append(text(730, MID + 62, "写回 %eax：两种解释下位串相同", 17, MUTED))
    # unsigned path, leaving the upper slant of the adder
    out.append(elbow([(AX + 86, Y0 + 60), (560, Y0 + 60), (560, 60), (716, 60)],
                     ORANGE, 2.4))
    out.append(text(574, 46, "最高位进位", 17, ORANGE, anchor="start"))
    out += box(720, 28, 110, 64, "CF = 1", FILL_ORANGE, ORANGE, 22)
    out.append(text(850, 54, "无符号：结果超出范围", 18, INK, "bold", anchor="start"))
    out.append(text(850, 82, "由 jb、ja 等读取", 18, ORANGE, anchor="start"))
    # signed path, leaving the lower slant
    out.append(elbow([(AX + 86, Y1 - 60), (560, Y1 - 60), (560, 450), (716, 450)],
                     BLUE, 2.4))
    out.append(text(574, 436, "符号位与溢出检测", 17, BLUE, anchor="start"))
    out += box(720, 418, 110, 64, "OF = 0", FILL_BLUE, BLUE, 22)
    out.append(text(850, 444, "有符号：-1 + 1 = 0，SF = 0", 18, INK, "bold",
                    anchor="start"))
    out.append(text(850, 472, "由 jl、jg 等读取", 18, BLUE, anchor="start"))
    return out


if __name__ == "__main__":
    save("adder-flags", W, H, build())
