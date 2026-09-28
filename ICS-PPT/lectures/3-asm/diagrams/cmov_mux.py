#!/usr/bin/env python3
"""One selection compiled two ways, and cmovge as a multiplexer.

The C function is

    int max(int a, int b) { int v = a; if (a < b) v = b; return v; }

and the listings are gcc -Og and gcc -O2 with -fcf-protection=none, passed
through ../examples/asm.sed. -Og selects with jl and the label .L2; -O2
selects with cmovge and has no jump. Right: cmovge writes %edi into %eax when
SF = OF and leaves %eax (holding b) as it is otherwise.
Run it to refresh ../assets/cmov-mux.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, LINE,
                    MONO, MUTED, ORANGE, WHITE, arrow, box, elbow, listing, mono,
                    rect, save, text)

W, H = 1120, 350
SZ, LH = 18, 34                         # listing font size and row height
TOP = 50                                # top of the listing boxes
BOX_W, BOX_H = 320, 6 * LH + 20


def row_y(i):
    """Vertical centre of listing row i."""
    return TOP + 10 + i * LH + LH / 2


def jump_version():
    x = 10
    lines = ["  movl  %esi, %eax", "  cmpl  %esi, %edi", "  jl    .L2",
             "  movl  %edi, %eax", ".L2:", "  ret"]
    out = [text(x, 30, "gcc -Og：条件跳转", 18, INK, "bold", anchor="start"),
           rect(x, TOP, BOX_W, BOX_H, WHITE, LINE, rx=6, width=1.4)]
    out += listing(x + 20, TOP + 10, lines, SZ, LH, marks={2: FILL_ORANGE},
                   width=BOX_W - 28)
    out.append(elbow([(x + 150, row_y(2)), (x + 270, row_y(2)), (x + 270, row_y(4)),
                      (x + 72, row_y(4))], ORANGE, 2.2))
    out.append(text(x + 280, row_y(3) + 5, "a < b", 15, ORANGE, "bold", anchor="start",
                    font=MONO))
    out.append(text(x, TOP + BOX_H + 34, "条件跳转 jl 与标号 .L2", 18, ORANGE, "bold",
                    anchor="start"))
    return out


def cmov_version():
    x = 360
    lines = ["  cmpl    %esi, %edi", "  movl    %esi, %eax", "  cmovge  %edi, %eax",
             "  ret"]
    out = [text(x, 30, "gcc -O2：条件传送", 18, INK, "bold", anchor="start"),
           rect(x, TOP, BOX_W, BOX_H, WHITE, LINE, rx=6, width=1.4)]
    out += listing(x + 20, TOP + 10, lines, SZ, LH, marks={2: FILL_GREEN},
                   width=BOX_W - 28)
    out.append(text(x, TOP + BOX_H + 34, "没有跳转指令，按顺序执行", 18, GREEN, "bold",
                    anchor="start"))
    return out


def mux():
    out = [mono(720, 30, "cmovge %edi, %eax", 18, INK, "bold")]
    out += box(720, 70, 150, 58, "%eax", FILL_BLUE, BLUE, 18, font=MONO, sub="原值 b")
    out += box(720, 190, 150, 58, "%edi", FILL_BLUE, BLUE, 18, font=MONO, sub="a")
    x, y0, y1 = 920, 56, 262
    d = f"M {x} {y0} L {x + 60} {y0 + 40} L {x + 60} {y1 - 40} L {x} {y1} Z"
    out.append(f'<path d="{d}" fill="{FILL_ORANGE}" stroke="{ORANGE}" stroke-width="2"/>')
    out.append(text(x + 30, 165, "MUX", 16, INK, "bold"))
    out.append(mono(x + 8, 105, "0", 15, MUTED))
    out.append(mono(x + 8, 225, "1", 15, MUTED))
    out.append(arrow(870, 99, 916, 99, BLUE, 2.2))
    out.append(arrow(870, 219, 916, 219, BLUE, 2.2))
    out += box(1010, 130, 100, 58, "%eax", FILL_GREEN, GREEN, 18, font=MONO, sub="结果")
    out.append(arrow(980, 159, 1006, 159, GREEN, 2.4))
    out += box(875, 290, 150, 50, "SF = OF ?", WHITE, ORANGE, 17, sub="RFLAGS")
    out.append(arrow(950, 288, 950, 248, ORANGE, 2.2))
    return out


def build():
    return jump_version() + cmov_version() + mux()


if __name__ == "__main__":
    save("cmov-mux", W, H, build())
