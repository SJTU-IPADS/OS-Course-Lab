#!/usr/bin/env python3
"""The charts of the performance pages; every length is computed from the numbers shown.

level-latency.svg: the access latency of each level, to scale.
metrics-time.svg: one hit and one miss in L1, to scale.
Drawn at the width they are shown on the slide.
Run it to refresh ../assets/level-latency.svg and metrics-time.svg.
"""

from cdraw import *

LEVELS = [("L1", 1.1), ("L2", 3.1), ("L3", 12.0), ("内存", 114.5)]   # measured, as in pages.py


def segment(x, y, w, h, fill, stroke, label="", colour=INK, size=18):
    out = [rect(x, y, w, h, fill, stroke, rx=0, width=1.6)]
    if label:
        out.append(text(x + w / 2, y + h / 2 + size * 0.36, label, size, colour, "bold"))
    return out


def level_latency():
    # Beside a table of the same rows: the caption stands level with its header and each bar
    # with its row, so the pitch is the row height. The blank above the caption balances the
    # margin the theme keeps under a figure.
    W, H = 500, 268
    x0, unit, bar_h, step = 66, 3.1, 36, 51
    out = [text(x0, 49, "访问延迟（ns，按比例）", 18, MUTED, "bold", "start")]
    for k, (name, ns) in enumerate(LEVELS):
        y = 76.5 + k * step
        out.append(text(x0 - 12, y + bar_h / 2 + 7, name, 19, INK, "bold", "end"))
        fill, stroke = (FILL_GREEN, GREEN) if name == "内存" else (FILL_BLUE, BLUE)
        w = ns * unit
        out += segment(x0, y, w, bar_h, fill, stroke)
        out.append(text(x0 + w + 10, y + bar_h / 2 + 7, f"{ns:g}", 19, INK, "bold", "start"))
    return W, H, out


def metrics_time():
    # The blank above the caption balances the margin the theme keeps under a figure, so the
    # chart centres on the table beside it.
    W, H = 560, 224
    x0, unit, bar_h = 84, 118, 48
    hit, pen = 1.1, 2.0
    hit_w, pen_w = hit * unit, pen * unit
    out = [text(x0, 38, "L1 的一次访问（按比例）", 18, MUTED, "bold", "start")]
    y = 58
    out.append(text(x0 - 12, y + bar_h / 2 + 7, "命中", 19, INK, "bold", "end"))
    out += segment(x0, y, hit_w, bar_h, FILL_BLUE, BLUE, "命中时间")
    out.append(text(x0 + hit_w + 10, y + bar_h / 2 + 7, f"{hit} ns", 19, INK, "bold", "start"))
    y = 136
    out.append(text(x0 - 12, y + bar_h / 2 + 7, "不命中", 19, INK, "bold", "end"))
    out += segment(x0, y, hit_w, bar_h, FILL_BLUE, BLUE, "命中时间")
    out += segment(x0 + hit_w, y, pen_w, bar_h, FILL_RED, RED, "不命中惩罚", INK)
    out.append(text(x0 + hit_w + pen_w + 10, y + bar_h / 2 + 7, f"{hit + pen:g} ns", 19, INK, "bold",
                    "start"))
    out += brace(x0 + hit_w, x0 + hit_w + pen_w, y + bar_h + 6, INK, f"比命中多出 {pen} ns", 18)
    return W, H, out


if __name__ == "__main__":
    save("level-latency", *level_latency())
    save("metrics-time", *metrics_time())
    