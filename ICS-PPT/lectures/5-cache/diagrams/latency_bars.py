#!/usr/bin/env python3
"""One serial read through a single cache, with all time lengths to scale.

The teaching model spends 1 ns checking the cache. A miss adds 100 ns to fetch
the block from memory and transfer the data, so its total is 101 ns.
Run it to refresh ../assets/metrics-time.svg.
"""

from cdraw import *

HIT_NS, PENALTY_NS = 1, 100


def metrics_time():
    width, height = 560, 190
    start, unit, bar_height = 80, 4, 32
    hit_width, penalty_width = HIT_NS * unit, PENALTY_NS * unit
    miss_end = start + hit_width + penalty_width
    out = [text(start, 24, "一次读取的用时（按比例）", 18, MUTED, "bold", "start")]

    for label, top in (("hit", 56), ("miss", 112)):
        out.append(text(start - 12, top + 23, label, 19, INK, "bold", "end"))
        out.append(rect(start, top, hit_width, bar_height, FILL_BLUE, BLUE, rx=0, width=1.4))

    out.append(text(start + hit_width + 12, 79, "1 ns", 19, BLUE, "bold", "start"))
    out.append(rect(start + hit_width, 112, penalty_width, bar_height,
                    FILL_RED, RED, rx=0, width=1.4))
    out.append(text(start + hit_width + penalty_width / 2, 135,
                    "从内存取块并传输", 18, INK, "bold"))
    out.append(text(miss_end + 8, 135, "101 ns", 19, INK, "bold", "start"))
    out += brace(start + hit_width, miss_end, 150, RED,
                 f"miss 惩罚：{PENALTY_NS} ns", 18)
    return width, height, out


if __name__ == "__main__":
    save("metrics-time", *metrics_time())
