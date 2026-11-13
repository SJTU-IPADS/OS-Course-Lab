#!/usr/bin/env python3
"""Why a token needs a query: the earlier tokens relate to the last one with different strength.

The row of tokens is the sentence of the lecture. Arcs run from the last token back to each
earlier token, and the stroke width of an arc is the weight of that token in the last row of
the weight matrix of kv_qkv.py (WEIGHTS[-1]), so the widths are those weights; a short label
says what the earlier token is to the last one. To the right of the last token, on the same
row, its vector x4 is multiplied by the matrix W_Q and gives the query q4; the label under q4
says what the last token is looking for (the wording of kv_qkv_roles.py). Run it to refresh ../assets/kv-relation.svg.
"""

from kv_kit import *
from kv_matrix import WORDS, sub_label
from kv_qkv import WEIGHTS
from kv_qkv_roles import CH, CW, WANTED, vec
from svgkit import box, path

W, H = 860, 198
N = 4
TW, TH, TP = 84, 32, 112                  # a token cell: width, height, pitch
TX0, TY = 56, 134                         # left edge of the first token, top of the row
SHARE = WEIGHTS[-1]
UNIT = 15                                 # stroke width per unit of weight
SPREAD = 20                               # arcs leave the last token this far apart
HEIGHT = [98, 68, 38]                    # arc height above the row, for tokens 1, 2, 3
RELATION = ["时间", "形容的对象", "程度"]    # what token j is to the last token
CHAIN_Y = TY + TH / 2                     # centre line of the x4 -> W_Q -> q4 chain


def centre(i):
    return TX0 + i * TP + TW / 2


def arcs():
    out = []
    last = N - 1
    for j in range(last):
        # the farther the target, the farther right the arc leaves, so arcs do not cross
        x1 = centre(last) + SPREAD * (1 - j)
        x2 = centre(j)
        h = HEIGHT[j] / 0.75              # control height giving this apex height
        tone = INK if SHARE[j] > 0.3 else MUTED
        d = f"M {x1:.1f} {TY} C {x1:.1f} {TY - h:.1f} {x2:.1f} {TY - h:.1f} {x2:.1f} {TY}"
        out.append(path(d, tone, SHARE[j] * UNIT, head=False))
        out.append(text((x1 + x2) / 2, TY - HEIGHT[j] - SHARE[j] * UNIT / 2 - 5, RELATION[j],
                        15, INK if tone == INK else MUTED, "bold"))
    return out


def tokens():
    out = []
    for i in range(N):
        x = TX0 + i * TP
        out += cell(x, TY, TW, TH, "", "mid")
        out.append(text(centre(i), TY + TH / 2 + 6, WORDS[i], 17, INK, "bold"))
    return out


def chain():
    out = []
    right = TX0 + (N - 1) * TP + TW         # right edge of the last token
    xx = right + 34
    wx, ww, wh = xx + CW + 40, 76, 38
    qx = wx + ww + 40
    out.append(arrow(right + 3, CHAIN_Y, xx - 5, CHAIN_Y, MUTED, 1.6))
    out += vec(xx, CHAIN_Y, f"x{N}")
    out.append(arrow(xx + CW + 3, CHAIN_Y, wx - 5, CHAIN_Y, INK, 2))
    out += box(wx, CHAIN_Y - wh / 2, ww, wh, "", fill=FILL_PURPLE, stroke=PURPLE)
    out += sub_label(wx + ww / 2, CHAIN_Y + 6, "W", "Q", 18)
    out.append(arrow(wx + ww + 3, CHAIN_Y, qx - 5, CHAIN_Y, INK, 2))
    out += vec(qx, CHAIN_Y, f"q{N}")
    out.append(text(qx + CW / 2, TY + TH + 24, f"要找：{WANTED}", 15, MUTED, "bold"))
    return out


if __name__ == "__main__":
    save("kv-relation", W, H, arcs() + tokens() + chain())
