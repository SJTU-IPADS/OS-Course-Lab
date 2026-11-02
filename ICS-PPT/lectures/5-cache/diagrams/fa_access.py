#!/usr/bin/env python3
"""Three steps of a read in a fully associative cache (S = 1, E = 8, B = 2, m = 4).
The address is 9 = 1001: tag 100, no set index, block offset 1. Frame 1: there is one set,
so the set needs no selecting. Frame 2: the tag of the address is compared with the tag
of every line at once, the invalid lines take no part, one line matches: a hit. Frame 3:
the block offset picks the byte of the matching line. The comparisons come from the data
below. Run it to refresh ../assets/fa-access-1.svg ... fa-access-3.svg.
"""

from cachekit import *

W, H = 472, 440
LEFT = 40
GEO = Geo(vw=26, tw=50, bw=48, lh=32, size=15)
CX, Y0 = 150, 112
BITS, TAGBITS = "1001", "100"
LINES = [(1, "011", ["m[6]", "m[7]"]), (1, "000", ["m[0]", "m[1]"]), None,
         (1, "110", ["m[12]", "m[13]"]), (1, "010", ["m[4]", "m[5]"]),
         (1, "100", ["m[8]", "m[9]"]), None, (1, "111", ["m[14]", "m[15]"])]
MATCH = [k for k, e in enumerate(LINES) if e is not None and e[1] == TAGBITS]
CAPS = {1: ("① 组选择", "S = 1，整个缓存是一组，不需要组索引", IDX),
        2: ("② 行匹配", f"同时比较 8 个标记，第 {MATCH[0] + 1} 行匹配：hit", GREEN),
        3: ("③ 字抽取", "块偏移 1 选出匹配行内的第 1 个字节", ORANGE)}


def frame(n):
    out = []
    dim = {1: ("tag", "off"), 2: ("off",), 3: ("tag",)}[n]
    shapes, boxes = addr_bits(CX, 52, BITS, 3, 0, 1, cw=43, ch=34, size=20, dim=dim)
    out += shapes
    out.append(text(CX - 12, 76, "地址 9 =", 20, INK, "bold", "end"))
    sets = [[]]
    for k, e in enumerate(LINES):
        kw = dict(v=0) if e is None else dict(v=e[0], tag=e[1], data=e[2])
        if n == 2 and e is not None:
            kw["tag_halo"] = GREEN if k in MATCH else RED
        if n == 3:
            if k in MATCH:
                kw["req"] = 1
            else:
                kw["dim"] = True
        sets[0].append(kw)
    shapes, pos, bottom = cache_sets(CX, Y0, GEO, 2, sets, set_gap=0, headers=False,
                                     labels=False, group=True)
    out += shapes
    if n == 1:
        out.append(rect(CX - 9, Y0 - 7, GEO.width(2) + 18, bottom - Y0 + 14, "none", BLUE,
                        rx=7, width=3))
    out.append(text(CX - 40, (Y0 + bottom) / 2 + 2, "组 0", 19, IDX, "bold", "end"))
    out.append(text(CX - 40, (Y0 + bottom) / 2 + 26, "S = 1", 16, MUTED, anchor="end"))
    out += vbrace(CX - 22, Y0, bottom, INK, "", 16, right=False)
    tx = CX + GEO.width(2) + 22
    for k, e in enumerate(LINES):
        y = pos[(0, k)][1] + GEO.lh / 2 + 6
        if n == 2:
            if e is None:
                out.append(text(tx, y, "无效，不参与比较", 15, MUTED, anchor="start"))
            elif k in MATCH:
                out.append(text(tx, y, "100 = 100：匹配", 16, GREEN, "bold", "start"))
            else:
                out.append(text(tx, y, f"{e[1]} ≠ 100", 15, RED, anchor="start"))
        if n == 3 and k in MATCH:
            yy = pos[(0, k)][1] + GEO.lh / 2
            bx = CX + GEO.vw + GEO.tw + 1.5 * GEO.bw
            out.append(arrow(bx + 30, yy, tx + 12, yy, ORANGE, 2.4))
            out.append(text(tx + 20, yy + 6, "返回 m[9] 给 CPU", 16, ORANGE, "bold", "start"))
    head, rest, col = CAPS[n]
    caption = [text(CX - 100, 416, head, 19, col, "bold", "start"),
               text(CX - 100 + 96, 416, rest, 19, INK, anchor="start")]
    if n == 1:
        caption = [part.replace(">", ' text-decoration="line-through">', 1)
                   for part in caption]
    out += caption
    return out


if __name__ == "__main__":
    for n in (1, 2, 3):
        save(f"fa-access-{n}", W, H, frame(n), left=LEFT)
