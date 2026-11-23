#!/usr/bin/env python3
"""Who calls whom in src/lab2/cache.c, and the calls one line access makes.

The framework calls the four functions of mcpu.h: frame/main.c cache_init
and cache_flush, frame/cpu/mem.c cache_read and cache_write. cache_read and
cache_write receive bytes within one line and look the line up with
cache_access, which uses
cache_set_index, cache_tag, cache_lookup, and on a miss cache_victim and
cache_fill. cache_fill and cache_flush find the address of a line with
cache_line_address and move it with dram_line_write and dram_line_read of
frame/cpu/dram.c. The order follows the TODO comments of the skeleton.

Every node and edge has a name. build(None) draws the whole graph
(cache-funcs.svg). build(PATH) draws the calls of one read access at full
strength, with thicker edges, and fades everything else (FADE): a hit, a
miss whose victim is invalid or clean, and a miss whose victim is dirty
(cache-funcs-hit/-miss/-evict.svg, one slide each, on the same canvas).
Run this script to refresh the four figures in ../assets/.
"""

from cache_kit import *

W, H = 1080, 486
R0, R1, R2, R3, R4, R5 = 30, 116, 200, 284, 364, 438
CAP = 15                       # caption size
BOLD = 2.6                     # width of the edges of a path

HIT = {"mem.c", "cache_read", "cache_access", "cache_set_index", "cache_tag", "cache_lookup",
       "mem.c>cache_read", "cache_read>cache_access", "cache_access>cache_set_index",
       "cache_access>cache_tag", "cache_access>cache_lookup"}
MISS = HIT | {"cache_victim", "cache_fill", "cache_line_address", "dram_line_read", "dram.c",
              "cache_access>cache_victim", "cache_access>cache_fill",
              "cache_fill>cache_line_address", "cache_fill>dram_line_read"}
EVICT = MISS | {"dram_line_write", "cache_fill>dram_line_write"}
PATHS = {"hit": HIT, "miss": MISS, "evict": EVICT}


def node(kind, x, y, name, caption=None, size=15):
    shapes, x0, x1 = kind(x, y, name, size, "middle")
    if caption:
        shapes.append(text(x, y + 30, caption, CAP, MUTED))
    return shapes, (x0, x1)


def caller(x, y, name):
    w = width_of(name, 14, MONO) + 24
    return [rect(x - w / 2, y - 15, w, 30, FILL_GREY, MUTED, rx=6, width=1.3),
            mono(x, y + 5, name, 14, INK, anchor="middle")]


def build(on=None):
    """The graph; with on, a set of names, only those at full strength."""
    out = []

    def put(name, shapes):
        if on is None or name in on:
            out.extend(shapes)
        else:
            out.extend([f'<g opacity="{FADE}">'] + shapes + ["</g>"])

    def width(name, normal):
        return BOLD if on is not None and name in on else normal

    put("main.c", caller(220, R0, "frame/main.c"))
    put("mem.c", caller(650, R0, "frame/cpu/mem.c"))
    out.append(rect(20, R0 + 34, W - 40, R4 + 44 - (R0 + 34), WHITE, INK, rx=10, width=2.4))
    out.extend(chip(W / 2, R0 + 34, "src/lab2/cache.c", "student"))

    for kind, x, name, cap in ((given_fn, 120, "cache_init", "建立空的缓存（已写出）"),
                               (fn, 320, "cache_flush", "脏行全部写回"),
                               (fn, 560, "cache_read", "从一行复制出"),
                               (fn, 740, "cache_write", "复制入一行，置 dirty")):
        put(name, node(kind, x, R1, name, cap)[0])
    for x, name in ((120, "cache_init"), (320, "cache_flush")):
        edge = f"main.c>{name}"
        put(edge, [arrow(220 + (x - 220) * 0.35, R0 + 15, x, R1 - 15, INK, width(edge, 1.6))])
    for x, name in ((560, "cache_read"), (740, "cache_write")):
        edge = f"mem.c>{name}"
        put(edge, [arrow(650 + (x - 650) * 0.35, R0 + 15, x, R1 - 15, INK, width(edge, 1.6))])

    put("cache_access", node(fn, 650, R2, "cache_access", "一行：命中或缺失，计数，last_use")[0])
    for x0, x1, name in ((560, 630, "cache_read"), (740, 670, "cache_write")):
        edge = f"{name}>cache_access"
        put(edge, [arrow(x0, R1 + 40, x1, R2 - 15, INK, width(edge, 1.6))])

    row3 = [(395, "cache_set_index", "组号"), (552, "cache_tag", "tag"),
            (690, "cache_lookup", "在一组中找 tag"), (836, "cache_victim", "选被替换的路"),
            (975, "cache_fill", "写回脏行，读入新行")]
    for x, name, cap in row3:
        put(name, node(fn, x, R3, name, cap)[0])
        edge = f"cache_access>{name}"
        put(edge, [arrow(650 + (x - 650) * 0.12, R2 + 40, x, R3 - 15, INK, width(edge, 1.4))])

    shapes, (la0, la1) = node(fn, 560, R4, "cache_line_address", "由 tag 与组号求行的地址")
    put("cache_line_address", shapes)
    edge = "cache_fill>cache_line_address"
    put(edge, [arrow(950, R3 + 40, la1 + 4, R4 - 6, INK, width(edge, 1.4))])

    # the framework below the cache: dram_line_write and dram_line_read
    shapes, w0, w1 = frame_fn(760, R5, "dram_line_write", 15, "middle")
    put("dram_line_write", shapes)
    shapes, r0_, r1_ = frame_fn(950, R5, "dram_line_read", 15, "middle")
    put("dram_line_read", shapes)
    put("dram.c", [text(855, R5 + 32, "frame/cpu/dram.c：一次读写一行，访问宿主的内存", CAP, RED, "bold")])
    edge = "cache_fill>dram_line_read"
    put(edge, [arrow(965, R3 + 40, 950, R5 - 15, RED, width(edge, 1.8))])
    edge = "cache_fill>dram_line_write"
    put(edge, [arrow(985 - 30, R3 + 40, 790, R5 - 15, RED, width(edge, 1.8))])

    # cache_flush: down the left side to the line address and to dram_line_write
    fx = 320
    put("cache_flush>cache_line_address",
        [path(f"M {fx:.1f} {R1 + 40:.1f} L {fx:.1f} {R4:.1f} L {la0 - 4:.1f} {R4:.1f}", INK, 1.4)])
    put("cache_flush>dram_line_write",
        [path(f"M {fx - 20:.1f} {R1 + 40:.1f} L {fx - 20:.1f} {R5:.1f} "
              f"L {w0 - 4:.1f} {R5:.1f}", RED, 1.8)])
    return out


if __name__ == "__main__":
    save("cache-funcs", W, H, build())
    for case, names in PATHS.items():
        save(f"cache-funcs-{case}", W, H, build(names))
