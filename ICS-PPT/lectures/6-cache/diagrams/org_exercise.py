#!/usr/bin/env python3
"""The two exercise sequences of the cache-organisation exercise, one frame per read.

Reads 0, 16, 1, 17, 32, 0, 4, 20, 5, 21 (m = 6, B = 4 bytes, so b = 2) on a direct-mapped
cache (S = 4, E = 1, t = 2, s = 2) and on a 2-way set-associative cache with LRU
replacement (S = 2, E = 2, t = 3, s = 1), both empty at the start. The left table lists the
reads so far with the split of each address and its result; the right side is the cache
after the read, the touched line framed green (hit) or red (miss). Hits, misses and the cache
states come from the simulation in sa_trace.py. Run it to refresh
../assets/exercise-dm-1.svg ... exercise-dm-10.svg and exercise-sa-1.svg ... exercise-sa-10.svg.
"""

from cachekit import *
from sa_trace import simulate

W, H = 1000, 430
SEQ = [0, 16, 1, 17, 32, 0, 4, 20, 5, 21]
M, B = 6, 4
GEO = Geo(vw=30, tw=46, bw=48, lh=40)
CX = 650
TX = 14
PITCH = 32
COL = {"n": 14, "addr": 62, "bits": 140, "tag": 222, "idx": 292, "off": 360, "res": 462}
SUMMARY = {"dm": "10 次读取全部不命中：块 0、4、8 轮流占用组 0，块 1、5 轮流占用组 1，组 2 与组 3 始终为空",
           "sa": "10 次读取中 {misses} 次不命中，少于直接映射缓存的 10 次"}
SPECS = {"dm": dict(sets=4, ways=1, name="exercise-dm"),
         "sa": dict(sets=2, ways=2, name="exercise-sa")}


def split_bits(sets):
    b = B.bit_length() - 1
    s = sets.bit_length() - 1
    return M - s - b, s, b


def note(kind, sim, n):
    spec = SPECS[kind]
    sets, ways = spec["sets"], spec["ways"]
    t, s, b = split_bits(sets)
    r = sim[n - 1]
    a, st = r["addr"], r["set"]
    tag = format(r["tag"], f"0{t}b")
    blk = a >> b
    row = r["line"] + 1
    if r["hit"]:
        return f"地址 {a}：组 {st} 的第 {row} 行有效，标记 {tag} 相等，命中" if ways > 1 else \
            f"地址 {a}：组 {st} 的行有效，标记 {tag} 相等，命中"
    if r["old"] is None:
        return (f"地址 {a}：组 {st} 有无效的行，不命中，块 {blk} 放入第 {row} 行" if ways > 1 else
                f"地址 {a}：组 {st} 的行无效，不命中，取入块 {blk}")
    old_blk = r["old"] * sets + st
    if ways > 1:
        return (f"地址 {a}：组 {st} 的两行标记都不是 {tag}，不命中，"
                f"块 {blk} 替换最久未用的块 {old_blk}")
    return f"地址 {a}：组 {st} 的标记 {format(r['old'], f'0{t}b')} ≠ {tag}，不命中，块 {blk} 替换块 {old_blk}"


def frame(kind, n):
    spec = SPECS[kind]
    sets, ways = spec["sets"], spec["ways"]
    sim = simulate(SEQ, sets, ways, b=2)
    t, s, b = split_bits(sets)
    out = []
    heads = [("读", "n"), ("地址", "addr"), ("二进制", "bits"), ("标记", "tag"),
             ("组索引", "idx"), ("块偏移", "off"), ("结果", "res")]
    for name, key in heads:
        out.append(text(TX + COL[key], 26, name, 16, MUTED, "bold"))
    right = TX + COL["res"] + 62
    out.append(line(TX, 38, right, 38, LINE, 1.4))
    for i in range(n):
        r = sim[i]
        y = 48 + i * PITCH
        if i == n - 1:
            out.append(rect(TX, y - 2, right - TX, PITCH - 4, FILL_GREY, LINE, rx=4, width=1))
        base = y + 21
        out.append(text(TX + COL["n"], base, str(i + 1), 18, INK, "bold"))
        out.append(text(TX + COL["addr"], base, str(r["addr"]), 19, INK, "bold", font=MONO))
        for j, ch in enumerate(format(r["addr"], f"0{M}b")):
            col = TAG if j < t else IDX if j < t + s else OFF
            out.append(text(TX + COL["bits"] + (j - (M - 1) / 2) * 13.2, base, ch, 19, col, "bold",
                            font=MONO))
        out.append(text(TX + COL["tag"], base, format(r["tag"], f"0{t}b"), 19, TAG, "bold",
                        font=MONO))
        out.append(text(TX + COL["idx"], base, str(r["set"]), 19, IDX, "bold", font=MONO))
        out.append(text(TX + COL["off"], base, format(r["off"], f"0{b}b"), 19, OFF, "bold",
                        font=MONO))
        out += badge(TX + COL["res"], base - 7, "命中" if r["hit"] else "不命中",
                     "hit" if r["hit"] else "miss", size=16)
    r = sim[n - 1]
    cache = []
    for st in range(sets):
        group = []
        for k, tg in enumerate(r["state"][st]):
            if tg is None:
                kw = dict(v=0)
            else:
                blk = tg * sets + st
                kw = dict(v=1, tag=format(tg, f"0{t}b"), data=[f"m[{B * blk + i}]" for i in range(B)])
            if st == r["set"] and k == r["line"]:
                kw.update(halo=GREEN if r["hit"] else RED, req=r["off"])
            group.append(kw)
        cache.append(group)
    height = sets * ways * GEO.lh + (sets - 1) * 26
    top = 48 + (10 * PITCH - height) / 2
    shapes, pos, bottom = cache_sets(CX, top, GEO, B, cache, set_gap=26, headers=False,
                                     group=ways > 1, label_dx=14)
    out += shapes
    out.append(text(CX + GEO.width(B) / 2 - 24, top - 28, f"读 {n} 之后的缓存", 17, INK, "bold"))
    out.append(text(W / 2, H - 36, note(kind, sim, n), 19, INK, "bold"))
    if n == len(SEQ):
        misses = sum(not r["hit"] for r in sim)
        out.append(text(W / 2, H - 8, SUMMARY[kind].format(misses=misses), 20, RED, "bold"))
    return out


if __name__ == "__main__":
    for kind, spec in SPECS.items():
        for n in range(1, len(SEQ) + 1):
            save(f"{spec['name']}-{n}", W, H, frame(kind, n))
