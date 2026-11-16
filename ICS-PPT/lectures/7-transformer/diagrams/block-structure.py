#!/usr/bin/env python3
"""block-structure: an attention block is many heads working side by side.

Top: "a fluffy blue creature roamed the verdant forest" and its embeddings
E_1 .. E_8 (grey columns, as in fluffy-example). They enter the attention
block (the blue box of layer-stack), which holds heads 1, 2, 3, ..., 96.
Each head is drawn over the same eight positions (small squares, in the
order of the sentence), with arcs from the earlier word to the later one:
  head 1  adjective -> noun     fluffy, blue -> creature; verdant -> forest
  head 2  determiner -> noun    a -> creature; the -> forest
  head 3  subject -> verb       creature -> roamed
Head 96 stands for the heads not drawn: no relation is named for it.
Under the squares, the head's change for every position (small green
columns, dark where the head changes that position). The heads' changes are
added (the + between the boxes) and the sum is added, in the adder, to the
embeddings that bypass the block, giving E'_1 .. E'_8 at
the bottom; the positions most changed (creature, roamed, forest) are green.
Head 1, the head fluffy-example works through, has the orange outline of a
highlight.
"""
from tk import *
from attn_data import WORDS

H = 400
N = len(WORDS)

# embeddings: columns of shaded cells, the drawing of fluffy-example at a
# smaller scale
CW, CH, NC = 18, 8, 5
CS = 17                                  # chip font size
CHIP_Y, E_Y = 2, 38
BRK = E_Y + NC * CH + 6                  # top of the bracket under the E row

# the attention block and its heads
BX0, BX1 = 22, 998
BT = BRK + 24                            # top of the block
HT = BT + 30                             # top of the head boxes
HW, HH = 202, 118                        # head box
PITCH, SQ = 26, 12                       # positions inside a head
PLUS_GAP, MORE_GAP = 30, 80
DOT_Y = HT + 46                          # top of the position squares
DE_Y, DE_W, DE_C = HT + 64, 12, 7        # change columns: top, width, cell
BB = HT + HH + 10                        # bottom of the block

PY = BB + 42                             # the adder
E2_Y = PY + 48                           # top of the E' row
XR = BX1 + 20                            # the bypass

# per head: (number, relation, arcs (from, to), change per position)
HEADS = [
    (1, "形容词 → 名词", [(1, 3), (2, 3), (6, 7)],
     [0.06, 0.10, 0.08, 0.80, 0.06, 0.05, 0.10, 0.75]),
    (2, "限定词 → 名词", [(0, 3), (5, 7)],
     [0.08, 0.05, 0.06, 0.60, 0.10, 0.08, 0.05, 0.60]),
    (3, "主语 → 动词", [(3, 4)],
     [0.05, 0.08, 0.05, 0.10, 0.80, 0.06, 0.08, 0.05]),
    (96, "", [],
     [0.14, 0.18, 0.10, 0.20, 0.16, 0.12, 0.18, 0.14]),
]
CHANGED = (3, 4, 7)                      # creature, roamed, forest


def mix(c1, c2, t):
    a = [int(c1[i:i + 2], 16) for i in (1, 3, 5)]
    b_ = [int(c2[i:i + 2], 16) for i in (1, 3, 5)]
    return "#%02x%02x%02x" % tuple(round(x + (y - x) * t) for x, y in zip(a, b_))


def col(x, y, seed, role="data"):
    """An embedding as a column of shaded cells (fluffy-example's drawing)."""
    stroke, fill = ROLE[role]
    out = []
    for i in range(NC):
        t = 0.12 + 0.5 * (pseudo(seed, i, 0) + 1) / 2
        out.append(rect(x, y + i * CH, CW, CH, mix(fill, stroke, t), WHITE, rx=0, width=1))
    out.append(rect(x - 2, y - 2, CW + 4, NC * CH + 4, "none", stroke, rx=3, width=1.6))
    return out


def change(x, y, t, seed):
    """One head's change for one position: a small green column, darker for
    a larger change."""
    out = []
    for i in range(3):
        k = t * (0.85 + 0.15 * pseudo(seed, i, 0))
        out.append(rect(x, y + i * DE_C, DE_W, DE_C, mix(FILL_UPD, UPD, k), WHITE,
                        rx=0, width=0.8))
    out.append(rect(x - 1.5, y - 1.5, DE_W + 3, 3 * DE_C + 3, "none", UPD, rx=2, width=1.2))
    return out


