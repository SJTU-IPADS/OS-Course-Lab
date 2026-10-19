#!/usr/bin/env python3
"""What main of the bomb does: for each of the 8 phases it reads one line,
hands it to the phase, and reports the phase as defused when the phase
returns. A phase that rejects its line calls explode_bomb, which reports the
explosion and ends the program. Run it to refresh ../assets/bomb_main_flow.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_RED, GREEN, INK,
                    MONO, MUTED, RED, arrow, box, elbow, mono, rect, save, text)

W, H = 1000, 290
BW, BH = 210, 64
Y = 70
X1, X2, X3 = 40, 320, 600          # read_line, phase_k, phase_defused
X4 = 870                            # the end


def build():
    out = []
    # the loop over the phases
    out.append(rect(X1 - 20, Y - 42, X3 + BW + 10 - (X1 - 20), BH + 70, "none", MUTED,
                    rx=8, width=1.2, dash="6 4"))
    out.append(text(X1 - 8, Y - 20, "main：k = 1 … 8，每个阶段依次调用三个函数", 15, MUTED,
                    anchor="start"))
    out += box(X1, Y, BW, BH, "line = read_line()", FILL_GREY, MUTED, 17, font=MONO,
               sub="读一行输入", sub_size=14)
    out += box(X2, Y, BW, BH, "phase_k(line)", FILL_BLUE, BLUE, 18, font=MONO,
               sub="检查这一行", sub_size=14)
    out += box(X3, Y, BW, BH, "phase_defused()", FILL_GREY, MUTED, 17, font=MONO,
               sub="报告：阶段 k 已拆除", sub_size=14)
    out.append(arrow(X1 + BW, Y + BH / 2, X2 - 2, Y + BH / 2, INK, 2))
    out.append(mono(X1 + BW + 12, Y + BH / 2 - 8, "line", 14, INK))
    out.append(arrow(X2 + BW, Y + BH / 2, X3 - 2, Y + BH / 2, GREEN, 2))
    out.append(text((X2 + BW + X3) / 2, Y + BH / 2 - 8, "返回", 14, GREEN))
    # the next phase
    out.append(elbow([(X3 + BW / 2, Y + BH), (X3 + BW / 2, Y + BH + 22),
                      (X1 + BW / 2, Y + BH + 22), (X1 + BW / 2, Y + BH + 2)], INK, 2))
    out.append(text((X1 + BW / 2 + X2) / 2, Y + BH + 40, "k < 8：下一个阶段", 14, INK))
    # the end
    out.append(arrow(X3 + BW, Y + BH / 2, X4 - 2, Y + BH / 2, GREEN, 2))
    out.append(text((X3 + BW + X4) / 2 + 6, Y + BH / 2 + 22, "k = 8", 14, GREEN))
    out += box(X4, Y, 110, BH, "退出状态 0", FILL_GREEN, GREEN, 16,
               sub="全部拆除", sub_size=14)
    # the explosion
    EY = Y + BH + 76
    out.append(arrow(X2 + BW / 2, Y + BH, X2 + BW / 2, EY - 2, RED, 2))
    out.append(text(X2 + BW / 2 + 10, Y + BH + 46, "输入错误", 14, RED, "bold",
                    anchor="start"))
    out += box(X2, EY, BW, BH, "explode_bomb()", FILL_RED, RED, 17, font=MONO,
               sub="报告：阶段 k 爆炸", sub_size=14)
    out.append(arrow(X2 + BW, EY + BH / 2, X3 - 2, EY + BH / 2, RED, 2))
    out += box(X3, EY, BW, BH, "退出状态 1", FILL_RED, RED, 16,
               sub="显示爆炸，程序结束", sub_size=14)
    return out


if __name__ == "__main__":
    save("bomb_main_flow", W, H, build())
