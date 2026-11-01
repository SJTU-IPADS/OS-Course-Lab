#!/usr/bin/env python3
"""A miss in a direct-mapped cache (m = 4, S = 4, B = 2) replaces the line of the set,
in five frames. Read of address 13 = 1101: (1) the address is split into tag 1, set
index 10 and block offset 1; (2) the set index selects set 2, whose line holds block
2 with tag 0; (3) the tag 0 of the line differs from the tag 1 of the address, a miss;
(4) the block offset is cleared to get the first address of the block, 12, and main
memory returns the aligned block of 2 bytes, 12 and 13; (5) the block, the tag of the
address and a valid bit of 1 are written into the line. Every frame has the same
coordinates; only the marked parts change. Run it to refresh
../assets/dm-replacement-1.svg ... dm-replacement-5.svg.
"""

from cachekit import *

W, H = 1000, 362
GEO = Geo(vw=30, tw=46, bw=60, lh=40)
ADDR_X, ADDR_Y = 150, 38
LINE_X, LINE_Y = 190, 130
MEM_X0, MEM_W, MEM_Y = 52, 112, 262
TARGET = 6
CAPTIONS = {1: ("读地址 13 = 1101：标记 1，组索引 10，块偏移 1", INK),
            2: ("组索引 10 = 2，选出组 2，其中的一行保存块 2，标记 0", IDX),
            3: ("行的标记 0 与地址的标记 1 不等：不命中", RED),
            4: ("块偏移清零得块起始地址 12，内存读出块 6，即 m[12] 与 m[13]", GREEN),
            5: ("整行替换：写入数据，标记写为 1，有效位置 1", ORANGE)}


def frame(n):
    out = []
    dim = {2: ("tag", "off"), 3: ("idx", "off")}.get(n, ())
    shapes, boxes = addr_bits(ADDR_X, ADDR_Y, "1101", 1, 2, 1, cw=54, ch=40, size=22, dim=dim)
    out += shapes
    out.append(text(ADDR_X - 16, ADDR_Y + 27, "地址 13 =", 19, INK, "bold", "end"))
    if n >= 4:
        shapes, _ = addr_bits(740, ADDR_Y, "1100", 1, 2, 1, cw=54, ch=40, size=22, labels=False)
        out += shapes
        out.append(text(722, ADDR_Y + 27, "块起始地址 12 =", 19, INK, "bold", "end"))
        out.append(arrow(400, ADDR_Y + 20, 540, ADDR_Y + 20, ORANGE, 2.2))
        out.append(text(470, ADDR_Y + 12, "块偏移清零", 16, ORANGE, "bold"))
    # main memory strip
    out.append(text(MEM_X0, MEM_Y - 12, "内存", 17, GREEN, "bold", "start"))
    for k in range(8):
        x = MEM_X0 + k * MEM_W
        hot = k == TARGET and n >= 4
        out.append(rect(x, MEM_Y, MEM_W - 4, 50, FILL_GREEN, ORANGE if hot else GREEN, rx=2,
                        width=3.2 if hot else 1.4))
        out.append(text(x + (MEM_W - 4) / 2, MEM_Y + 21, f"块 {k}", 16, INK, "bold"))
        out.append(text(x + (MEM_W - 4) / 2, MEM_Y + 41, f"地址 {2 * k}–{2 * k + 1}", 14,
                        MUTED))
    # the line of set 2
    if n >= 2:
        w = GEO.width(2)
        if n == 5:
            kw = dict(v=1, tag="1", data=["m[12]", "m[13]"], req=1, halo=ORANGE)
        else:
            kw = dict(v=1, tag="0", data=["m[4]", "m[5]"])
            if n == 2:
                kw["halo"] = BLUE
            if n == 3:
                kw["tag_halo"] = RED
        out.append(text(LINE_X - 16, LINE_Y + 27, "组 2", 19, IDX, "bold", "end"))
        out += cache_line(LINE_X, LINE_Y, GEO, 2, **kw)
        old = "替换后：块 6，标记 1，有效位 1" if n == 5 else "替换前：块 2，标记 0"
        out.append(text(LINE_X + w / 2, LINE_Y + 68, old, 16, INK if n == 5 else MUTED,
                        "bold" if n == 5 else "normal"))
    if n == 2:
        ix = (boxes["idx"][0] + boxes["idx"][1]) / 2
        out.append(arrow(ix, ADDR_Y + 44, ix, LINE_Y - 8, BLUE, 2.4))
    if n == 3:
        tx = (boxes["tag"][0] + boxes["tag"][1]) / 2
        out.append(elbow([(tx, ADDR_Y + 44), (tx, 106), (LINE_X + GEO.vw + GEO.tw / 2, 106),
                          (LINE_X + GEO.vw + GEO.tw / 2, LINE_Y - 8)], PURPLE, 2.2))
        out.append(text(LINE_X + GEO.width(2) + 30, LINE_Y + 27, "0 ≠ 1", 22, RED, "bold",
                        "start"))
        out += badge(LINE_X + GEO.width(2) + 140, LINE_Y + 20, "不命中", "miss", size=16)
    if n == 4:
        bx = MEM_X0 + TARGET * MEM_W + (MEM_W - 4) / 2
        out.append(elbow([(bx, MEM_Y - 4), (bx, LINE_Y + 20), (LINE_X + GEO.width(2) + 12,
                                                              LINE_Y + 20)], GREEN, 3))
        out.append(text(bx - 14, LINE_Y + 12, "读出块 6：m[12]、m[13]", 17, GREEN, "bold", "end"))
    out.append(text(W / 2, H - 14, CAPTIONS[n][0], 20, CAPTIONS[n][1], "bold"))
    return out


if __name__ == "__main__":
    for n in range(1, 6):
        save(f"dm-replacement-{n}", W, H, frame(n))
