#!/usr/bin/env python3
"""A core with split first-level caches.

The fetch unit reads the instruction cache and the memory unit accesses the data cache.
Instructions (purple) flow one way, data (orange) both ways. L2 belongs to the core, as in
the multilevel figure. The sizes are those of a typical x86-64 processor. Drawn at the
width it is shown on the slide (640 px).
Run it to refresh ../assets/icache-dcache.svg.
"""

from cdraw import *
from cachekit import save

W, H = 640, 456
LEFT, RIGHT, UNIT_W = 34, 346, 260     # the two columns of the core
UNIT_Y, UNIT_H = 44, 54
L1_Y, L1_H = 150, 74
L2_Y, L2_H = 276, 92
LOW_Y, LOW_H = 404, 46


def figure():
    out = [rect(12, 26, W - 24, L2_Y + L2_H + 14 - 26, "none", INK, rx=10, width=2, dash="7 4"),
           text(28, 18, "处理器核", 17, MUTED, "bold", "start")]
    out += box(LEFT, UNIT_Y, UNIT_W, UNIT_H, "取指部件", FILL_PURPLE, PURPLE, 19)
    out += box(RIGHT, UNIT_Y, UNIT_W, UNIT_H, "访存部件", FILL_ORANGE, ORANGE, 19)
    out += box(LEFT, L1_Y, UNIT_W, L1_H, "L1 指令缓存", FILL_BLUE, INK, 19, sub="32 KB · 8 路",
               sub_size=16)
    out += box(RIGHT, L1_Y, UNIT_W, L1_H, "L1 数据缓存", FILL_BLUE, INK, 19, sub="48 KB · 12 路",
               sub_size=16)
    # L2: blocks of both kinds, one of each column above its L1
    out.append(rect(LEFT, L2_Y, RIGHT + UNIT_W - LEFT, L2_H, FILL_BLUE, INK, rx=6, width=1.6))
    mid = (LEFT + RIGHT + UNIT_W) / 2
    out.append(text(mid, L2_Y + 30, "L2 缓存", 19, INK, "bold"))
    out.append(text(mid, L2_Y + 54, "1280 KB · 20 路", 16, MUTED))
    out.append(text(mid, L2_Y + 78, "指令与数据共用", 16, MUTED))
    for x, label, fill, stroke in ((LEFT + 12, "指令块", FILL_PURPLE, PURPLE),
                                   (RIGHT + UNIT_W - 112, "数据块", FILL_ORANGE, ORANGE)):
        out.append(rect(x, L2_Y + 22, 100, 48, fill, stroke, rx=2, width=1.4))
        out.append(text(x + 50, L2_Y + 52, label, 18, INK, "bold"))
    out += box(LEFT, LOW_Y, RIGHT + UNIT_W - LEFT, LOW_H, "L3 与内存", FILL_GREEN, GREEN, 19)
    out.append(arrow(mid, L2_Y + L2_H + 2, mid, LOW_Y - 3, INK, 2, both=True))

    icx, dcx = LEFT + UNIT_W / 2, RIGHT + UNIT_W / 2
    upper, lower = (UNIT_Y + UNIT_H + L1_Y) / 2 + 6, (L1_Y + L1_H + L2_Y) / 2 + 6
    # the instruction side flows upwards only, the data side both ways
    out.append(arrow(icx, L1_Y - 2, icx, UNIT_Y + UNIT_H + 3, PURPLE, 3))
    out.append(text(icx + 12, upper, "取指令", 17, INK, "bold", "start"))
    out.append(arrow(dcx, UNIT_Y + UNIT_H + 3, dcx, L1_Y - 3, ORANGE, 3, both=True))
    out.append(text(dcx + 12, upper, "读写数据", 17, INK, "bold", "start"))
    out.append(arrow(icx, L2_Y - 2, icx, L1_Y + L1_H + 3, PURPLE, 3))
    out.append(text(icx + 12, lower, "读入块", 17, INK, "bold", "start"))
    out.append(arrow(dcx, L1_Y + L1_H + 3, dcx, L2_Y - 3, ORANGE, 3, both=True))
    out.append(text(dcx + 12, lower, "读写块", 17, INK, "bold", "start"))
    return out


if __name__ == "__main__":
    save("icache-dcache", W, H, figure())
