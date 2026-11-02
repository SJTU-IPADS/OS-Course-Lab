#!/usr/bin/env python3
"""Three steps of a read in a direct-mapped cache (S = 4, E = 1, B = 2, m = 4).
The address is 13 = 1101: tag 1, set index 10 = 2, block offset 1. Frame 1 selects
the set with the index, frame 2 compares the valid bit and the tag of its only
line (a hit), frame 3 picks the byte with the offset. Run it to refresh
../assets/dm-access-1.svg ... dm-access-3.svg.
"""

from cachekit import *

W, H = 1000, 300
GEO = Geo(vw=30, tw=46, bw=56, lh=36)
CX, Y0, GAP = 470, 24, 30
BITS = "1101"
STATE = [(1, "1", ["m[8]", "m[9]"]), None, (1, "1", ["m[12]", "m[13]"]),
         (1, "0", ["m[6]", "m[7]"])]
CHOSEN = 2


def frame(n):
    out = []
    set_top, set_bottom = Y0, Y0 + 4 * GEO.lh + 3 * GAP     # the stack of sets; the address bar is centred on it
    mid_y = (set_top + set_bottom) / 2
    bar_x, bar_y = 130, mid_y - 20
    # fields not used in this step are washed out
    dim = {1: ("tag", "off"), 2: ("idx", "off"), 3: ("tag", "idx")}[n]
    shapes, boxes = addr_bits(bar_x, bar_y, BITS, 1, 2, 1, cw=54, ch=40, size=22, dim=dim)
    out += shapes
    out.append(text(bar_x - 18, bar_y + 28, "地址 13 =", 20, INK, "bold", "end"))
    sets = []
    for s, e in enumerate(STATE):
        kw = dict(v=0) if e is None else dict(v=e[0], tag=e[1], data=e[2])
        kw["dim"] = s != CHOSEN
        if s == CHOSEN:
            if n == 2:
                kw.update(tag_halo=GREEN, v_halo=GREEN)
            if n == 3:
                kw.update(req=1)
        sets.append([kw])
    shapes, pos, bottom = cache_sets(CX, Y0, GEO, 2, sets, set_gap=GAP, headers=False,
                                     group=False, label_dx=14)
    out += shapes
    cy = pos[(CHOSEN, 0)][1] + GEO.lh / 2
    ax, ay = boxes["idx"][0] + 54, bar_y + 40 + 2
    if n == 1:
        out.append(elbow([(ax, ay), (ax, cy), (CX - 64, cy)], BLUE, 2.4))
        out.append(rect(CX - 60, cy - 17, 54, 34, "none", BLUE, rx=6, width=3))
    right = CX + GEO.width(2) + 36
    if n == 2:
        out.append(text(right, cy - 4, "有效位 = 1，标记 1 = 地址的标记 1", 17, INK, "bold",
                        "start"))
        out += badge(right + 28, cy + 26, "hit", "hit")
    if n == 3:
        bx = CX + GEO.vw + GEO.tw + 1.5 * GEO.bw
        out.append(arrow(bx + 30, cy, right + 20, cy, ORANGE, 2.4))
        out.append(text(right + 30, cy + 6, "返回 m[13] 给 CPU", 17, ORANGE, "bold", "start"))
        off_x = boxes["off"][1] + 2
        out.append(elbow([(off_x, mid_y), (bx, mid_y), (bx, cy - GEO.lh / 2 - 2)], ORANGE, 2.2))
        out.append(text(off_x + 14, mid_y + 20, "块偏移 1", 15, ORANGE, "bold", "start"))
    caps = {1: ("① 组选择", "组索引 10 = 2，选中组 2", IDX),
            2: ("② 行匹配", "组 2 的行有效，标记相等：hit", GREEN),
            3: ("③ 字抽取", "块偏移 1 选出块内第 1 个字节", ORANGE)}
    head, rest, col = caps[n]
    out.append(text(bar_x, H - 8, head, 20, col, "bold", "start"))
    out.append(text(bar_x + 100, H - 8, rest, 20, INK, anchor="start"))
    return out


if __name__ == "__main__":
    for n in (1, 2, 3):
        save(f"dm-access-{n}", W, H, frame(n))
