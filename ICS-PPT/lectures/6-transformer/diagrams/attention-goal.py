#!/usr/bin/env python3
"""attention-goal: what attention adds to the embeddings (1 frame).

Left panel: "a fluffy blue creature" as chips, each with the grey column the
lookup table gives it; a column depends only on its own word and position.
Right panel: the same columns after attention; the creature column (green)
has taken in information from the fluffy and blue columns.
"""
from tk import *

H = 350
WORDS = ["a", "fluffy", "blue", "creature"]
CW, CH, NC = 32, 20, 7          # embedding column: cell size, cell count


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
            out.append(text(x + CW / 2, cy + CH / 2 + 6, "⋮", 17, stroke))
        else:
            t = 0.12 + 0.5 * (pseudo(seed, i, 0) + 1) / 2
            out.append(rect(x, cy, CW, CH, mix(fill, stroke, t), WHITE, rx=0, width=1))
    out.append(rect(x - 3, y - 3, CW + 6, NC * CH + 6, "none", stroke, rx=4, width=1.8))
    return out


PW, GAP, PY, PH = 448, 96, 14, 322   # panel width, gap between panels, top, height
LX = (W - 2 * PW - GAP) / 2
RX = LX + PW + GAP
CHIP_Y = PY + 46
COL_Y = CHIP_Y + 36 + 22


def side(px, after):
    b = []
    rw = row_width(WORDS, gap=14)
    els, spans = chip_row(px + (PW - rw) / 2, CHIP_Y, WORDS, gap=14)
    b += els
    centres = [c for _, c, _ in spans]
    for word, c in zip(WORDS, centres):
        b.append(arrow(c, CHIP_Y + 38, c, COL_Y - 6, LINE, 1.6))
        role = "upd" if (after and word == "creature") else "data"
        seed = word + ("'" if role == "upd" else "")
        b += col(c - CW / 2, COL_Y, seed, role)
    return b, centres


def build():
    b = []
    b += panel(LX, PY, PW, PH, "查表得到的嵌入", size=19)
    b += panel(RX, PY, PW, PH, "经过注意力", size=19)

    els, _ = side(LX, False)
    b += els
    b.append(text(LX + PW / 2, PY + PH - 16, "每列只取决于自身的词与位置", 18, MUTED))

    els, cs = side(RX, True)
    b += els
    bot = COL_Y + NC * CH + 8
    # information flows from fluffy and blue into creature
    b.append(curve(cs[1], bot, cs[3] + 6, bot + 2, bend=60, color=GREEN, width=2.2))
    b.append(curve(cs[2], bot, cs[3] - 8, bot + 2, bend=28, color=GREEN, width=2.2))
    b.append(text(RX + PW / 2, PY + PH - 16, "向量带有上下文", 18, GREEN, "bold"))

    my = COL_Y + NC * CH / 2
    b.append(arrow(LX + PW + 12, my, RX - 12, my, INK, 2.4))
    b.append(text((LX + PW + RX) / 2, my - 14, "注意力", 19, INK, "bold"))
    save("attention-goal", b, H)


build()
