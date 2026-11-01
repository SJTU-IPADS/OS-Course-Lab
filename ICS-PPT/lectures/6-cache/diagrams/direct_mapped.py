#!/usr/bin/env python3
"""A direct-mapped cache: S = 4 sets of one line, B = 2 bytes, in front of a 16-byte
main memory (m = 4) of 8 blocks. Main memory is drawn as an address space: a block is
two adjacent squares (its two bytes), the 4-bit address of the block sits in its upper
left corner and "块 k" to its left. Block k goes to set k mod 4; blocks of the same
colour compete for the one line of their set, and the tag (0 or 1) says which of the two
is there. Blocks 0 and 4 are framed: they can only take turns in the one line of set 0.
Run it to refresh ../assets/direct-mapped.svg.
"""

from cachekit import *

W, H = 700, 480
Y0, PITCH, BH = 76, 44, 38
LABEL_X, MEM_X, SQ = 100, 112, 38        # right edge of "块 k", left edge of a block, byte square
GEO = Geo(vw=30, tw=46, bw=44, lh=40)
CACHE_X = 480
CAPTION = "块 0 与块 4 只能轮流占用组 0 的这一行"


def frame():
    out = []
    out.append(text(MEM_X + SQ, 36, "内存：16 字节，8 个块", 18, GREEN, "bold"))
    out.append(text(CACHE_X + GEO.width(2) / 2, 36, "缓存：S = 4，E = 1，B = 2", 18, INK, "bold"))
    out.append(text(CACHE_X + GEO.width(2) / 2, 58, "4 组，每组 1 行，每个块 2 字节", 16, MUTED))
    centers = [Y0 + 40 + 80 * s for s in range(4)]
    focus = {0, 4}
    start = MEM_X + 2 * SQ + 78              # the connectors leave to the right of the tag
    # connectors first, so the boxes lie on top
    for k in range(8):
        s = k % 4
        ym = Y0 + k * PITCH + BH / 2
        colour = SET_STROKES[s] if k in focus else LINE
        out.append(line(start, ym, CACHE_X - 70, centers[s], colour, 3 if k in focus else 1.8))
    for k in range(8):
        s = k % 4
        y = Y0 + k * PITCH
        out.append(text(LABEL_X, y + BH / 2 + 6, f"块 {k}", 17, INK, "bold", "end"))
        for byte in range(2):
            out.append(rect(MEM_X + byte * SQ, y, SQ, BH, SET_FILLS[s], GREEN, rx=0, width=1.4))
        out.append(text(MEM_X + 3, y + 14, format(2 * k, "04b"), 12, MUTED, anchor="start",
                        font=MONO))
        out.append(text(MEM_X + 2 * SQ + 12, y + BH / 2 + 5, f"tag = {k // 4}", 16, TAG, "bold",
                        "start"))
        if k in focus:
            out.append(rect(MEM_X - 4, y - 4, 2 * SQ + 8, BH + 8, "none", ORANGE, rx=4,
                            width=3.2))
    for s in range(4):
        cy = centers[s]
        out += cache_line(CACHE_X, cy - GEO.lh / 2, GEO, 2, sym=True,
                          halo=ORANGE if s == 0 else None)
        out.append(rect(CACHE_X - 66, cy - 17, 54, 34, SET_FILLS[s], SET_STROKES[s], rx=6,
                        width=1.6))
        out.append(text(CACHE_X - 39, cy + 6, f"组 {s}", 17, IDX, "bold"))
    out.append(text(W / 2, H - 16, CAPTION, 20, ORANGE, "bold"))
    return out


if __name__ == "__main__":
    save("direct-mapped", W, H, frame())
