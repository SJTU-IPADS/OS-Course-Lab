#!/usr/bin/env python3
"""The four GCC stages as a pipeline of files and the programs between them.

Each file box carries its suffix and what kind of file it is; each hop carries
the program gcc runs for that stage and the option that stops gcc after it.
main.o joins at the link step, as in `gcc dot.o main.o -o dot_product`.
Run it to refresh ../assets/toolchain.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED,
                    ORANGE, WHITE, arrow, box, elbow, line, mono, rect, save, text)

W, H = 1120, 330
FW, GAP = 124, 118                  # file box width, hop width
FY, FH = 130, 84                    # file box top and height

FILES = [("dot.c", "C 源文件", WHITE), ("dot.i", "预处理后的 C", WHITE),
         ("dot.s", "汇编文本", WHITE), ("dot.o", "ELF 可重定位", FILL_GREY),
         ("dot_product", "ELF 可执行文件", FILL_GREY)]
STAGES = [("cpp", "① 预处理", "-E"), ("cc1", "② 编译", "-S"),
          ("as", "③ 汇编", "-c"), ("ld", "④ 链接", "")]


def fx(i):
    return 12 + i * (FW + GAP)


def doc(x, y, name, kind, fill):
    """A file box with a folded corner."""
    f = 16
    d = (f"M {x} {y} L {x + FW - f} {y} L {x + FW} {y + f} L {x + FW} {y + FH} "
         f"L {x} {y + FH} Z")
    return [f'<path d="{d}" fill="{fill}" stroke="{BLUE}" stroke-width="1.6"/>',
            f'<path d="M {x + FW - f} {y} L {x + FW - f} {y + f} L {x + FW} {y + f}" '
            f'fill="none" stroke="{BLUE}" stroke-width="1.2"/>',
            mono(x + FW / 2, y + 38, name, 17, INK, "bold", anchor="middle"),
            text(x + FW / 2, y + 64, kind, 14, MUTED)]


def build():
    out = []
    for i, (name, kind, fill) in enumerate(FILES):
        out += doc(fx(i), FY, name, kind, fill)
    mid = FY + FH / 2
    for i, (tool, stage, opt) in enumerate(STAGES):
        x0, x1 = fx(i) + FW + 6, fx(i + 1) - 6
        cx = (x0 + x1) / 2
        out += box(cx - 48, 20, 96, 64, tool, FILL_ORANGE, ORANGE, size=19,
                   font="SFMono-Regular, Menlo, DejaVu Sans Mono, Consolas, monospace",
                   sub=stage, sub_size=14)
        out.append(line(cx, 84, cx, mid - 8, ORANGE, 1.4, dash="4 3"))
        out.append(arrow(x0, mid, x1, mid, INK, 2.4))
        if opt:
            out.append(mono(cx, mid + 26, "gcc " + opt, 15, ORANGE, "bold",
                            anchor="middle"))
    # main.o joins the link step
    mx = fx(3)
    out += doc(mx, 236, "main.o", "ELF 可重定位", FILL_GREY)
    lx = (fx(3) + FW + 6 + fx(4) - 6) / 2
    out.append(elbow([(mx + FW + 6, 278), (lx, 278), (lx, mid + 8)], INK, 2))
    return out


if __name__ == "__main__":
    save("toolchain", W, H, build())
