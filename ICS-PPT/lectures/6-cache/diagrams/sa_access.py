#!/usr/bin/env python3
"""Three steps of a read in a 2-way set-associative cache (S = 2, E = 2, B = 2, m = 4).
The address is 9 = 1001: tag 10, set index 0, block offset 1. Frame 1 selects the
set with the index, frame 2 compares the tags of both lines of the set (one match:
a hit), frame 3 picks the byte with the offset. Run it to refresh
../assets/sa-access-1.svg ... sa-access-3.svg.
"""

from cachekit import *

W, H = 1000, 250
GEO = Geo(vw=30, tw=46, bw=56, lh=36)
CX, Y0, GAP = 470, 24, 30
BITS = "1001"
STATE = [[(1, "00", ["m[0]", "m[1]"]), (1, "10", ["m[8]", "m[9]"])],
         [(1, "01", ["m[6]", "m[7]"]), None]]
CHOSEN = 0


def frame(n):
    out = []
    set_top, set_bottom = Y0, Y0 + 4 * GEO.lh + GAP     # the stack of sets; the address bar is centred on it
    mid_y = (set_top + set_bottom) / 2
    bar_x, bar_y = 130, mid_y - 20
    dim = {1: ("tag", "off"), 2: ("idx", "off"), 3: ("tag", "idx")}[n]
    shapes, boxes = addr_bits(bar_x, bar_y, BITS, 2, 1, 1, cw=54, ch=40, size=22, dim=dim, labels=False)
    out += shapes
    for name, word in (("tag", "标记"), ("idx", "组索引"), ("off", "块偏移")):
        x0, x1 = boxes[name]                      # field names under the bar, the arrows leave from above
        out.append(text((x0 + x1) / 2, bar_y + 40 + 22, word, 15, FIELD_COLOR[name] if name not in dim else MUTED,
                        "bold"))
    out.append(text(bar_x - 18, bar_y + 28, "地址 9 =", 20, INK, "bold", "end"))
    sets = []
    for s, lines_ in enumerate(STATE):
        group = []
        for k, e in enumerate(lines_):
            kw = dict(v=0) if e is None else dict(v=e[0], tag=e[1], data=e[2])
            kw["dim"] = s != CHOSEN
            if s == CHOSEN and n == 2:
                kw.update(tag_halo=GREEN if k == 1 else RED)
            if s == CHOSEN and n == 3 and k == 1:
                kw.update(req=1, halo=None)
            if s == CHOSEN and n == 3 and k == 0:
                kw["dim"] = True
            group.append(kw)
        sets.append(group)
    shapes, pos, bottom = cache_sets(CX, Y0, GEO, 2, sets, set_gap=GAP, headers=False,
                                     group=True, label_dx=14)
    out += shapes
    top = pos[(CHOSEN, 0)][1]
    mid = top + GEO.lh
    right = CX + GEO.width(2) + 36
    if n == 1:
        ax = boxes["idx"][0] + 27
        out.append(elbow([(ax, bar_y - 2), (ax, mid), (CX - 72, mid)], BLUE, 2.4))
        out.append(rect(CX - 68, mid - 17, 52, 34, "none", BLUE, rx=6, width=3))
    if n == 2:
        out.append(text(right, top + 24, "标记 00 ≠ 10", 17, RED, "bold", "start"))
        out.append(text(right, top + GEO.lh + 24, "有效位 = 1，标记 10 = 10", 17, GREEN, "bold",
                        "start"))
        out += badge(right + 28, top + 2 * GEO.lh + 22, "命中", "hit")
    if n == 3:
        bx = CX + GEO.vw + GEO.tw + 1.5 * GEO.bw
        yy = top + GEO.lh + GEO.lh / 2
        out.append(arrow(bx + 34, yy, right + 20, yy, ORANGE, 2.4))
        out.append(text(right + 30, yy + 6, "返回 m[9] 给 CPU", 17, ORANGE, "bold", "start"))
        off_x = boxes["off"][1] + 2
        out.append(elbow([(off_x, mid_y), (bx, mid_y), (bx, top + 2 * GEO.lh + 2)], ORANGE, 2.2))
        out.append(text(off_x + 14, mid_y - 8, "块偏移 1", 15, ORANGE, "bold", "start"))
    caps = {1: ("① 组选择", "组索引 0，选中组 0", IDX),
            2: ("② 行匹配", "同时比较组内两行的标记，第二行匹配：命中", GREEN),
            3: ("③ 字抽取", "块偏移 1 选出匹配行内的第 1 个字节", ORANGE)}
    head, rest, col = caps[n]
    out.append(text(bar_x, H - 8, head, 20, col, "bold", "start"))
    out.append(text(bar_x + 100, H - 8, rest, 20, INK, anchor="start"))
    return out


if __name__ == "__main__":
    for n in (1, 2, 3):
        save(f"sa-access-{n}", W, H, frame(n))
