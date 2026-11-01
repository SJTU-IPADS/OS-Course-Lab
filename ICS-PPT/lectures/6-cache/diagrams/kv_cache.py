#!/usr/bin/env python3
"""Decoding the same six tokens with a KV cache, one frame per step.

The grid has the geometry of kv_naive.py. Row s is step s: only the new token is computed
(orange); the k, v of the earlier tokens are read from the cache (green), not computed. The
cache on the right is allocated for six positions; the new k, v are appended at position s
(orange arrow), the positions the query reads are framed green. The counts on the right come
from the loop below. Run it to refresh ../assets/kv-cache-1.svg ... kv-cache-6.svg.
"""

from kv_kit import *
from kv_naive import (CAPTION_Y, CW, GX, H, HEAD_Y, LEGEND_Y, RH, RPITCH, RY, T, W, col_x,
                      header, row_y)

SX, SW = 668, 124                        # cache slots: left edge, width


def cache_panel(s):
    out = [text(SX + SW / 2, HEAD_Y + 22, "KV Cache", 17, INK, "bold")]
    for p in range(1, T + 1):
        y = row_y(p)
        kind = "new" if p == s else ("cache" if p < s else "empty")
        out += cell(SX, y, SW, RH, f"k{p} v{p}" if p <= s else "", kind, size=15)
        out.append(text(SX + SW + 10, y + RH / 2 + 5, f"位置 {p}", 14, MUTED, "normal", "start"))
    out.append(rect(SX - 5, row_y(1) - 5, SW + 10, (s - 1) * RPITCH + RH + 10, "none", GREEN,
                    rx=6, width=3))
    ex = col_x(s) + CW + 8
    out.append(arrow(ex, row_y(s) + RH / 2, SX - 10, row_y(s) + RH / 2, ORANGE, 2.5))
    out.append(text((ex + SX - 10) / 2, row_y(s) + RH / 2 - 8, "追加", 15, ORANGE, "bold"))
    return out


def stats(s):
    x0 = 900
    out = []
    for k, (label, value, color) in enumerate([("本步 k、v 投影", "1 次", ORANGE),
                                                ("累计投影", f"{s} 次", INK),
                                                ("本步读取缓存", f"{s} 个位置", GREEN)]):
        y = RY + 14 + k * 66
        out.append(text(x0, y, label, 16, MUTED, "normal", "start"))
        out.append(text(x0, y + 30, value, 24, color, "bold", "start"))
    return out


def frame(s):
    out = []
    header(s, out)
    for r in range(1, s + 1):
        y = row_y(r)
        out.append(text(GX - 14, y + RH / 2 + 5, f"第 {r} 步", 16, INK if r == s else MUTED,
                        "bold" if r == s else "normal", "end"))
        for i in range(1, r + 1):
            out += cell(col_x(i), y, CW, RH, f"k{i} v{i}", "new" if i == r else "reuse",
                        size=15)
    out += cache_panel(s)
    out += stats(s)
    cap = (f"第 {s} 步：只算 x{s}，k{s} v{s} 追加到位置 {s}，q{s} 读取位置 1 至 {s}" if s > 1
           else "第 1 步：只算 x1，k1 v1 追加到位置 1，q1 读取位置 1")
    out.append(text(W / 2, CAPTION_Y, cap, 20, INK, "bold"))
    out += legend(130, LEGEND_Y, [("new", "本步新算并追加"), ("reuse", "从缓存读出，不重算"),
                                  ("cache", "缓存中已有"), ("empty", "预分配，空")])
    return out


if __name__ == "__main__":
    for s in range(1, T + 1):
        save(f"kv-cache-{s}", W, H, frame(s))
