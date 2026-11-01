#!/usr/bin/env python3
"""One read of x walked down the cache levels and back, five frames.

Every core has its own L1 (instruction and data) and L2, all cores share L3, and below L3
is main memory. In the walk x is in L3 only: L1 misses, L2 misses, L3 hits, the block is
filled into L2 and L1, and x goes to the processor. Requests are orange, a block filled
upwards is green, the steps already taken stay drawn in a light colour. Capacities and
latencies are in the table of the next page. Drawn at the width it is shown on the slide
(640 px).
Run it to refresh ../assets/multilevel-1.svg ... multilevel-5.svg.
"""

from cdraw import *

W, H = 640, 526
CORE_Y, CORE_H, CORE_W = 28, 266, 306
CPU_Y, L1_Y, L2_Y = 40, 124, 218
CPU_H, L1_H, L2_H = 46, 52, 56
L3_Y, L3_H = 332, 58
MEM_Y, MEM_H = 426, 54
L1I_DX, L1D_DX, L1_W = 16, 160, 130    # the two first-level caches, relative to the core
CORE0, CORE1 = 8, 326
COL = CORE0 + L1D_DX + L1_W / 2        # the column of the walk, through the data cache

CAPTIONS = ["① 读 x：先访问 L1 数据缓存，不命中",
            "② 访问 L2，不命中",
            "③ 访问 L3，命中",
            "④ 含 x 的块从 L3 填入 L2 与 L1",
            "⑤ 把 x 送到处理器"]


def chip(x, y):
    return [rect(x, y, 30, 26, FILL_ORANGE, ORANGE, rx=3, width=1.8),
            text(x + 15, y + 19, "x", 18, INK, "bold", font=MONO)]


def halo(x, y, w, h, colour):
    return rect(x - 3, y - 3, w + 6, h + 6, "none", colour, rx=8, width=3.6)


def level(x, y, w, h, label, size=19, has_chip=False):
    """A level of the hierarchy; the chip of x stands at its right end."""
    out = [rect(x, y, w, h, FILL_BLUE, INK, rx=6, width=1.6)]
    if has_chip:
        out.append(text(x + (w - 36) / 2, y + h / 2 + size * 0.36, label, size, INK, "bold"))
        out += chip(x + w - 38, y + h / 2 - 13)
    else:
        out.append(text(x + w / 2, y + h / 2 + size * 0.36, label, size, INK, "bold"))
    return out


def core(x, name, walked=(), l1_chip=False, l2_chip=False, cpu_chip=False):
    """One core; the gaps in `walked` carry the arrows of the walk instead of a grey one."""
    out = [rect(x, CORE_Y, CORE_W, CORE_H, "none", INK, rx=8, width=1.8, dash="7 4"),
           text(x + CORE_W / 2, CORE_Y - 8, name, 17, MUTED, "bold")]
    out += box(x + L1I_DX, CPU_Y, CORE_W - 2 * L1I_DX, CPU_H, "处理器", WHITE, INK, 19)
    if cpu_chip:
        out += chip(x + CORE_W - L1I_DX - 38, CPU_Y + CPU_H / 2 - 13)
    out += level(x + L1I_DX, L1_Y, L1_W, L1_H, "L1 指令")
    out += level(x + L1D_DX, L1_Y, L1_W, L1_H, "L1 数据", has_chip=l1_chip)
    out += level(x + L1I_DX, L2_Y, CORE_W - 2 * L1I_DX, L2_H, "L2", has_chip=l2_chip)
    for dx in (L1I_DX + L1_W / 2, L1D_DX + L1_W / 2):
        on_walk = x + dx == COL
        if not (on_walk and 1 in walked):
            out.append(arrow(x + dx, CPU_Y + CPU_H + 3, x + dx, L1_Y - 3, LINE, 1.8, both=True))
        if not (on_walk and 2 in walked):
            out.append(arrow(x + dx, L1_Y + L1_H + 3, x + dx, L2_Y - 3, LINE, 1.8, both=True))
    return out


def frame(n):
    walked = {1: {1}, 2: {1, 2}, 3: {1, 2, 3}, 4: {2, 3}, 5: {1, 2, 3}}[n]
    out = core(CORE0, "核 0", walked, l1_chip=n >= 4, l2_chip=n >= 4, cpu_chip=n == 5)
    out += ['<g opacity="0.4">'] + core(CORE1, "核 1") + ["</g>"]
    out += level(CORE0, L3_Y, W - 2 * CORE0, L3_H, "L3（所有核共享）", has_chip=n >= 3)
    out += box(CORE0, MEM_Y, W - 2 * CORE0, MEM_H, "内存", FILL_GREEN, GREEN, 19)
    if 3 not in walked:
        out.append(arrow(COL, L2_Y + L2_H + 3, COL, L3_Y - 3, LINE, 1.8, both=True))
    out.append(arrow(CORE1 + L1D_DX + L1_W / 2, L2_Y + L2_H + 3, CORE1 + L1D_DX + L1_W / 2, L3_Y - 3,
                     LINE, 1.8, both=True))
    out.append(arrow(W / 2, L3_Y + L3_H + 3, W / 2, MEM_Y - 3, LINE, 1.8, both=True))

    gaps = [(CPU_Y + CPU_H + 3, L1_Y - 3), (L1_Y + L1_H + 3, L2_Y - 3),
            (L2_Y + L2_H + 3, L3_Y - 3)]
    for k, (y0, y1) in enumerate(gaps, 1):
        if n <= 3 and k <= n:                   # the request going down
            out.append(arrow(COL, y0, COL, y1, ORANGE if k == n else LINE, 3))
        if n >= 4 and k >= 2:                   # the block coming back up
            out.append(arrow(COL, y1, COL, y0, GREEN if n == 4 else LINE, 3))
    if n == 4:
        out.append(text(COL - 12, (gaps[1][0] + gaps[1][1]) / 2 + 6, "块", 17, GREEN, "bold", "end"))
        out.append(text(COL - 12, (CORE_Y + CORE_H + L3_Y) / 2 + 6, "块", 17, GREEN, "bold", "end"))
    if n == 5:
        out.append(arrow(COL, gaps[0][1], COL, gaps[0][0], ORANGE, 3))
    if n == 1:
        out.append(halo(CORE0 + L1D_DX, L1_Y, L1_W, L1_H, RED))
        out.append(text(COL - 14, (gaps[1][0] + gaps[1][1]) / 2 + 6, "不命中", 17, RED, "bold", "end"))
    if n == 2:
        out.append(halo(CORE0 + L1I_DX, L2_Y, CORE_W - 2 * L1I_DX, L2_H, RED))
        out.append(text(CORE0 + L1I_DX + 16, L2_Y + L2_H / 2 + 6, "不命中", 17, RED, "bold", "start"))
    if n == 3:
        out.append(halo(CORE0, L3_Y, W - 2 * CORE0, L3_H, GREEN))
        out.append(text(CORE0 + 18, L3_Y + L3_H / 2 + 6, "命中", 17, GREEN, "bold", "start"))
    out.append(text(W / 2, H - 12, CAPTIONS[n - 1], 20, INK, "bold"))
    return out


if __name__ == "__main__":
    for n in range(1, 6):
        save(f"multilevel-{n}", W, H, frame(n))
