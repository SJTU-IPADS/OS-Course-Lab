#!/usr/bin/env python3
"""Placing blocks in a 2-way set-associative cache: S = 2 sets of E = 2 lines, B = 2 bytes,
C = 8 bytes, the same capacity as the direct-mapped example, in front of a 16-byte main
memory (m = 4) of 8 blocks. Main memory is drawn as in direct_mapped.py: an address space,
a block is two adjacent squares with its 4-bit address in the upper left corner, "块 k" to
its left and its tag to its right. Block k goes to set (k mod 2) and may take either line
of it. One static figure: the state after blocks 0, 2 (the same set, the other line) and 1
(the other set) are loaded. Blocks 0 and 2 are framed orange, the set they share is framed
blue. The placement comes from the simulation below. Run it to refresh
../assets/sa-place.svg.
"""

from cachekit import *

W, H = 700, 480
Y0, PITCH, BH = 76, 44, 38
LABEL_X, MEM_X, SQ = 100, 112, 38        # right edge of "块 k", left edge of a block, byte square
GEO = Geo(vw=30, tw=46, bw=44, lh=40)
CACHE_X = 480
SEQ = [0, 2, 1]
SETS = 2
CAPTION = "块 0 与块 2 都映射到组 0，各占一行，同时在缓存中"
SAME = (0, 2)                       # the blocks of the example, in the same set


def simulate(seq, sets=SETS, ways=2):
    """Blocks loaded into an empty cache, first free line of the set; one dict per block."""
    lines = [[None] * ways for _ in range(sets)]
    out = []
    for blk in seq:
        st = blk % sets
        k = lines[st].index(None)
        lines[st][k] = blk
        out.append(dict(blk=blk, set=st, line=k, lines=[list(row) for row in lines]))
    return out


RES = simulate(SEQ)


def frame():
    out = []
    r = RES[-1]
    out.append(text(MEM_X + SQ, 36, "内存：16 字节，8 个块", 18, GREEN, "bold"))
    out.append(text(CACHE_X + GEO.width(2) / 2, 36, "缓存：S = 2，E = 2，B = 2", 18, INK, "bold"))
    out.append(text(CACHE_X + GEO.width(2) / 2, 58, "2 组，每组 2 行，每个块 2 字节", 16, MUTED))
    set_top = [Y0 + 50, Y0 + 50 + 2 * GEO.lh + 92]
    centers = [t + GEO.lh for t in set_top]
    start = MEM_X + 2 * SQ + 78              # the connectors leave to the right of the tag
    # connectors first, so the boxes lie on top
    for k in range(8):
        s = k % SETS
        ym = Y0 + k * PITCH + BH / 2
        on = k in SAME
        out.append(line(start, ym, CACHE_X - 70, centers[s],
                        SET_STROKES[s * 2] if on else LINE, 3 if on else 1.8))
    for k in range(8):
        s = k % SETS
        y = Y0 + k * PITCH
        out.append(text(LABEL_X, y + BH / 2 + 6, f"块 {k}", 17, INK, "bold", "end"))
        for byte in range(2):
            out.append(rect(MEM_X + byte * SQ, y, SQ, BH, SET_FILLS[s * 2], GREEN, rx=0,
                            width=1.4))
        out.append(text(MEM_X + 3, y + 14, format(2 * k, "04b"), 12, MUTED, anchor="start",
                        font=MONO))
        out.append(text(MEM_X + 2 * SQ + 12, y + BH / 2 + 5, f"tag = {k // 2}", 16, TAG,
                        "bold", "start"))
        if k in SAME:
            out.append(rect(MEM_X - 4, y - 4, 2 * SQ + 8, BH + 8, "none", ORANGE, rx=4,
                            width=3.2))
    for s in range(SETS):
        top = set_top[s]
        for e in range(2):
            blk = r["lines"][s][e]
            if blk is None:
                kw = dict(v=0)
            else:
                kw = dict(v=1, tag=str(blk // SETS), data=[f"m[{2 * blk}]", f"m[{2 * blk + 1}]"])
            out += cache_line(CACHE_X, top + e * GEO.lh, GEO, 2, **kw)
        if s == 0:
            out.append(rect(CACHE_X - 9, top - 7, GEO.width(2) + 18, 2 * GEO.lh + 14, "none",
                            BLUE, rx=7, width=3))
        else:
            out.append(rect(CACHE_X - 7, top - 5, GEO.width(2) + 14, 2 * GEO.lh + 10, "none",
                            LINE, rx=6, width=1.4, dash="5 3"))
        out.append(rect(CACHE_X - 66, centers[s] - 17, 54, 34, SET_FILLS[s * 2],
                        SET_STROKES[s * 2], rx=6, width=1.6))
        out.append(text(CACHE_X - 39, centers[s] + 6, f"组 {s}", 17, IDX, "bold"))
    out.append(text(W / 2, H - 16, CAPTION, 20, ORANGE, "bold"))
    return out


if __name__ == "__main__":
    save("sa-place", W, H, frame())
