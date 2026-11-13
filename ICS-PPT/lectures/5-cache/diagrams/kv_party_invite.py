#!/usr/bin/env python3
"""The crew combines the profile and the conclusion of guest 3 and invites the next
guest, in three frames.

Frame 1: on the left the profile card x3 of guest 3 (grey, under the muted label
个人资料) and below it the conclusion card c3 the crew wrote (orange). A bracket joins
them, and one arrow runs from it to the card x3 + c3 (strong outline, muted label
资料加结论 below it), level with the middle of the two. Frame 2: arrows from x3 + c3 to
the profile of each person on the waiting list (A, B, C, column title 候场名单), and one
score each (column title 比对得分). Frame 3: B, the highest score (2.4), is invited: B's
card on the waiting list gets the strong outline, and an arrow runs from B's score to B
drawn as a guest (orange), under the muted label 得分最高. The cards x3 and c3 stand on
the first and the third row of the waiting list, x3 + c3 on the second. The scores are
schematic. The data comes from kv_party_kit.py.
A later frame only adds elements or changes an outline. Nothing moves.
Run it to refresh ../assets/kv-party-invite-1.svg, kv-party-invite-2.svg and
kv-party-invite-3.svg.
"""

from kv_party_kit import *

W, H = 960, 236
LEFT = -12                                 # window shift: centres the drawing on the canvas
G = 2                                      # guest 3, 0-based
ROWS = [42 + 62 * i for i in range(3)]     # tops of the three rows of the waiting list
MID = ROWS[1] + CARD_H / 2                 # the vertical centre of the rows
TITLE = 26                                 # baseline of the column titles
X3X, X3W = 120, 150                        # the profile card x3, on the first row
C3W = 96                                   # the conclusion card c3, on the third row
JOIN = 300                                 # the bracket that joins x3 and c3
SUMX, SUMW = 344, 80                       # the card x3 + c3, on the second row
LISTX, LISTW = 512, 124                    # the profile cards of the waiting list
SCOREX = 694                               # centre of the score column
GX = 790                                   # centre of the glyph of the invited guest
CAPTIONS = [
    "① 节目组把嘉宾 3 的资料 x3 与结论 c3 合起来",
    "② x3 + c3 与候场名单上每人的资料比对，得到得分",
    "③ B 的得分最高，节目组邀请 B 入场",
]


def combine():
    """The profile x3 and the conclusion c3 join into the card x3 + c3."""
    xy, cy = ROWS[0] + CARD_H / 2, ROWS[2] + CARD_H / 2
    out = card(X3X, ROWS[0], X3W, f"x{G + 1}", PROFILE[G])
    out.append(text(X3X + X3W / 2, TITLE, "个人资料", 14, MUTED, "bold"))
    out += card(X3X, ROWS[2], C3W, f"c{G + 1}", "结论", "new")
    out.append(elbow([(X3X + X3W + 4, xy), (JOIN, xy), (JOIN, cy), (X3X + C3W + 4, cy)],
                     INK, 2, head=False))
    out.append(arrow(JOIN, MID, SUMX - 3, MID, INK, 2))
    out += card(SUMX, MID - CARD_H / 2, SUMW, "x3 + c3", strong=True)
    out.append(text(SUMX + SUMW / 2, MID + CARD_H / 2 + 19, "资料加结论", 14, MUTED, "bold"))
    return out


def scores():
    """From x3 + c3 to the profile of each person on the waiting list, and the scores."""
    out = [text(LISTX + LISTW / 2, TITLE, "候场名单", 14, MUTED, "bold"),
           text(SCOREX, TITLE, "比对得分", 14, MUTED, "bold")]
    for (name, prof, score), top in zip(WAITING, ROWS):
        cy = top + CARD_H / 2
        out.append(arrow(SUMX + SUMW + 3, MID, LISTX - 3, cy, INK, 1.8))
        out += card(LISTX, top, LISTW, name, prof)
        out.append(arrow(LISTX + LISTW + 4, cy, SCOREX - 22, cy, MUTED, 1.6))
        out.append(mono(SCOREX, cy + 5, f"{score:.1f}", 15, INK, "bold", "middle"))
    return out


def invite():
    """The guest with the highest score is invited: its card gets the strong outline, and
    an arrow runs from its score to the guest itself."""
    row = max(range(len(WAITING)), key=lambda idx: WAITING[idx][2])
    name, prof, _ = WAITING[row]
    top = ROWS[row]
    cy = top + CARD_H / 2
    out = card(LISTX, top, LISTW, name, prof, strong=True)
    out.append(arrow(SCOREX + 22, cy, GX - 26, cy, INK, 1.8))
    glyph = cy - PERSON_H / 2
    out += person(GX, glyph, "new", name)
    out.append(text(GX, glyph - 10, "得分最高", 14, MUTED, "bold"))
    return out


def figure(stage):
    out = combine()
    if stage >= 2:
        out += scores()
    if stage >= 3:
        out += invite()
    out.append(text(LEFT + W / 2, H - 10, CAPTIONS[stage - 1], 17, INK, "bold"))
    return out


if __name__ == "__main__":
    for stage in (1, 2, 3):
        save(f"kv-party-invite-{stage}", W, H, figure(stage), left=LEFT)
