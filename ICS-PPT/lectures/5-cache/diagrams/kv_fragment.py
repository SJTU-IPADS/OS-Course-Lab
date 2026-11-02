#!/usr/bin/env python3
"""Preallocating the KV cache of every request for the maximum length: four requests with
their current lengths, the used part blue and the reserved but unused part red. The
numbers are computed below. Run it to refresh ../assets/kv-fragment.svg."""

from kv_kit import *

W, H = 1000, 372
MAX_LEN = 4096
LENGTHS = {"A": 37, "B": 612, "C": 1500, "D": 90}
BX, BW = 100, 860                        # the bar of one request: left edge, width for MAX_LEN


def stats():
    used = sum(LENGTHS.values())
    total = MAX_LEN * len(LENGTHS)
    return used, total, 1 - used / total


def figure():
    out = []
    for k, (name, length) in enumerate(LENGTHS.items()):
        y = 20 + k * 74
        used_w = BW * length / MAX_LEN
        out.append(text(BX - 14, y + 22, f"请求 {name}", 18, INK, "bold", "end"))
        out.append(rect(BX, y, BW, 32, "#fbe7e5", RED, rx=4, width=1.4))
        out.append(rect(BX, y, max(used_w, 3), 32, "#eaf3f7", BLUE, rx=3, width=1.6))
        out.append(text(BX, y + 54, f"已有 {length} 个 token，预留 {MAX_LEN} 个位置，未使用 "
                        f"{MAX_LEN - length} 个", 16, MUTED, "normal", "start"))
    used, total, waste = stats()
    out.append(text(W / 2, 328, f"合计预留 {total} 个位置，已用 {used} 个，未用的占 {waste * 100:.1f}%",
                    20, RED, "bold"))
    out += legend(340, 358, [("cache", "已存放 k、v"), ("waste", "已预留，未使用")])
    return out


if __name__ == "__main__":
    save("kv-fragment", W, H, figure())
