#!/usr/bin/env python3
"""A hit and a miss with registers and cache inside one CPU, in three frames.

A hit returns the requested word from the cache. A miss fetches its block
from external memory, replaces an old block in this full-cache example,
and returns the requested word to the CPU's registers.
Run this script to refresh ../assets/cache-hit-miss-{1..3}.svg.
"""

import cache_between as model
from svgkit import GREEN, ORANGE, RED, save, text
from cachekit import badge


W, H = model.W, model.ANIMATION_H
CAPTIONS = {
    1: ("hit：A 所在的块在缓存中，缓存直接返回 A 处的字", GREEN),
    2: ("miss：A 所在的块不在缓存中，缓存向内存请求整个块", RED),
    3: ("取回整个块，替换旧块，向寄存器返回 A 处的字", ORANGE),
}


def frame(number):
    states = ["valid"] * model.LINES
    if number == 1:
        states[model.HIT_LINE] = "hit"
    else:
        states[model.NEW_LINE] = "old" if number == 2 else "new"
    out = model.structure(cache_states=states, memory_selected=number >= 2)
    out += model.access_times(model.Y_REQ - 40)
    out += model.lane(model.AX0, model.AX1, model.Y_REQ, ORANGE,
                      "地址 A", False)
    if number in (1, 3):
        out += model.lane(model.AX0, model.AX1, model.Y_REP, ORANGE,
                          "A 处的字", True, False)
    if number >= 2:
        out += model.lane(model.BX0, model.BX1, model.Y_REQ, GREEN,
                          "块的地址", number == 2)
    if number == 3:
        out += model.lane(model.BX0, model.BX1, model.Y_REP, GREEN,
                          "整个块", True, False)
        out.append(text((model.AX0 + model.AX1) / 2,
                        model.CPU_Y + model.CPU_H - 16,
                        "替换旧块", 17, ORANGE, "bold"))
    else:
        kind = "hit" if number == 1 else "miss"
        out += badge((model.AX0 + model.AX1) / 2,
                     model.CPU_Y + model.CPU_H - 22, kind, kind, size=16)
    label, color = CAPTIONS[number]
    out.append(text(W / 2, H - 14, label, 20, color, "bold"))
    return out


if __name__ == "__main__":
    for number in range(1, 4):
        save(f"cache-hit-miss-{number}", W, H, frame(number))
