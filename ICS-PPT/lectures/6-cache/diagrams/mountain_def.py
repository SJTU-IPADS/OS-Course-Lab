#!/usr/bin/env python3
"""The two parameters of the memory mountain on an array of long: the working set
is a prefix of the array (size), the reads step through it every `stride`
elements. A thick line marks the 64-byte blocks (8 longs). Requested elements
are orange, the colour of requested data on the other figures.
Run it to refresh ../assets/mountain-def.svg.
"""

from cdraw import *
from svgkit import brace

W, H = 1100, 332
X0, CW, CH = 156, 32, 48
N, WORK = 28, 24                           # cells drawn, cells in the working set


def row(y, stride, label):
    out = [text(X0 - 16, y + CH / 2 + 7, label, 20, INK, "bold", "end")]
    for i in range(N):
        inside = i < WORK
        hot = inside and i % stride == 0
        fill = FILL_ORANGE if hot else (WHITE if inside else FILL_GREY)
        stroke = ORANGE if hot else LINE
        out.append(rect(X0 + i * CW, y, CW, CH, fill, stroke, rx=0, width=1.6 if hot else 1))
    return out


def build():
    out = []
    # the array and its working set
    out += row(66, 99, "数组")
    for i in range(0, N + 1, 8):
        out.append(line(X0 + i * CW, 60, X0 + i * CW, 66 + CH + 6, INK, 2.4))
    out += brace(X0, X0 + WORK * CW, 52, BLUE, "工作集：数组的前 size 个字节", 20, depth=8, below=False)
    out.append(text(X0 + 4 * CW, 66 + CH + 36, "一个块：8 个 long", 17, MUTED))
    out += row(184, 1, "步长 1")
    out += row(256, 4, "步长 4")
    return out


if __name__ == "__main__":
    save("mountain-def", W, H, build())
