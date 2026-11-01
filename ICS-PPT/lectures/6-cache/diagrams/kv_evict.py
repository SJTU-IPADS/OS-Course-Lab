#!/usr/bin/env python3
"""Which tokens keep their k, v: all of them, the most recent W, or the first few (attention
sinks) plus the most recent W. Sixteen tokens are cached and the seventeenth is being
generated. Run it to refresh ../assets/kv-evict.svg."""

from kv_kit import *

W, H = 1000, 340
N, CW, PITCH, X0 = 16, 48, 54, 56
WINDOW, SINK = 6, 4


def strip(top, keep, title):
    out = [text(X0, top, title, 18, INK, "bold", "start")]
    for i in range(1, N + 2):
        x = X0 + (i - 1) * PITCH
        if i == N + 1:
            out += cell(x, top + 14, CW, 34, "新", "new", size=16)
        else:
            out += cell(x, top + 14, CW, 34, str(i), "cache" if keep(i) else "gone", size=15,
                        weight="normal")
    return out


def figure():
    out = []
    out += strip(26, lambda i: True, "保留全部 token：缓存随 t 增长，现为 16 个位置")
    out += strip(116, lambda i: i > N - WINDOW,
                 f"滑动窗口 W = {WINDOW}：只保留最近 {WINDOW} 个，缓存固定为 {WINDOW} 个位置")
    out += strip(206, lambda i: i <= SINK or i > N - WINDOW,
                 f"汇聚点 {SINK} 个 + 窗口 W = {WINDOW}：缓存固定为 {SINK + WINDOW} 个位置")
    x = X0 + (N - WINDOW) * PITCH
    out += brace(x, x + WINDOW * PITCH - 6, 268, BLUE, f"窗口：最近 {WINDOW} 个", size=15)
    out += brace(X0, X0 + SINK * PITCH - 6, 268, INK, f"汇聚点：最前 {SINK} 个", size=15)
    out += legend(430, 326, [("cache", "保留 k、v"), ("gone", "淘汰")])
    return out


if __name__ == "__main__":
    save("kv-evict", W, H, figure())
