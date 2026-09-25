#!/usr/bin/env python3
"""CPU, Memory and Disk, with Memory as the thing that has addresses.

Four sample bytes written as 8 bits so that “contents are 0s and 1s”
is visible, not just asserted. The disk side is the 1.9 GB weight file.
Writes ../assets/cpu-memory-disk.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import (BLUE, FILL_BLUE, FILL_GREY, INK, LINE, MONO, MUTED, cells,
                    line, rect, svg, text)

W, H = 1000, 300

BYTES = [
    ("0x00", "01000111"),
    ("0x01", "01000111"),
    ("0x02", "01010101"),
    ("0x03", "01000110"),
]


def arrow(x1, y1, x2, y2, color=BLUE):
    """A short shaft with a head at each end."""
    # Head size along the shaft.
    hx, hy = 7.0, 4.5
    left = [
        f'<polygon points="{x1:.1f},{y1:.1f} {x1 + hx:.1f},{y1 - hy:.1f} '
        f'{x1 + hx:.1f},{y1 + hy:.1f}" fill="{color}"/>',
        f'<polygon points="{x2:.1f},{y2:.1f} {x2 - hx:.1f},{y2 - hy:.1f} '
        f'{x2 - hx:.1f},{y2 + hy:.1f}" fill="{color}"/>',
        line(x1 + hx, y1, x2 - hx, y2, color, 1.6),
    ]
    return left


def build():
    out = []

    # CPU (muted, smaller)
    out.append(rect(28, 78, 148, 164, FILL_GREY, LINE, rx=8, width=1.4))
    out.append(text(102, 112, "CPU", 18, MUTED, "bold"))
    out.append(text(102, 142, "运算", 14, MUTED))
    out.append(text(102, 168, "只能按地址", 13, MUTED))
    out.append(text(102, 190, "读写内存", 13, MUTED))

    # Memory (the subject)
    out.append(rect(220, 18, 560, 264, FILL_BLUE, BLUE, rx=8, width=2.2))
    out.append(text(500, 46, "Memory", 20, INK, "bold"))
    out.append(text(500, 68, "每个字节一个地址，里面是 8 个 0/1", 13.5, BLUE, "bold"))

    cw, ch = 30.0, 26.0
    x_bits = 430
    for i, (addr, bits) in enumerate(BYTES):
        y = 86 + i * 36
        out.append(text(410, y + ch / 2 + 5, addr, 13, INK, "bold",
                        anchor="end", font=MONO))
        out += cells(x_bits, y, list(bits), cw, ch, "#ffffff", BLUE, 13)[0]

    out.append(text(410, 86 + 4 * 36 + 14, "…", 16, MUTED, anchor="end"))
    out.append(text(x_bits + 4 * cw, 86 + 4 * 36 + 14, "…", 16, MUTED))
    out.append(text(500, 266, "装入后每个字节一个地址，存的是 8 个 0/1", 12.5, MUTED))

    # Disk (muted, smaller)
    out.append(rect(824, 78, 148, 164, FILL_GREY, LINE, rx=8, width=1.4))
    out.append(text(898, 112, "Disk", 18, MUTED, "bold"))
    out.append(text(898, 142, "文件", 14, MUTED))
    out.append(text(898, 168, "1.9 GB", 13, MUTED, font=MONO))
    out.append(text(898, 190, "权重在磁盘上", 13, MUTED))

    # Buses
    out += arrow(180, 160, 216, 160, BLUE)
    out.append(text(198, 148, "按地址", 11.5, BLUE, "bold"))
    out += arrow(784, 160, 820, 160, LINE)
    out.append(text(802, 148, "读入", 11.5, MUTED, "bold"))

    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "cpu-memory-disk.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
