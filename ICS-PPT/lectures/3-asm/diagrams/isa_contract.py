#!/usr/bin/env python3
"""Software above, the ISA in the middle, two different chips below it.

The orange band is the contract: what it lists is all the compiler may rely
on. Both chips hang under the same band and run the same executable, but the
blocks inside them are drawn in different numbers and arrangements, which is
the point of page 5.
Run it to refresh ../assets/isa-contract.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MONO,
                    MUTED, ORANGE, WHITE, arrow, box, line, rect, save, text)

W, H = 1120, 520
LEFT = 170                                       # layer labels sit left of this


def layer_label(y, name, sub):
    out = [text(20, y, name, 19, INK, "bold", anchor="start")]
    if sub:
        out.append(text(20, y + 24, sub, 15, MUTED, anchor="start"))
    return out


def software():
    out = layer_label(62, "软件", "")
    xs = [LEFT + 10, LEFT + 330, LEFT + 650]
    items = [("ollama 源代码", "Go 与 C/C++"), ("编译器", "go build 与 gcc"),
             ("可执行文件", "ollama")]
    for x, (name, sub) in zip(xs, items):
        out += box(x, 20, 250, 70, name, FILL_GREY, LINE, size=18, sub=sub)
    for a, b in zip(xs, xs[1:]):
        out.append(arrow(a + 254, 55, b - 4, 55, INK, 2))
    return out


def isa_band():
    y0, y1 = 130, 250
    out = [rect(LEFT, y0, W - 20 - LEFT, y1 - y0, FILL_ORANGE, "none", rx=0, width=0),
           line(LEFT, y0, W - 20, y0, ORANGE, 4), line(LEFT, y1, W - 20, y1, ORANGE, 4)]
    out += layer_label(182, "ISA", "接口")
    out.append(text((LEFT + W - 20) / 2, y0 + 34, "x86-64 指令集架构：软硬件之间的契约",
                    19, INK, "bold"))
    parts = ["程序员可见状态", "指令编码与格式", "数据类型与寻址模式"]
    cw = (W - 20 - LEFT - 40) / 3
    for k, s in enumerate(parts):
        x = LEFT + 20 + k * cw
        out.append(rect(x + 8, y0 + 54, cw - 16, 46, WHITE, ORANGE, rx=6, width=1.4))
        out.append(text(x + cw / 2, y0 + 83, s, 16, INK))
    return out


def chip(x, y, w, h, name, arch, blocks):
    """A chip outline with pins and a grid of named blocks inside."""
    out = []
    for k in range(9):
        px = x + 30 + k * (w - 60) / 8
        out.append(line(px, y - 8, px, y, MUTED, 2))
        out.append(line(px, y + h, px, y + h + 8, MUTED, 2))
    out.append(rect(x, y, w, h, FILL_BLUE, BLUE, rx=8, width=2))
    out.append(text(x + w / 2, y + 30, name, 17, INK, "bold"))
    out.append(text(x + w / 2, y + 52, arch, 15, MUTED))
    for bx, by, bw, bh, s in blocks:
        out.append(rect(x + bx, y + by, bw, bh, WHITE, LINE, rx=3, width=1.2))
        out.append(text(x + bx + bw / 2, y + by + bh / 2 + 5, s, 14, INK))
    return out


def hardware():
    out = layer_label(392, "微架构", "实现")
    cw, ch, cy = 400, 188, 300
    xa, xb = LEFT + 50, W - 20 - 50 - cw
    intel = [(16, 66, 176, 36, "流水线"), (208, 66, 176, 36, "分支预测器"),
             (16, 110, 110, 30, "ALU"), (136, 110, 110, 30, "ALU"),
             (256, 110, 128, 30, "寄存器堆"), (16, 148, 368, 26, "乱序调度窗口")]
    amd = [(16, 66, 368, 30, "乱序调度窗口"), (16, 104, 86, 30, "ALU"),
           (110, 104, 86, 30, "ALU"), (204, 104, 86, 30, "ALU"),
           (298, 104, 86, 30, "ALU"), (16, 142, 120, 32, "寄存器堆"),
           (144, 142, 110, 32, "流水线"), (262, 142, 122, 32, "分支预测器")]
    out += chip(xa, cy, cw, ch, "Intel Core i9-11900H", "Tiger Lake 微架构", intel)
    out += chip(xb, cy, cw, ch, "AMD Ryzen 7000", "Zen 4 微架构", amd)
    for x in (xa + cw / 2, xb + cw / 2):
        out.append(arrow(x, 252, x, cy - 12, ORANGE, 2.4, both=True))
    out.append(text(W / 2 + (LEFT - 20) / 2, 280, "同一个可执行文件", 15, ORANGE, "bold"))
    return out


def build():
    exe = LEFT + 650 + 125                       # centre of the executable's box
    return software() + [arrow(exe, 94, exe, 126, INK, 2)] + isa_band() + hardware()


if __name__ == "__main__":
    save("isa-contract", W, H, build())
