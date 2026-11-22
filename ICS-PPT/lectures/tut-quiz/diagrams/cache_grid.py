#!/usr/bin/env python3
"""The cache of lab 2: 64 sets of 4 ways, one array cache_lines.

Left, the drawing of section 3 of src/lab2/cache.c: a row per set, a box per
way, every box a cache_line. cache_init (given) allocates the 256 lines with
calloc, all invalid. A lookup compares only the 4 ways of one set, the
work of cache_lookup. Right, the fields of one cache_line. Below, the same
lines as the one array cache_lines, set after set: way 1 of set 13 is
cache_lines[13 * 4 + 1].
Run this script to refresh ../assets/cache-grid.svg.
"""

from cache_kit import *

W, H = 1040, 384
GX, CW, CH = 120, 66, 26
ROWS = [("0", 52), ("1", 82), ("⋮", 112), ("13", 136), ("⋮", 166), ("63", 190)]
SET13_Y = 136


def grid():
    out = []
    for way in range(4):
        out.append(mono(GX + (way + 0.5) * CW, 40, f"way {way}", 13, MUTED, anchor="middle"))
    for name, y in ROWS:
        if name == "⋮":
            out.append(text(GX + 2 * CW, y + 14, "⋮", 16, MUTED))
            continue
        out.append(text(GX - 12, y + CH - 8, f"组 {name}", 15, IDX, "bold", "end"))
        for way in range(4):
            out.append(rect(GX + way * CW, y, CW, CH, LINE_FILL, LINE_EDGE, rx=0, width=1.3))
    out.append(rect(GX + CW + 2, SET13_Y + 2, CW - 4, CH - 4, "none", INK, rx=2, width=3))
    out.append(rect(GX - 4, SET13_Y - 4, 4 * CW + 8, CH + 8, "none", IDX, rx=4, width=3))
    shapes, x0, x1 = given_fn(GX - 60, 244, "cache_init(64, 4)", 15)
    out += shapes
    out.append(text(x1 + 10, 249, "calloc 256 行，全部 valid = 0", 15, MUTED, "normal", "start"))
    return out


def zoom():
    out = []
    geo = LineGeo(vw=46, dw=46, tw=64, uw=84, dataw=250, h=32, size=16)
    x, y = 520, 56
    out += line_header(x, y - 8, geo)
    out += cache_line(x, y, geo, valid=1, dirty=0, tag="0x12", last_use=57)
    caps = {"valid": "有副本", "dirty": "写过", "tag": "哪一行", "last_use": "最近使用",
            "data": "64 字节的内容"}
    for field, cap in caps.items():
        cx = geo.x_of(field, x) + geo.w_of(field) / 2
        out.append(text(cx, y + geo.h + 22, cap, 14, MUTED))
    # from way 3 of set 0 to the drawn line
    out.append(line(GX + 4 * CW + 4, 52 + CH / 2, x - 8, y + geo.h / 2, LINE_EDGE, 1.4,
                    dash="5 4"))
    return out


def lookup():
    out = []
    y = SET13_Y + CH / 2
    out.append(arrow(GX + 4 * CW + 60, y, GX + 4 * CW + 10, y, IDX, 2))
    shapes, x0, x1 = fn(GX + 4 * CW + 66, y, "cache_lookup(13, tag)", 15)
    out += shapes
    out.append(text(x1 + 12, y + 6, "只比较组 13 的 4 路：valid = 1 且 tag 相等", 15, INK, "normal",
                    "start"))
    return out


def array():
    out = []
    y, cw, ch = 318, 24, 26
    out.append(mono(GX - 60, y - 34, "cache_lines", 15, INK, "bold"))
    out.append(text(GX - 60 + width_of("cache_lines", 15, MONO) + 8, y - 34,
                    "一个数组，一组的 4 路相邻", 15, MUTED, "normal", "start"))
    x = GX - 60
    groups = [("组 0", 0), ("组 1", 1), ("…", None), ("组 13", 13), ("…", None), ("组 63", 63)]
    target = None
    for label, s in groups:
        if s is None:
            out.append(text(x + 22, y + 19, "…", 16, MUTED))
            x += 44
            continue
        for way in range(4):
            hot = s == 13
            out.append(rect(x + way * cw, y, cw, ch, LINE_FILL, IDX if hot else LINE_EDGE,
                            rx=0, width=2 if hot else 1.2))
            if s == 13 and way == 1:
                target = (x + way * cw + cw / 2, y + ch)
                out.append(rect(x + way * cw + 2, y + 2, cw - 4, ch - 4, "none", INK, rx=2,
                                width=3))
        out.append(text(x + 2 * cw, y - 8, label, 14, IDX, "bold"))
        x += 4 * cw + 10
    tx, ty = target
    lx = x + 10
    out.append(path(f"M {tx:.1f} {ty + 2:.1f} L {tx:.1f} {ty + 26:.1f} L {lx - 6:.1f} {ty + 26:.1f}",
                    INK, 1.6, head=False))
    out.append(mono(lx, ty + 31, "cache_lines[13 * 4 + 1]", 15, INK, "bold"))
    out.append(text(lx + width_of("cache_lines[13 * 4 + 1]", 15, MONO), ty + 31,
                    "：组 13 的 way 1", 15, INK, "bold", "start"))
    return out


def build():
    return grid() + zoom() + lookup() + array()


if __name__ == "__main__":
    save("cache-grid", W, H, build())
