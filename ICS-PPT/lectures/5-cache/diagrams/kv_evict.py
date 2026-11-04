#!/usr/bin/env python3
"""Two policies for choosing which KV cache to invalidate when GPU memory is full:
(1) LRU: invalidate the session that has not been accessed for the longest time;
(2) Smallest KV cache first: invalidate the session with the fewest cached layers/tokens,
minimising extra recomputation and preserving cache-hit billing discounts for large caches.
Run it to refresh ../assets/kv-evict.svg.
"""

from kv_kit import *

W, H = 960, 296
CAPTION_Y, LEGEND_Y = 244, 278


def badge(x, y, w, h, label, kind):
    fill, stroke = KINDS[kind]
    tone = RED if kind == "waste" else BLUE
    return [rect(x, y, w, h, fill, stroke, rx=4, width=1.6),
            text(x + w / 2, y + h / 2 + 5, label, 14, tone, "bold")]


def figure():
    out = []
    # Left panel: LRU
    lx, lw = 32, 434
    out.append(rect(lx, 14, lw, 194, WHITE, INK, rx=8, width=1.8))
    out.append(text(lx + lw / 2, 42, "策略一：最近最少使用（LRU）", 18, INK, "bold"))
    out.append(text(lx + lw / 2, 64, "优先使最久未访问的会话缓存失效", 14, MUTED))
    lru_rows = [
        ("会话 A", "最后访问：45 分钟前", "使失效", "waste"),
        ("会话 B", "最后访问：6 分钟前", "保留", "cache"),
        ("会话 C", "最后访问：1 分钟前", "保留", "cache"),
    ]
    for i, (name, info, act, kind) in enumerate(lru_rows):
        y = 78 + i * 40
        fill, stroke = KINDS[kind]
        out.append(rect(lx + 18, y, 304, 32, fill, stroke, rx=4, width=1.5))
        out.append(text(lx + 32, y + 21, name, 15, INK, "bold", "start"))
        out.append(text(lx + 308, y + 21, info, 14, MUTED, "normal", "end"))
        out += badge(lx + 332, y, 84, 32, act, kind)

    # Right panel: Smallest KV cache (fewest cached layers/tokens) first
    rx, rw = 494, 434
    out.append(rect(rx, 14, rw, 194, WHITE, INK, rx=8, width=1.8))
    out.append(text(rx + rw / 2, 42, "策略二：最小缓存优先", 18, INK, "bold"))
    out.append(text(rx + rw / 2, 64, "优先使已缓存层数或 token 最少的会话失效", 14, MUTED))
    size_rows = [
        ("会话 A", 236, "已缓存多（省费用、重算慢）", "", "保留", "cache"),
        ("会话 B", 152, "已缓存中等", "", "保留", "cache"),
        ("会话 C", 76, "缓存少", "重算代价小", "使失效", "waste"),
    ]
    for i, (name, bw, inside, outside, act, kind) in enumerate(size_rows):
        y = 78 + i * 40
        fill, stroke = KINDS[kind]
        out.append(text(rx + 18, y + 21, name, 15, INK, "bold", "start"))
        out.append(rect(rx + 82, y, bw, 32, fill, stroke, rx=4, width=1.5))
        out.append(text(rx + 82 + bw / 2, y + 21, inside, 13, INK, "bold"))
        if outside:
            out.append(text(rx + 90 + bw, y + 21, outside, 13, RED, "bold", "start"))
        out += badge(rx + 332, y, 84, 32, act, kind)

    out.append(text(W / 2, CAPTION_Y,
                    "LRU 淘汰最久未访问的缓存，最小缓存优先淘汰重算代价最小、对用户体验影响最小的缓存",
                    18, INK, "bold"))
    items = [("cache", "保留在显存的 KV Cache"), ("waste", "选中使失效的 KV Cache")]
    out += legend((W - legend_width(items)) / 2, LEGEND_Y, items)
    return out


if __name__ == "__main__":
    save("kv-evict", W, H, figure())
