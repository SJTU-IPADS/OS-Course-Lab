#!/usr/bin/env python3
"""What one guest of the matchmaking show brings, for guest 3, in two figures.

On the left guest 3 and the profile x3 (个人资料). Three arrows fan out from x3 to what is
derived from it: the feature k3 (特点), the value v3 (价值) and the interest q3 (兴趣),
each named to the right of its card. kv-party-info.svg shows only these: the three cards
are evenly spaced and x3 is level with v3, so the arrows are symmetric about the middle
one. kv-party-note.svg keeps x3, k3 and q3 at the same positions and moves v3 up under k3,
both inside the note the crew records (纸条（节目组记录）), q3 apart with the title
只在入场时使用. Everything is grey. The data and the guest, card and
note shapes come from kv_party_kit.py.
Run it to refresh ../assets/kv-party-info.svg and kv-party-note.svg.
"""

from kv_party_kit import *

W, H = 960, 186
LEFT = 30                                   # window shift: centres the drawing on the canvas
G = 2                                      # guest 3, 0-based
GX = 206                                   # centre of the glyph
X3X, X3W = 260, 150                        # the card x3
NX, NY, NW = 590, 32, 176                  # the note: left, top, width
QY = 150                                   # top of the card q3
TITLE = 15                                 # size of the titles of the note and of q3


def figure(note):
    krow, vrow = slip_rows(NY)
    if not note:                           # evenly spaced, so the arrows are symmetric
        vrow = (krow + QY) / 2
    rows = [krow, vrow, QY]
    targets = [row + CARD_H / 2 for row in rows]
    mid = (targets[0] + targets[2]) / 2
    x3y = mid - CARD_H / 2
    cx, cw = NX + SLIP_PAD, NW - 2 * SLIP_PAD      # the cards, at their place in the note
    out = person(GX, mid - 32, "mid", "嘉宾 3")
    out += card(X3X, x3y, X3W, "x3", PROFILE[G])
    out.append(text(X3X + X3W / 2, x3y - 10, "个人资料", 14, MUTED, "bold"))
    for y in targets:
        out.append(arrow(X3X + X3W + 3, mid, cx - 4, y, INK, 1.8))
    if note:
        out += slip(NX, NY, NW, G)
        out.append(text(NX + NW / 2, NY - 10, "纸条（节目组记录）", TITLE, INK, "bold"))
        out.append(text(NX + NW / 2, QY - 10, "只在入场时使用", TITLE, INK, "bold"))
    else:
        out += card(cx, krow, cw, f"k{G + 1}", FEATURE[G])
        out += card(cx, vrow, cw, f"v{G + 1}", VALUE[G])
    out += card(cx, QY, cw, f"q{G + 1}", INTEREST[G])
    for row, word in zip(rows, ("特点", "价值", "兴趣")):
        out.append(text(NX + NW + 12, row + CARD_H / 2 + 5, word, 14, MUTED, "bold", "start"))
    return out


if __name__ == "__main__":
    save("kv-party-info", W, H, figure(False), left=LEFT)
    save("kv-party-note", W, H, figure(True), left=LEFT)
