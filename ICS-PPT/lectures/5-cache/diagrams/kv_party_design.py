#!/usr/bin/env python3
"""The parties of the matchmaking show, for the question page, in one figure.

The three regions of kv_party_flow.py at the same horizontal positions, so the next page
reads as this figure with the process added: the backstage box (幕后) with three waiting
guests, the stage (舞台) with three guests, and the crew box (节目组) with three crew
members. Here the three boxes have the height of the backstage box, the title centred at
the top and the person glyphs in one row below it, evenly spaced. Everyone is a grey
person glyph, nobody is named, and nothing else is drawn: no notes, no arrows, no labels
besides the region titles, since the page asks for the process and gives none of it.
Run it to refresh ../assets/kv-party-design.svg .
"""

from kv_party_kit import *
from kv_party_flow import BH, BW, BX, KR, KX, LEFT, SR, SX, W

TOP = 4                                    # top of the three boxes
H = TOP + BH + 4


def box(x, w, title, count):
    """A box BH tall with its title centred at the top and count glyphs below it."""
    out = [rect(x, TOP, w, BH, WHITE, LINE, rx=6, width=1.4),
           text(x + w / 2, TOP + 24, title, 16, INK, "bold")]
    if x == BX:                            # the backstage box: glyphs as in kv_party_flow.py
        centres = [BX + 28 + 42 * idx for idx in range(count)]
    else:
        centres = [x + w * (idx + 1) / (count + 1) for idx in range(count)]
    for cx in centres:
        out += person(cx, TOP + 36)
    return out


def figure():
    return (box(BX, BW, "幕后", 3) + box(SX, SR - SX, "舞台", 3)
            + box(KX, KR - KX, "节目组", 3))


if __name__ == "__main__":
    save("kv-party-design", W, H, figure(), left=LEFT)
