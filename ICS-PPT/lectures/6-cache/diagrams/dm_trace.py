#!/usr/bin/env python3
"""Reads of addresses 0, 1, 13, 8, 0 on the direct-mapped example cache (m = 4,
S = 4, E = 1, B = 2), one frame per read. The left table lists the reads so far
with the split of each address and its result; the right side is the cache after
the read, the touched line framed green (hit) or red (miss). The results come from
the simulation below. Run it to refresh ../assets/dm-trace-1.svg ... dm-trace-5.svg.
"""

from cachekit import *

W, H = 1000, 350
SEQ = [0, 1, 13, 8, 0]
GEO = Geo(vw=30, tw=46, bw=54, lh=40)
CX = 712
TX = 24
COLS = [("读", 30), ("地址", 90), ("二进制", 190), ("标记", 270), ("组索引", 350),
        ("块偏移", 440), ("结果", 540)]


NOTES = ["地址 0：组 0 的行无效，不命中，取出块 0（地址 0–1）放入组 0",
         "地址 1：组 0 的行有效，标记 0 相等，命中，块偏移 1 取出 m[1]",
         "地址 13：组 2 的行无效，不命中，取出块 6（地址 12–13）放入组 2",
         "地址 8：组 0 的行有效，标记不等，不命中，块 4 替换块 0",
         "地址 0：块 0 已被替换，不命中，组 1 与组 3 仍空着，这是冲突不命中"]


def simulate(seq, m=4, s=2, b=1):
    """Direct-mapped, S = 2**s sets: returns [(addr, set, tag, off, hit, state)]."""
    state, out = {}, []
    for a in seq:
        blk = a >> b
        st, tag = blk % (1 << s), blk >> s
        hit = state.get(st) == tag
        state[st] = tag
        out.append((a, st, tag, a & ((1 << b) - 1), hit, dict(state)))
    return out


def frame(n):
    out = []
    res = simulate(SEQ)
    # table
    for name, x in COLS:
        out.append(text(TX + x, 28, name, 16, MUTED, "bold"))
    out.append(line(TX + 4, 40, TX + 590, 40, LINE, 1.4))
    for i in range(n):
        a, st, tag, off, hit, state = res[i]
        y = 52 + i * 46
        if i == n - 1:
            out.append(rect(TX + 4, y - 4, 586, 42, FILL_GREY, LINE, rx=4, width=1))
        bits = format(a, "04b")
        out.append(text(TX + 30, y + 24, str(i + 1), 18, INK, "bold"))
        out.append(text(TX + 90, y + 24, str(a), 19, INK, "bold", font=MONO))
        for j, ch in enumerate(bits):
            col = TAG if j < 1 else IDX if j < 3 else OFF
            out.append(text(TX + 190 - 1.5 * 13.2 + j * 13.2, y + 24, ch, 19, col, "bold",
                            font=MONO))
        out.append(text(TX + 270, y + 24, str(tag), 19, TAG, "bold", font=MONO))
        out.append(text(TX + 350, y + 24, f"{st:02b}", 19, IDX, "bold", font=MONO))
        out.append(text(TX + 440, y + 24, str(off), 19, OFF, "bold", font=MONO))
        out += badge(TX + 540, y + 17, "命中" if hit else "不命中", "hit" if hit else "miss",
                     size=16)
    # cache after read n
    a, st, tag, off, hit, state = res[n - 1]
    sets = []
    for s in range(4):
        if s in state:
            blk = (state[s] << 2) | s
            kw = dict(v=1, tag=str(state[s]), data=[f"m[{2 * blk}]", f"m[{2 * blk + 1}]"])
            if s == st:
                kw.update(halo=GREEN if hit else RED, req=off)
        else:
            kw = dict(v=0)
        sets.append([kw])
    shapes, pos, bottom = cache_sets(CX, 74, GEO, 2, sets, set_gap=14, headers=False,
                                     group=False, label_dx=14)
    out += shapes
    out.append(text(CX + GEO.width(2) / 2 - 30, 44, f"读 {n} 之后的缓存", 17, INK, "bold"))
    out.append(text(W / 2, 322, NOTES[n - 1], 19, INK, "bold"))
    return out


if __name__ == "__main__":
    for n in range(1, 6):
        save(f"dm-trace-{n}", W, H, frame(n))
