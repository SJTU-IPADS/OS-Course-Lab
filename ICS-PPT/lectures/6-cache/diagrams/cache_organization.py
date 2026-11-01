#!/usr/bin/env python3
"""The general organisation of a cache: a block of B bytes, a line (valid bit, tag,
block), a set of E lines, S sets, and the capacity C = S x E x B. The drawn cache has
S = 4, E = 2, B = 8, so C = 4 x 2 x 8 = 64 bytes; no line holds data yet, the figure
shows the layout. The page shows the last of the five frames this module can draw
(frame(n) adds one part per frame); only that one is saved.
Run it to refresh ../assets/cache-organization.svg.
"""

from cachekit import *

W, H = 640, 420
GEO = Geo(vw=30, tw=46, bw=30, lh=32)
X0, Y0 = 170, 74
S, E, B = 4, 2, 8
SET_GAP = 16
NOTES = {1: "块：内存中连续 B 字节的副本",
         2: "行：有效位、标记与一个块",
         3: "组：E 行",
         4: "缓存：S 个组"}


def block_cells(x, y):
    """The B byte cells of a block, labelled B0 ... B7, without a valid bit or tag."""
    out = []
    for i in range(B):
        out.append(rect(x + i * GEO.bw, y, GEO.bw, GEO.lh, FILL_BLUE, INK, rx=0, width=1.2))
        out.append(text(x + (i + 0.5) * GEO.bw, y + GEO.lh / 2 + GEO.size * 0.36 - 1, f"B{i}",
                        GEO.size - 2, MUTED, font=MONO))
    out.append(rect(x, y, B * GEO.bw, GEO.lh, "none", INK, rx=0, width=1.8))
    return out


def frame(n):
    out = []
    w = GEO.width(B)
    bx = X0 + GEO.vw + GEO.tw
    set_h = E * GEO.lh
    bottom = Y0 + S * set_h + (S - 1) * SET_GAP
    if n == 1:
        out += block_cells(bx, Y0)
        out.append(rect(bx - 4, Y0 - 4, B * GEO.bw + 8, GEO.lh + 8, "none", ORANGE, rx=5,
                        width=3.2))
    elif n == 2:
        out += cache_line(X0, Y0, GEO, B, sym=True)
        out.append(rect(X0 - 4, Y0 - 4, GEO.vw + GEO.tw + 8, GEO.lh + 8, "none", ORANGE,
                        rx=5, width=3.2))
        out.append(text(X0 + GEO.vw, Y0 - 14, "有效位", 14, MUTED, anchor="end"))
        out.append(text(X0 + GEO.vw + 6, Y0 - 14, "标记", 14, TAG, "bold", anchor="start"))
    elif n == 3:
        shapes, pos, _ = cache_sets(X0, Y0, GEO, B, [[dict(sym=True)] * E], set_gap=SET_GAP,
                                    headers=False)
        out += shapes
        out.append(rect(X0 - 4, Y0 + GEO.lh - 4, w + 8, GEO.lh + 8, "none", ORANGE, rx=5,
                        width=3.2))
    else:
        sets = [[dict(sym=True) for _ in range(E)] for _ in range(S)]
        halo = {1: ORANGE, 2: ORANGE, 3: ORANGE} if n == 4 else None
        shapes, pos, _ = cache_sets(X0, Y0, GEO, B, sets, set_gap=SET_GAP, headers=False,
                                    set_halo=halo)
        out += shapes
    # the braces that name the parameters appear with the part they measure
    out += brace(bx, X0 + w, Y0 - 8 - (22 if n == 2 else 0), INK, "块：B = 8 字节", 16,
                 below=False)
    if n >= 3:
        out += vbrace(X0 + w + 12, Y0, Y0 + E * GEO.lh, INK, "E = 2 行", 16)
    if n >= 4:
        out += vbrace(X0 - 78, Y0, bottom, INK, "S = 4 组", 16, right=False)
    # the caption sits under what the frame has drawn so far
    under = {1: Y0 + GEO.lh, 2: Y0 + GEO.lh, 3: Y0 + set_h, 4: bottom, 5: bottom}[n]
    CAPTION_Y = under + 38
    if n == 5:
        out.append(text(X0 + w / 2, CAPTION_Y, "C = S × E × B = 4 × 2 × 8 = 64 字节", 20, INK,
                        "bold"))
    else:
        out.append(text(X0 + w / 2, CAPTION_Y, NOTES[n], 20, INK, "bold"))
    return out


if __name__ == "__main__":
    save("cache-organization", W, H, frame(5))
