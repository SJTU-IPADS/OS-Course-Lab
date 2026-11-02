#!/usr/bin/env python3
"""A 4 x 4 int matrix W stored by rows, read one element at a time, one frame per read.

Eight frames for the row-by-row loop (addresses 0, 4, ..., 28: stride 1); stride_k.py
(which imports this module) draws the column-by-column loop (addresses 0, 16, 32,
48, 4, 20, 36, 52: stride 4). Top left: the matrix as the program sees it, the
element being read in orange, the elements read so far carrying their read number
(green = hit, red = miss). Top right: a cache of two blocks, the block just used
framed green (hit) or red (miss). Under the figure a miss names the policy that acted:
placement (a free slot is used) or replacement (the least recently used block leaves). Bottom: the same 16 ints in memory, one row after
the other, four ints to a block. The results come from the simulation below (LRU
replacement), not from a table. Run it to refresh ../assets/stride-1-1.svg ... -8.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, FILL_RED, GREEN, INK, MUTED,
                    ORANGE, RED, WHITE, brace, circle, elbow, mono, rect, save, text)

W, H = 1000, 392
M = N = 4                                # the matrix
PER_BLOCK = 4                            # ints in one block: one row of W
SLOTS = 2                                # the cache holds two blocks
MX, MY, MW, MH = 78, 58, 58, 34          # matrix origin and cell size
CX, CW, CH, CY = 440, 42, 38, 86         # cache: first slot, cell width and height, top
CGAP = 70                                # gap between the two slots
SX, SY, SW, SH = 56, 212, 58, 42         # memory strip origin and cell size
CAPTION_Y = 338
POLICY_Y = 370


def sequence(by_rows):
    """The first eight reads as (i, j): row by row, or column by column."""
    cells = ([(i, j) for i in range(M) for j in range(N)] if by_rows
             else [(i, j) for j in range(N) for i in range(M)])
    return cells[:8]


def simulate(seq):
    """LRU cache of SLOTS blocks. Returns one dict per read: block, hit, slot, evicted."""
    held = [None] * SLOTS
    used = [-1] * SLOTS
    out = []
    for t, (i, j) in enumerate(seq):
        block = (i * N + j) // PER_BLOCK
        hit = block in held
        evicted = None
        if hit:
            slot = held.index(block)
        else:
            empty = [s for s in range(SLOTS) if held[s] is None]
            slot = empty[0] if empty else used.index(min(used))
            evicted = held[slot]
            held[slot] = block
        used[slot] = t
        out.append(dict(i=i, j=j, block=block, hit=hit, slot=slot, evicted=evicted,
                        held=list(held)))
    return out


def badge(cx, cy, number, hit):
    color, fill = (GREEN, FILL_GREEN) if hit else (RED, FILL_RED)
    return [circle(cx, cy, 8.5, fill, color, 1.8),
            text(cx, cy + 4.5, str(number), 12.5, INK, "bold")]


def slot_x(slot):
    return CX + slot * (PER_BLOCK * CW + CGAP)


def frame(by_rows, n):
    seq = sequence(by_rows)
    res = simulate(seq)
    cur = res[n - 1]
    done = {(r["i"], r["j"]): (t + 1, r["hit"]) for t, r in enumerate(res[:n])}
    out = []
    # the matrix
    out.append(text(MX + N * MW / 2, 24, f"二维数组 W[{M}][{N}]", 17, INK, "bold"))
    for j in range(N):
        out.append(text(MX + j * MW + MW / 2, MY - 8, f"j = {j}", 15, MUTED))
    for i in range(M):
        out.append(text(MX - 10, MY + i * MH + MH / 2 + 5, f"i = {i}", 15, MUTED, anchor="end"))
        out.append(text(MX + N * MW + 12, MY + i * MH + MH / 2 + 5, f"块 {i}", 15, MUTED,
                        anchor="start"))
        for j in range(N):
            now = (i, j) == (cur["i"], cur["j"])
            x, y = MX + j * MW, MY + i * MH
            out.append(rect(x, y, MW, MH, FILL_ORANGE if now else FILL_BLUE,
                            ORANGE if now else BLUE, rx=3, width=2.6 if now else 1.4))
            out.append(mono(x + MW / 2, y + MH - 7, f"[{i}][{j}]", 14, INK, "bold" if now
                            else "normal", anchor="middle"))
            if (i, j) in done:
                number, hit = done[(i, j)]
                out += badge(x + MW - 10, y + 9, number, hit)
    # the cache
    out.append(text(CX + (2 * PER_BLOCK * CW + CGAP) / 2, 24, f"缓存：{SLOTS} 个块", 17, INK,
                    "bold"))
    for slot in range(SLOTS):
        x = slot_x(slot)
        block = cur["held"][slot]
        used = slot == cur["slot"]
        if block is None:
            out.append(text(x + PER_BLOCK * CW / 2, CY - 10, "空", 15, MUTED, "bold"))
        else:
            out.append(text(x + PER_BLOCK * CW / 2, CY - 10, f"块 {block}", 16, INK, "bold"))
        for k in range(PER_BLOCK):
            hot = used and k == (cur["i"] * N + cur["j"]) % PER_BLOCK
            fill = FILL_ORANGE if hot else (FILL_BLUE if block is not None else WHITE)
            out.append(rect(x + k * CW, CY, CW, CH, fill, ORANGE if hot else INK, rx=0,
                            width=2 if hot else 1.4))
        out.append(rect(x, CY, PER_BLOCK * CW, CH, "none", INK, rx=0, width=2))
        if used:
            out.append(rect(x - 5, CY - 5, PER_BLOCK * CW + 10, CH + 10, "none",
                            GREEN if cur["hit"] else RED, rx=4, width=3))
    misses = sum(1 for r in res[:n] if not r["hit"])
    out.append(text(CX + (2 * PER_BLOCK * CW + CGAP) / 2, 50, f"已读 {n} 次，其中 miss {misses} 次",
                    18, INK, "bold"))
    # memory
    out.append(text(SX - 8, SY + SH / 2 + 5, "内存", 15, MUTED, "bold", "end"))
    for e in range(M * N):
        i, j = divmod(e, N)
        now = (i, j) == (cur["i"], cur["j"])
        x = SX + e * SW
        out.append(rect(x, SY, SW, SH, FILL_ORANGE if now else FILL_GREEN,
                        ORANGE if now else GREEN, rx=0, width=2.6 if now else 1.2))
        out.append(mono(x + SW / 2, SY + SH - 8, f"[{i}][{j}]", 14, INK,
                        "bold" if now else "normal", anchor="middle"))
        out.append(mono(x + SW / 2, SY + SH + 18, str(4 * e), 15, MUTED, anchor="middle"))
        if (i, j) in done:
            number, hit = done[(i, j)]
            out += badge(x + SW - 10, SY + 9, number, hit)
    out.append(text(SX - 8, SY + SH + 18, "地址", 14, MUTED, "normal", "end"))
    for b in range(M * N // PER_BLOCK):
        x0 = SX + b * PER_BLOCK * SW
        edge = ORANGE if b == cur["block"] else INK
        out.append(rect(x0, SY, PER_BLOCK * SW, SH, "none", edge, rx=0, width=3 if b == cur["block"]
                        else 2))
        out += brace(x0 + 4, x0 + PER_BLOCK * SW - 4, SY + SH + 26, ORANGE if b == cur["block"]
                     else BLUE, f"块 {b}", 16)
    # a miss moves the block from memory to the cache
    if not cur["hit"]:
        bx = SX + (cur["block"] * PER_BLOCK + PER_BLOCK / 2) * SW
        sx = slot_x(cur["slot"]) + PER_BLOCK * CW / 2
        mid = (MY + M * MH + SY) / 2
        out.append(elbow([(bx, SY - 3), (bx, mid), (sx, mid), (sx, CY + CH + 8)], ORANGE, 3))
    # the caption
    i, j = cur["i"], cur["j"]
    head = f"读 {n}：W[{i}][{j}]，地址 {4 * (i * N + j)}，在块 {cur['block']} 中，"
    if cur["hit"]:
        cap, color = head + "块已在缓存中：hit", GREEN
    elif cur["evicted"] is None:
        cap, color = head + f"块不在缓存中：miss，取入缓存", RED
    else:
        cap, color = head + f"块不在缓存中：miss，取入缓存并替换块 {cur['evicted']}", RED
    out.append(text(W / 2, CAPTION_Y, cap, 20, color, "bold"))
    if not cur["hit"]:                   # a miss is where the two policies act
        if cur["evicted"] is None:
            rule = "放置策略：缓存有空位，块放入空位"
        else:
            rule = (f"替换策略：缓存已满，逐出最久没有被使用的块 {cur['evicted']}，"
                    f"块 {cur['block']} 放入它的位置")
        out.append(text(W / 2, POLICY_Y, rule, 19, BLUE, "bold"))
    return out


if __name__ == "__main__":
    for n in range(1, 9):
        save(f"stride-1-{n}", W, H, frame(True, n))
