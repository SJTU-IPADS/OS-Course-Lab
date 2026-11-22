#!/usr/bin/env python3
"""One miss with a dirty victim, step by step: cache_access(0x13345).

The set continues from the last frame of cache-hit: set 13 holds tags 0x05,
0x12, 0x07 and 0x2a (dirty), last_use 41, 60, 50, 33, cache_use_clock 60.
0x13345 has set 13 and tag 0x13, which no way holds. The frames follow the
TODO steps: cache_lookup finds nothing (a miss), cache_victim takes the way
with the smallest last_use (way 3), cache_fill writes it back since it is
dirty, at cache_line_address(0x2a, 13) = 0x2a340, then reads the line at
cache_line_address(0x13, 13) = 0x13340, and cache_access gives the way the
new last_use. Two transfers: the victim written back, the new line read.
Run this script to refresh ../assets/cache-miss-1.svg ... cache-miss-5.svg.
"""

from cache_kit import *
from cache_hit import BEFORE, GEO, LX, W, H, DRAM_Y, scene, way_y

STATE = [dict(d) for d in BEFORE]
STATE[1]["last_use"] = 60
VICTIM = 3


def transfer(down, key, label):
    """A red arrow between the victim and a line of DRAM, with its label."""
    def draw(boxes, pos):
        x = pos[key]
        y_way = way_y(VICTIM) + GEO.h + 4
        ys, ye = (y_way, DRAM_Y - 4) if down else (DRAM_Y - 4, y_way)
        out = [arrow(x, ys, x, ye, RED, 3)]
        name, rest = label
        tx, ty = x + 14, DRAM_Y - 16
        out.append(mono(tx, ty, name, 14, RED, "bold"))
        out.append(text(tx + width_of(name, 14, MONO) + 8, ty, rest, 14, RED, "bold", "start"))
        return out
    return draw


def frames():
    fields = ("0x13", 13, "0x05")
    pa = "0x13345"
    marks = {w: dict(tag_halo=RED) for w in range(4)}
    yield scene(1, ["cache_lookup"], "4 路的 tag 都不等于 0x13：返回 NULL，缺失", pa, fields,
                STATE, marks=marks, dim=("off",), changed={"cache_misses": "+1"})
    marks = {w: dict(use_halo=(RED if w == VICTIM else LINE)) for w in range(4)}
    marks[VICTIM]["halo"] = RED
    yield scene(2, ["cache_victim"], "4 路都有效：选 last_use 最小的 way 3", pa, fields,
                STATE, marks=marks, dim=("off",))
    marks = {VICTIM: dict(halo=RED, dirty_halo=RED)}
    yield scene(3, ["cache_fill", "cache_line_address"],
                "way 3 是脏行：先写回 (0x2a × 64 + 13) × 64 = 0x2a340", pa, fields, STATE,
                marks=marks, dim=("off",), changed={"cache_writebacks": "+1"},
                dram_marks={"0x2a340": RED},
                extra=transfer(True, "0x2a340", ("dram_line_write", "写回一行")))
    filled = [dict(d) for d in STATE]
    filled[VICTIM] = dict(tag="0x13", last_use=33, dirty=0)
    marks = {VICTIM: dict(halo=RED, tag_halo=TAG)}
    yield scene(4, ["cache_fill", "cache_line_address"],
                "读入 0x13340 的一行：valid = 1，dirty = 0，tag = 0x13", pa, fields, filled,
                marks=marks, dim=("off",), dram_marks={"0x13340": RED},
                extra=transfer(False, "0x13340", ("dram_line_read", "读入一行")))
    done = [dict(d) for d in filled]
    done[VICTIM]["last_use"] = 61
    marks = {VICTIM: dict(halo=INK, use_halo=INK, span=(5, 8))}
    yield scene(5, ["cache_access"], "last_use = 61，cache_read 从 data[5] 起复制", pa, fields,
                done, marks=marks, dim=("tag", "idx"), changed={"cache_use_clock": "60 → 61"},
                extra=lambda boxes, pos: badge(LX + GEO.width + 6, way_y(3) + GEO.h + 30,
                                               "缺失：读写两次内存", "miss"))


if __name__ == "__main__":
    for k, body in enumerate(frames(), 1):
        save(f"cache-miss-{k}", W, H, body)
