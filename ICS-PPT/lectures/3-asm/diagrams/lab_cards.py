#!/usr/bin/env python3
"""The two labs as cards: the command flow, then what to record.

Commands are the page's own, shortened to the step that matters.
Run it to refresh ../assets/lab-cards.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, GREEN, INK, LINE, WHITE, arrow,
                    mono, rect, save, text)

W, H = 1120, 420
CWD, GAP = 540, 16

LABS = [("实验一", "反汇编对照与寻址模式", BLUE, FILL_BLUE,
         ["gcc -Og -S dot.c", "gcc -c dot.s", "objdump -d dot.o"],
         ["标出 (%rsi,%r8,4) 寻址", "换用 -O2：addq $4 步进指针"]),
        ("实验二", "向量加速比与性能测量", GREEN, FILL_GREEN,
         ["make -C examples", "perf stat -e instructions,cycles", "dot_scalar / dot_avx2"],
         ["指令数、周期数、加速比", "Windows：WSL2 或 clock_gettime", "n 超出 L3：加速比变化"])]


def build():
    out = []
    for k, (tag, title, stroke, fill, cmds, notes) in enumerate(LABS):
        x = 12 + k * (CWD + GAP)
        out.append(rect(x, 8, CWD, 404, WHITE, stroke, rx=8, width=1.8))
        out.append(rect(x, 8, CWD, 58, fill, stroke, rx=8, width=1.8))
        out.append(text(x + 16, 32, tag, 16, stroke, "bold", anchor="start"))
        out.append(text(x + 16, 56, title, 19, INK, "bold", anchor="start"))
        y = 84
        for j, c in enumerate(cmds):
            out.append(rect(x + 16, y, CWD - 32, 34, WHITE, LINE, rx=4, width=1.2))
            out.append(mono(x + 28, y + 23, c, 16, INK))
            if j + 1 < len(cmds):
                out.append(arrow(x + CWD / 2, y + 34, x + CWD / 2, y + 48, stroke, 1.6))
            y += 48
        y = 272
        out.append(text(x + 16, y, "观察与记录", 17, stroke, "bold", anchor="start"))
        for j, s in enumerate(notes):
            out.append(text(x + 16, y + 34 + j * 32, "· " + s, 17, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("lab-cards", W, H, build())
