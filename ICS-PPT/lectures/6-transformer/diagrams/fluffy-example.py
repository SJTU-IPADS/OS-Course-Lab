#!/usr/bin/env python3
"""fluffy-example: the running example and the result attention should give.

1: "a fluffy blue creature roamed the verdant forest", each word with its
   embedding E_1 .. E_8 (grey columns); each encodes only its word and
   position.
2: the target row E′_1 .. E′_8: E′_4 (creature) and E′_8 (forest) are green,
   with information from fluffy and blue into creature, and from verdant
   into forest.
"""
from tk import *
from attn_data import WORDS

H = 400
CW, CH, NC = 30, 14, 8
CS, GAP = 19, 14
X0 = 100
CHIP_Y, E_Y = 10, 80
P_Y = E_Y + NC * CH + 58                # top of the E′ row


def mix(c1, c2, t):
    a = [int(c1[i:i + 2], 16) for i in (1, 3, 5)]
    b_ = [int(c2[i:i + 2], 16) for i in (1, 3, 5)]
    return "#%02x%02x%02x" % tuple(round(x + (y - x) * t) for x, y in zip(a, b_))


def col(x, y, seed, role="data"):
    """An embedding as a column of shaded cells, ⋮ in the cell before last."""
    stroke, fill = ROLE[role]
    out = []
    for i in range(NC):
        cy = y + i * CH
        if i == NC - 2:
            out.append(rect(x, cy, CW, CH, WHITE, WHITE, rx=0, width=1))
            out.append(text(x + CW / 2, cy + CH / 2 + 5, "⋮", 15, stroke))
        else:
            t = 0.12 + 0.5 * (pseudo(seed, i, 0) + 1) / 2
            out.append(rect(x, cy, CW, CH, mix(fill, stroke, t), WHITE, rx=0, width=1))
    out.append(rect(x - 3, y - 3, CW + 6, NC * CH + 6, "none", stroke, rx=4, width=1.8))
    return out


def frame(n):
    b = []
    els, spans = chip_row(X0, CHIP_Y, WORDS, size=CS, gap=GAP)
    b += els
    cs = [c for _, c, _ in spans]
    for i, c in enumerate(cs):
        b += math(c, E_Y - 14, f"E_{i + 1}", 20, DATA)
        b += col(c - CW / 2, E_Y, WORDS[i])
    right = spans[-1][2]
    if n == 1:
        y0, y1 = E_Y, E_Y + NC * CH
        bx = right + 26
        b.append(path(f"M {bx:.1f} {y0:.1f} L {bx + 8:.1f} {y0:.1f} L {bx + 8:.1f} {y1:.1f} "
                      f"L {bx:.1f} {y1:.1f}", MUTED, 1.6, head=False))
        b.append(text(bx + 22, (y0 + y1) / 2 + 6, "每个嵌入编码词与位置", 18, MUTED, anchor="start"))
    if n >= 2:
        for i, c in enumerate(cs):
            upd = i in (3, 7)
            b += col(c - CW / 2, P_Y, WORDS[i] + ("'" if upd else ""), "upd" if upd else "data")
            b += math(c, P_Y + NC * CH + 30, f"E′_{i + 1}", 20, GREEN if upd else DATA)
        top, bot = E_Y + NC * CH + 8, P_Y - 8
        b.append(curve(cs[1], top, cs[3] - 8, bot, bend=22, color=GREEN, width=2.2))
        b.append(curve(cs[2], top, cs[3] + 7, bot, bend=10, color=GREEN, width=2.2))
        b.append(curve(cs[6], top, cs[7], bot, bend=14, color=GREEN, width=2.2))
        mid = (top + bot) / 2
        b.append(arrow(right + 40, top + 2, right + 40, bot, INK, 2.2))
        b.append(text(right + 54, mid + 7, "注意力", 19, INK, "bold", anchor="start"))
    save(f"fluffy-example-{n}", b, H)


for n in (1, 2):
    frame(n)
