#!/usr/bin/env python3
"""Reads of addresses 0, 8, 1, 9, 0, 8 on the 2-way example cache (m = 4, S = 2, E = 2,
B = 2, LRU), one frame per read. The left table lists the reads
so far with the split of each address and its result; the right side is the cache after
the read, the touched line framed green (hit) or red (miss). Hits, misses and the cache
states come from the simulation below. Run it to refresh ../assets/sa-trace-1.svg ...
sa-trace-6.svg.
"""

from cachekit import *

W, H = 1000, 298
SEQ = [0, 8, 1, 9, 0, 8]
GEO = Geo(vw=30, tw=46, bw=54, lh=40)
CX = 756
TX = 14
PITCH = 36
# column centres of the table
COL = {"n": 14, "addr": 66, "bits": 142, "tag": 222, "idx": 298, "off": 372, "sa": 468}


def simulate(seq, sets, ways, b=1):
    """LRU set-associative cache of `sets` x `ways`; returns one dict per read.

    Each dict holds the split of the address, hit, the line touched, the tag the line
    held before a miss (None when it was empty) and the state after the read as
    {set: [None | tag per line]}.
    """
    lines = [[None] * ways for _ in range(sets)]      # tag per line
    used = [[0] * ways for _ in range(sets)]          # time of the last use
    out = []
    for now, a in enumerate(seq, 1):
        blk = a >> b
        st, tag = blk % sets, blk // sets
        hit_at = next((k for k, t in enumerate(lines[st]) if t == tag), None)
        old = None
        if hit_at is None:
            free = [k for k, t in enumerate(lines[st]) if t is None]
            k = free[0] if free else min(range(ways), key=lambda j: used[st][j])
            old = lines[st][k]
            lines[st][k] = tag
        else:
            k = hit_at
        used[st][k] = now
        out.append(dict(addr=a, set=st, tag=tag, off=a & ((1 << b) - 1), hit=hit_at is not None,
                        line=k, old=old, state=[list(row) for row in lines]))
    return out


SA = simulate(SEQ, 2, 2)


def note(n):
    r = SA[n - 1]
    a, tag, st = r["addr"], f"{r['tag']:02b}", r["set"]
    blk = a >> 1
    if r["hit"]:
        return (f"地址 {a}：组 {st} 的第 {r['line'] + 1} 行有效，标记 {tag} 相等，命中，"
                f"块偏移 {r['off']} 取出 m[{a}]")
    before = [t for t in SA[n - 2]["state"][st]] if n > 1 else [None, None]
    free = sum(t is None for t in before)
    lo, hi = 2 * blk, 2 * blk + 1
    if free == 2:
        why = f"组 {st} 的两行都无效"
    else:
        why = f"组 {st} 的第 1 行标记 {before[0]:02b} ≠ {tag}，第 2 行无效"
    return f"地址 {a}：{why}，不命中，取出块 {blk}（地址 {lo}–{hi}）放入第 {r['line'] + 1} 行"


def frame(n):
    out = []
    shown = n
    # table header
    heads = [("读", "n"), ("地址", "addr"), ("二进制", "bits"), ("标记", "tag"),
             ("组索引", "idx"), ("块偏移", "off"), ("2 路组相联", "sa")]
    for name, key in heads:
        out.append(text(TX + COL[key], 26, name, 16, MUTED, "bold"))
    right = TX + COL["sa"] + 62
    out.append(line(TX, 38, right, 38, LINE, 1.4))
    for i in range(shown):
        r = SA[i]
        y = 48 + i * PITCH
        if i == n - 1:
            out.append(rect(TX, y - 2, right - TX, PITCH - 4, FILL_GREY, LINE, rx=4, width=1))
        base = y + 21
        out.append(text(TX + COL["n"], base, str(i + 1), 18, INK, "bold"))
        out.append(text(TX + COL["addr"], base, str(r["addr"]), 19, INK, "bold", font=MONO))
        bits = format(r["addr"], "04b")
        for j, ch in enumerate(bits):
            col = TAG if j < 2 else IDX if j < 3 else OFF
            out.append(text(TX + COL["bits"] - 1.5 * 13.2 + j * 13.2, base, ch, 19, col, "bold",
                            font=MONO))
        out.append(text(TX + COL["tag"], base, f"{r['tag']:02b}", 19, TAG, "bold", font=MONO))
        out.append(text(TX + COL["idx"], base, str(r["set"]), 19, IDX, "bold", font=MONO))
        out.append(text(TX + COL["off"], base, str(r["off"]), 19, OFF, "bold", font=MONO))
        out += badge(TX + COL["sa"], base - 7, "命中" if r["hit"] else "不命中",
                     "hit" if r["hit"] else "miss", size=16)
    # the 2-way cache after read `shown`
    r = SA[shown - 1]
    sets = []
    for st in range(2):
        group = []
        for k, tag in enumerate(r["state"][st]):
            if tag is None:
                kw = dict(v=0)
            else:
                blk = tag * 2 + st
                kw = dict(v=1, tag=f"{tag:02b}", data=[f"m[{2 * blk}]", f"m[{2 * blk + 1}]"])
            if st == r["set"] and k == r["line"]:
                kw.update(halo=GREEN if r["hit"] else RED, req=r["off"])
            group.append(kw)
        sets.append(group)
    shapes, pos, bottom = cache_sets(CX, 66, GEO, 2, sets, set_gap=26, headers=False,
                                     group=True, label_dx=14)
    out += shapes
    out.append(text(CX + GEO.width(2) / 2 - 24, 34,
                    f"读 {shown} 之后的缓存", 17, INK,
                    "bold"))
    out.append(text(W / 2, 290, note(n), 19, INK, "bold"))
    return out


if __name__ == "__main__":
    for n in range(1, 7):
        save(f"sa-trace-{n}", W, H, frame(n))