def head(x, number, relation, arcs, changes):
    out = [rect(x, HT, HW, HH, WHITE, ORANGE if number == 1 else LINE, rx=6,
                width=2.4 if number == 1 else 1.4),
           text(x + HW / 2, HT + 23, f"头 {number}", 19, INK, "bold")]
    x0 = x + (HW - (N - 1) * PITCH) / 2
    xs = [x0 + k * PITCH for k in range(N)]
    used = {k for arc in arcs for k in arc}
    for k, cx in enumerate(xs):
        out.append(rect(cx - SQ / 2, DOT_Y, SQ, SQ, DATA if k in used else FILL_DATA, DATA,
                        rx=2, width=1.2))
        out += change(cx - DE_W / 2, DE_Y, changes[k], f"bs{number}:{k}")
    into = {}
    for s, t in arcs:
        into.setdefault(t, []).append(s)
    for s, t in arcs:
        srcs = sorted(into[t])
        shift = (srcs.index(s) - (len(srcs) - 1) / 2) * 6
        out.append(curve(xs[s], DOT_Y - 2, xs[t] + shift, DOT_Y - 3,
                         bend=-14 - 7 * (t - s), color=UPD, width=1.4))
    if relation:
        out.append(text(x + HW / 2, HT + HH - 10, relation, 18, INK))
    return out


def plus(x, y, r=14):
    return [circle(x, y, r, WHITE, INK, 1.8),
            line(x - 7, y, x + 7, y, INK, 2), line(x, y - 7, x, y + 7, INK, 2)]


def figure():
    b = []
    # the sentence and its embeddings
    rw = row_width(WORDS, CS, 12, 8)
    els, spans = chip_row((BX0 + BX1) / 2 - rw / 2, CHIP_Y, WORDS, size=CS, gap=12,
                          h=28, pad=8)
    b += els
    lanes = [c for _, c, _ in spans]
    for i, cx in enumerate(lanes):
        b += col(cx - CW / 2, E_Y, WORDS[i])
        b += math(cx + CW / 2 + 6, E_Y + NC * CH / 2 + 6, f"E_{i + 1}", 18, DATA,
                  anchor="start")
    mid = (BX0 + BX1) / 2
    lo, hi = lanes[0] - CW / 2 - 6, lanes[-1] + CW / 2 + 6
    b.append(path(f"M {lo:.1f} {BRK:.1f} L {lo:.1f} {BRK + 7:.1f} L {hi:.1f} {BRK + 7:.1f} "
                  f"L {hi:.1f} {BRK:.1f}", INK, 1.6, head=False))
    b.append(arrow(mid, BRK + 7, mid, BT - 3, INK, 2))
    # the bypass from the E row to the adder
    b.append(elbow([(hi, BRK + 7), (XR, BRK + 7), (XR, PY), (mid + 18, PY)], INK, 2))

    # the attention block
    b.append(rect(BX0, BT, BX1 - BX0, BB - BT, FILL_BLUE, BLUE, rx=6, width=1.6))
    b.append(text(BX0 + 14, BT + 23, "注意力块", 18, INK, "bold", anchor="start"))
    x = BX0 + 14
    plus_y = DE_Y + 3 * DE_C / 2 + 8
    for k, (number, relation, arcs, changes) in enumerate(HEADS):
        b += head(x, number, relation, arcs, changes)
        x += HW
        if k < 2:
            b.append(label(x + PLUS_GAP / 2, plus_y, "+", 24))
            x += PLUS_GAP
        elif k == 2:
            b.append(label(x + MORE_GAP / 2, plus_y, "+ ⋯ +", 22))
            x += MORE_GAP

    # the heads' changes, summed, are added to the embeddings
    b.append(arrow(mid, BB + 2, mid, PY - 18, UPD, 2))
    b += plus(mid, PY)
    b.append(arrow(mid, PY + 16, mid, E2_Y - 15, INK, 2))
    b.append(path(f"M {lo:.1f} {E2_Y - 6:.1f} L {lo:.1f} {E2_Y - 13:.1f} "
                  f"L {hi:.1f} {E2_Y - 13:.1f} L {hi:.1f} {E2_Y - 6:.1f}", INK, 1.6,
                  head=False))
    for i, cx in enumerate(lanes):
        upd = i in CHANGED
        b += col(cx - CW / 2, E2_Y, WORDS[i] + ("'" if upd else ""), "upd" if upd else "data")
        b += math(cx + CW / 2 + 6, E2_Y + NC * CH / 2 + 6, f"E′_{i + 1}", 18,
                  UPD if upd else DATA, anchor="start")
    save("block-structure", b, H)


figure()
