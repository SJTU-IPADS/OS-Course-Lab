#!/usr/bin/env python3
"""One hit, step by step: cache_access(0x12345) in a 64 x 4 cache.

The address is the example of src/lab2/cache.c: set 13, tag 0x12, offset 5.
Set 13 holds four valid lines; the one with tag 0x12 is in way 1. The
frames follow the TODO steps of cache_access: the two fields, the search of
cache_lookup, then the hit (cache_hits, cache_use_clock, last_use) and the
copy of cache_read. The state of the set is an example made up for the
figure; cache_miss.py continues from the last frame.

scene() is shared with cache_miss.py.
Run this script to refresh ../assets/cache-hit-1.svg ... cache-hit-3.svg.
"""

from cache_kit import *

W, H = 1040, 400
GEO = LineGeo(vw=52, dw=52, tw=64, uw=84, dataw=240, h=32, size=16)
LX, LY, LGAP = 160, 132, 42            # first way of set 13
DRAM_Y = 352
DRAM = [("…", None), ("0x12340", "0x12340"), ("0x13340", "0x13340"), ("…", None),
        ("0x2a340", "0x2a340"), ("…", None)]

BEFORE = [dict(tag="0x05", last_use=41, dirty=0), dict(tag="0x12", last_use=57, dirty=0),
          dict(tag="0x07", last_use=50, dirty=0), dict(tag="0x2a", last_use=33, dirty=1)]


def way_y(way):
    return LY + way * LGAP


def address(pa, tag, set_index, offset, dim=()):
    out = [mono(36, 52, f"pa = {pa}", 18, INK, "bold")]
    shapes, boxes = addr_fields(36 + 150, 30, (110, 96, 96), (f"tag {tag}", f"组 {set_index}",
                                                              f"偏移 {offset}"), h=36, size=16,
                                font=FONT, dim=dim)
    return out + shapes, boxes


def set_lines(state, marks=None):
    """The four ways of set 13; marks[way] = dict of cache_line keyword arguments."""
    marks = marks or {}
    out = line_header(LX, LY - 10, GEO, size=13)
    for way, line_ in enumerate(state):
        kw = dict(valid=1, dirty=line_["dirty"], tag=line_["tag"], last_use=line_["last_use"])
        kw.update(marks.get(way, {}))
        out += cache_line(LX, way_y(way), GEO, **kw)
        out.append(mono(LX - 12, way_y(way) + 21, f"way {way}", 14, MUTED, anchor="end"))
    top, bottom = way_y(0) - 6, way_y(3) + GEO.h + 6
    out.append(rect(LX - 82, top, GEO.width + 88, bottom - top, "none", LINE, rx=7, width=1.4,
                    dash="5 3"))
    out.append(text(LX - 92, (top + bottom) / 2 + 6, "组 13", 17, IDX, "bold", "end"))
    return out


def dram(marks=None):
    marks = marks or {}
    out = [text(LX - 92, DRAM_Y + 21, "内存", 17, MEM_EDGE, "bold", "end")]
    x = LX
    pos = {}
    for label, key in DRAM:
        w = 60 if key is None else 112
        if key is None:
            out.append(text(x + w / 2, DRAM_Y + 21, "…", 16, MUTED))
        else:
            out.append(rect(x, DRAM_Y, w, 30, MEM_FILL, MEM_EDGE, rx=2, width=1.3))
            out.append(mono(x + w / 2, DRAM_Y + 20, label, 14, INK, anchor="middle"))
            if key in marks:
                out.append(rect(x - 3, DRAM_Y - 3, w + 6, 36, "none", marks[key], rx=4, width=2.6))
            pos[key] = x + w / 2
        x += w + 8
    return out, pos


def counters(changed):
    """cache_hits, cache_misses, cache_writebacks, cache_use_clock; changed = {name: text}."""
    out = []
    x, y = 744, LY - 4
    out.append(text(x, y - 18, "frame/cpu/dram.c 的计数器", 14, MUTED, "bold", "start"))
    for k, name in enumerate(("cache_hits", "cache_misses", "cache_writebacks",
                              "cache_use_clock")):
        ry = y + k * 34
        value = changed.get(name, "")
        hot = bool(value)
        col = {"cache_hits": GREEN, "cache_misses": RED, "cache_writebacks": RED}.get(name, INK)
        out.append(rect(x, ry, 270, 28, FILL_GREY if not hot else WHITE, col if hot else LINE,
                        rx=4, width=2 if hot else 1.2))
        out.append(mono(x + 10, ry + 19, name, 14, INK if hot else MUTED,
                        "bold" if hot else "normal"))
        if value:
            out.append(mono(x + 262, ry + 19, value, 14, col, "bold", anchor="end"))
    return out


def step(n, pills, words, y=88):
    """The step caption at the top right: number, the functions, one line of words."""
    out = []
    x = 560
    out.append(text(x, y - 40, f"第 {n} 步", 15, MUTED, "bold", "start"))
    px = x + 58
    for name in pills:
        shapes, _, x1 = fn(px, y - 45, name, 14)
        out += shapes
        px = x1 + 8
    out.append(text(x, y - 12, words, 15, INK, "bold", "start"))
    return out


def scene(n, pills, words, pa, fields, state, marks=None, dim=(), changed=None,
          dram_marks=None, extra=None):
    out = []
    shapes, boxes = address(pa, *fields, dim=dim)
    out += shapes
    out += step(n, pills, words)
    out += set_lines(state, marks)
    out += counters(changed or {})
    shapes, pos = dram(dram_marks)
    out += shapes
    if extra:
        out += extra(boxes, pos)
    return out


def select_arrow(boxes, pos):
    x0, x1 = boxes["idx"]
    cx = (x0 + x1) / 2
    return [elbow([(cx, 66), (cx, 84), (LX - 120, 84), (LX - 120, way_y(1) + 6)], IDX, 2)]


def frames():
    fields = ("0x12", 13, "0x05")
    pa = "0x12345"
    yield scene(1, ["cache_set_index", "cache_tag"], "组号 13，tag 0x12：只在组 13 中找",
                pa, fields, BEFORE, extra=select_arrow)
    marks = {w: dict(tag_halo=(GREEN if w == 1 else RED)) for w in range(4)}
    yield scene(2, ["cache_lookup"], "4 路中 way 1 有效且 tag 为 0x12：返回 way 1",
                pa, fields, BEFORE, marks=marks, dim=("off",))
    after = [dict(d) for d in BEFORE]
    after[1]["last_use"] = 60
    marks = {1: dict(halo=GREEN, use_halo=GREEN, span=(5, 8))}
    yield scene(3, ["cache_access"], "命中：last_use = 60，cache_read 从 data[5] 起复制",
                pa, fields, after, marks=marks, dim=("tag", "idx"),
                changed={"cache_hits": "+1", "cache_use_clock": "59 → 60"},
                extra=lambda boxes, pos: badge(LX + GEO.width + 44, way_y(1) + 16, "命中", "hit"))


if __name__ == "__main__":
    for k, body in enumerate(frames(), 1):
        save(f"cache-hit-{k}", W, H, body)
