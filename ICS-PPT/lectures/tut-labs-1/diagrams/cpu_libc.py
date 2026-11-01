#!/usr/bin/env python3
"""printf, libc and syscall: the path of one line of output.

Two paths on one PC. Top row: a C program linked with libc, whose printf
calls libc's write. Bottom row: guest/hello, not linked with libc, whose
puts and putint call the write of guest/mini.h. Both writes execute syscall,
which enters the one operating system, Linux, drawn as one short box
between the two rows; the two syscall arrows turn towards it and cross the
dashed line side by side. The library boxes carry only
their names; what each does is in the notes. The dashed line is the
boundary that only syscall crosses. Library code grey, as the framework's
code in this part; the operating system blue; the syscall arrow orange, the
opcode colour.
Run it to refresh ../assets/cpu_libc.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, INK, LINE, MUTED, ORANGE, WHITE, arrow, elbow, line,
                    rect, save, text)
from cpu_kit import GIVEN, code, note

W, H = 1120, 188

COLS = [(16, 252), (318, 240), (608, 200), (930, 174)]     # x, width
SEP = 888                                                  # program | operating system
TURN = 846                                                 # where the syscall arrows turn
RH = 52                                                    # the height of a row
Y1, Y2 = 32, 128                                           # the two rows
OY, OH = 72, 64                                            # the operating system box, between them


def code_box(y, lines):
    """The program's own code: its lines of C, centred vertically."""
    x, w = COLS[0]
    out = [rect(x, y, w, RH, WHITE, LINE, rx=6, width=1.6)]
    for i, s in enumerate(lines):
        out.append(code(x + 12, y + RH / 2 + 4.5 + (i - (len(lines) - 1) / 2) * 20, s, 12.5))
    return out


def lib_box(k, y, name):
    """A library function: its name only, centred."""
    x, w = COLS[k]
    fill, stroke = GIVEN
    return [rect(x, y, w, RH, fill, stroke, rx=6, width=1.6),
            text(x + w / 2, y + RH / 2 + 5, name, 13.5, INK, "bold")]


def row(y, lines, names, into, below=False):
    """One path: code, two library boxes, and syscall into the OS box at height into."""
    out = code_box(y, lines) + lib_box(1, y, names[0]) + lib_box(2, y, names[1])
    for (x0, w0), (x1, _) in zip(COLS[:2], COLS[1:3]):
        out.append(arrow(x0 + w0 + 4, y + RH / 2, x1 - 4, y + RH / 2, INK, 1.8))
    x0, w0 = COLS[2]
    out.append(elbow([(x0 + w0 + 4, y + RH / 2), (TURN, y + RH / 2), (TURN, into),
                      (COLS[3][0] - 4, into)], ORANGE, 2.6))
    ly = into + 18 if below else into - 8           # the label above, or below for the lower path
    out.append(rect(SEP - 28, ly - 14, 56, 18, WHITE, WHITE, rx=0, width=0))
    out.append(code(SEP, ly, "syscall", 12.5, ORANGE, "bold", anchor="middle"))
    return out


def build():
    out = []
    out.append(line(SEP, 26, SEP, H - 6, MUTED, 1.4, "5 4"))
    out.append(note(SEP - 10, 16, "程序中的指令", 12, anchor="end"))
    out.append(note(SEP + 10, 16, "操作系统", 12))

    # the one operating system both paths enter
    x, w = COLS[3]
    out.append(rect(x, OY, w, OH, FILL_BLUE, BLUE, rx=6, width=1.6))
    out.append(text(x + w / 2, OY + OH / 2 + 5, "操作系统 Linux", 14, INK, "bold"))

    out.append(text(16, Y1 - 12, "PC 上的 C 程序：链接 libc", 14, INK, "bold", anchor="start"))
    out += row(Y1, ['printf("argc = %d\\n", argc);'], ["libc 的 printf", "libc 的 write"], OY + 18)

    out.append(text(16, Y2 - 12, "PC 上的 guest/hello：不链接 libc", 14, INK, "bold", anchor="start"))
    out += row(Y2, ['puts("argc = ");', "putint(argc);"],
               ["guest/mini.h 的 puts、putint", "guest/mini.h 的 write"], OY + OH - 18, below=True)
    return out


if __name__ == "__main__":
    save("cpu_libc", W, H, build())
