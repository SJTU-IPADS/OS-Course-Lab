#!/usr/bin/env python3
"""layer-stack: the vectors flow up through alternating blocks.

1: bottom to top: the tokens, their embeddings, then Attention, MLP,
   Attention, MLP and so on; one column of vectors per token.
2: + the last vector of the top row (orange) gives a probability for every
   token of the vocabulary.
"""
import hashlib

from tk import *

H = 400
TOKS = ["rest", "thinker", "of", "all", "time", "was"]
LX0, LP = 146, 82                 # lane centres
LANES = [LX0 + k * LP for k in range(len(TOKS))]
BL, BR = LANES[0] - 44, LANES[-1] + 44       # band extent
VW, VC, NV = 24, 9, 4             # mini vectors: width, cell height, cells

# stages from the top: (kind, top y)
TOPV = 8
STAGES = [("vec", TOPV), ("dots", 54), ("band:MLP", 80), ("band:Attention", 120),
          ("vec", 160), ("band:MLP", 206), ("band:Attention", 246), ("vec", 286)]
CHIP_Y = 352


def shade(seed, i, j):
    h = hashlib.md5(f"{seed}:{i}:{j}".encode()).digest()[0] / 255.0
    g = round(238 - 120 * h)
    return f"#{g - 10:02x}{g - 2:02x}{g + 6 if g + 6 < 256 else 255:02x}"


def mini(cx, y, seed, stroke=DATA, width=1.4):
    out = []
    for i in range(NV):
        out.append(rect(cx - VW / 2, y + i * VC, VW, VC, shade(seed, i, 0), WHITE, rx=0, width=0.8))
    out.append(rect(cx - VW / 2 - 2, y - 2, VW + 4, NV * VC + 4, "none", stroke, rx=3, width=width))
    return out


def frame(n):
    b = []
    # lanes behind everything
    for x in LANES:
        b.append(line(x, TOPV + NV * VC, x, CHIP_Y - 4, LINE, 1.6))
    b.append(label(LANES[0] - 50, CHIP_Y + 23, "⋯", 20, MUTED))
    for k, (x, t) in enumerate(zip(LANES, TOKS)):
        els, _ = chip(x, CHIP_Y, t, size=18, h=32, anchor="middle")
        b += els
    for kind, y in STAGES:
        if kind == "vec":
            for k, x in enumerate(LANES):
                hl = n >= 2 and y == TOPV and k == len(LANES) - 1
                b += mini(x, y, f"ls{y}:{k}", ORANGE if hl else DATA, 2.4 if hl else 1.4)
        elif kind == "dots":
            for x in LANES:
                b.append(rect(x - 8, y - 12, 16, 22, WHITE, "none", rx=0, width=0))
                b.append(label(x, y + 6, "⋮", 18, MUTED))
        else:
            name = kind.split(":")[1]
            b.append(rect(BL, y, BR - BL, 30, FILL_BLUE, BLUE, rx=6, width=1.6))
            b.append(label((BL + BR) / 2, y + 21, name, 18, INK, "bold"))
    # flow direction
    b.append(arrow(BL - 34, CHIP_Y - 10, BL - 34, TOPV + 10, MUTED, 2))

    if n >= 2:
        x0, base, hmax = 652, 262, 150
        vals = []
        for i in range(64):
            h = hashlib.md5(f"vocab:{i}".encode()).digest()[0] / 255.0
            vals.append(0.25 * h ** 8)
        vals[9], vals[23], vals[40], vals[51] = 1.0, 0.45, 0.3, 0.18
        b.append(line(x0 - 6, base, x0 + 64 * 5.5 + 2, base, LINE, 1.4))
        for i, v in enumerate(vals):
            hgt = max(1.5, hmax * v)
            b.append(rect(x0 + i * 5.5, base - hgt, 4, hgt, BLUE, "none", rx=0, width=0))
        b.append(label(x0 + 32 * 5.5, base + 30, "所有 token 的概率分布", 19, INK))
        ey = TOPV + NV * VC / 2
        b.append(elbow([(LANES[-1] + VW / 2 + 6, ey), (BR + 22, ey), (BR + 22, base - hmax / 2),
                        (x0 - 12, base - hmax / 2)], ORANGE, 2.2))
    save(f"layer-stack-{n}", b, H)


for n in (1, 2):
    frame(n)
