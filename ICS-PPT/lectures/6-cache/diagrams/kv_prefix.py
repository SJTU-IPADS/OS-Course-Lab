#!/usr/bin/env python3
"""Prefix caching: two requests start with the same 48-token system prompt (three blocks of
16 tokens). When request 2 arrives, its first three blocks are found in the cache and shared
(green); only its own block is computed (orange). Block tables above and below the pool.
Run it to refresh ../assets/kv-prefix.svg."""

from kv_kit import *

W, H = 1000, 322
BLOCK = 16
SYSTEM, USER1, USER2 = 48, 20, 11        # tokens of the shared prompt and the two inputs
BX, BW, BP = 50, 136, 150                # pool blocks: left edge, width, pitch
POOL_Y = 126


def blocks(n):
    return -(-n // BLOCK)


def figure():
    shared = SYSTEM // BLOCK
    r1 = list(range(shared + blocks(USER1)))               # P0 .. P4
    r2 = list(range(shared)) + [len(r1)]                   # P0 .. P2, P5
    out = []
    # block table of request 1 above the pool, of request 2 below it
    out.append(text(BX, 24, "请求 1 的块表", 17, INK, "bold", "start"))
    for logical, block in enumerate(r1):
        x = BX + block * BP
        out += cell(x + 10, 36, BW - 20, 30, f"{logical} → P{block}", "cache", size=14)
        out.append(line(x + BW / 2, 66, x + BW / 2, POOL_Y - 2, MUTED, 1.6))
    out.append(text(BX, 252, "请求 2 的块表", 17, INK, "bold", "start"))
    for logical, block in enumerate(r2):
        x = BX + block * BP
        kind = "reuse" if block < shared else "new"
        out += cell(x + 10, 262, BW - 20, 30, f"{logical} → P{block}", kind, size=14)
        out.append(line(x + BW / 2, 262, x + BW / 2, POOL_Y + 74, MUTED, 1.6))
    # the pool
    names = ["系统提示"] * shared + ["请求 1 的输入"] * blocks(USER1) + ["请求 2 的输入"]
    for block, name in enumerate(names):
        x = BX + block * BP
        kind = "reuse" if block < shared else ("new" if block == len(r1) else "cache")
        out += cell(x, POOL_Y, BW, 72, "", kind)
        out.append(text(x + BW / 2, POOL_Y + 28, f"P{block}", 17, INK, "bold"))
        out.append(text(x + BW / 2, POOL_Y + 56, name, 15, MUTED))
    out += legend(176, 316, [("reuse", "命中：已算好的块，两个请求共用"), ("new", "未命中：本请求新算"),
                             ("cache", "请求 1 的块")])
    return out


if __name__ == "__main__":
    save("kv-prefix", W, H, figure())
