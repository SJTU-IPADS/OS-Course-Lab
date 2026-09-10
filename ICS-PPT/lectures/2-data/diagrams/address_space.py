#!/usr/bin/env python3
"""How much of a 32-bit address space one model file takes.

The bar is the whole 4 GB a 32-bit pointer can name; the orange part is the
weight file. Everything else a process needs has to fit in what is left.
Run it to refresh ../assets/address-space.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, LINE, MONO, MUTED,
                    ORANGE, brace, line, rect, svg, text)

W, H = 840, 208
X0, X1 = 76, 800
BAR_Y, BAR_H = 74, 48
FILE_GB, SPAN_GB = 1.9, 4.0


def build():
    px = (X1 - X0) / SPAN_GB
    fx = X0 + FILE_GB * px
    out = [text(W / 2, 30, "32 位地址空间：指针能命名的全部字节", 15.5, INK, "bold"),
           rect(X0, BAR_Y, X1 - X0, BAR_H, FILL_BLUE, BLUE),
           rect(X0, BAR_Y, fx - X0, BAR_H, FILL_ORANGE, ORANGE),
           text((X0 + fx) / 2, BAR_Y + BAR_H / 2 + 5, "权重文件 1.9 GB", 14,
                ORANGE, "bold"),
           text((fx + X1) / 2, BAR_Y + BAR_H / 2 + 5,
                "代码、栈、堆与其他映射共用余下的部分", 13.5, MUTED)]

    for gb in range(int(SPAN_GB) + 1):
        x = X0 + gb * px
        out.append(line(x, BAR_Y + BAR_H, x, BAR_Y + BAR_H + 7, LINE, 1.4))
        out.append(text(x, BAR_Y + BAR_H + 24, f"{gb} GB", 11.5, MUTED, font=MONO))

    out += brace(X0, X1, BAR_Y - 6, BLUE, "2³² 个地址，共 4 GB", below=False)

    out.append(text(W / 2, 186, "64 位地址空间是它的 43 亿倍：同一比例下，这条横条要画 43 亿条",
                    14, INK, "bold"))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "address-space.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
