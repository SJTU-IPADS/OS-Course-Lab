#!/usr/bin/env python3
"""The process of the matchmaking show in the abstract, in four frames.

Three regions from left to right: the backstage box (幕后) with three waiting guests, the
stage (舞台) with two earlier guests (grey, labelled 已在场的嘉宾) and one newly entering
guest (orange, labelled 新入场的嘉宾), and the crew box (节目组). The notes are kept by the
crew: the crew box holds a row of notes, one per guest who has entered, and the notes of
the two earlier guests (grey) are there from the first frame on. No guest is named and no
note carries content: the notes are blank sheets with grey lines.
Frame 1: an arrow from the backstage box over the stage to the new guest, ① 入场.
Frame 2: an arrow from the new guest to the crew box, labelled ② 介绍 in the gap between
         the stage and the crew box. The new guest's note appears at the end of the
         crew's row of notes with an orange outline.
Frame 3: an orange card 兴趣 (the new guest's interest) appears in the crew box below its
         title, left of the notes. A line runs from it below the notes and rises into the
         conclusion sheet (结论, orange) at the end of the row. Every note the crew holds,
         the new one included, drops one line onto it. ③ 阅读 stands below the line.
Frame 4: an arrow from the crew box below the conclusion back to the backstage box, below
         the stage, closing the cycle, ④ 邀请.
A later frame only adds elements, nothing moves. The shapes come from kv_party_kit.py.
Run it to refresh ../assets/kv-party-flow-1.svg .. kv-party-flow-4.svg.
"""

from kv_party_kit import *

W, H = 960, 190
LEFT = 7                                   # window shift: centres the drawing on the canvas
LANE = 28                                  # the entry arrow runs above the stage
TOP = 42                                   # top of the three regions
BOTTOM = 156                               # bottom of the stage and of the crew box
BX, BW, BH = 20, 140, 92                   # the backstage box
SX, SR = 190, 478                          # the stage: left, right
GT = 58                                    # top of the glyphs on the stage
MID = GT + PERSON_H / 2                    # the introduction arrow runs here
OLD = [262, 334]                           # centres of the earlier guests
NEW = 420                                  # centre of the new guest
TAG = 128                                  # baseline of the guest labels
KX, KR = 556, 954                          # the crew box: left, right
QX, QW = 570, 46                           # the interest card: left, width
NOTES = [664, 736, 808]                    # centres of the notes, the last one the new
NT, NW, NH = MID - 18, 60, 36              # the notes: top, width, height
BUS = 124                                  # the reading line runs here
CX, CWD, CHT = 872, 66, 44                 # the conclusion sheet: left, width, height
CY = MID - CHT / 2
RET = 176                                  # the return arrow runs below the stage
STEP = 15                                  # size of the step labels


def blank(cx, kind="mid"):
    """A blank note centred on cx: a sheet with three short grey lines."""
    x = cx - NW / 2
    out = paper(x, NT, NW, NH, kind)
    for i, end in enumerate((NW - 12, NW - 12, NW - 26)):
        y = NT + 11 + 8 * i
        out.append(line(x + 9, y, x + end, y, LINE, 2))
    return out


def region(x, w, h, title, centred=False):
    """A box from TOP, h tall, with its title at the top (centred or at the left)."""
    out = [rect(x, TOP, w, h, WHITE, LINE, rx=6, width=1.4)]
    if centred:
        out.append(text(x + w / 2, TOP + 24, title, 16, INK, "bold"))
    else:
        out.append(text(x + 14, TOP + 24, title, 16, INK, "bold", "start"))
    return out


def regions():
    """The three boxes, the waiting guests, the guests on the stage and the notes the
    crew holds for the earlier guests."""
    out = region(BX, BW, BH, "幕后", centred=True)
    for i in range(3):
        out += person(BX + 28 + 42 * i, TOP + 36)
    out += region(SX, SR - SX, BOTTOM - TOP, "舞台")
    for cx in OLD:
        out += person(cx, GT)
    out.append(text(sum(OLD) / 2, TAG, "已在场的嘉宾", 14, MUTED, "bold"))
    out += person(NEW, GT, "new")
    out.append(text(NEW, TAG, "新入场的嘉宾", 14, MUTED, "bold"))
    out += region(KX, KR - KX, BOTTOM - TOP, "节目组")
    for cx in NOTES[:-1]:
        out += blank(cx)
    return out


def enter():
    """① the new guest comes from backstage over the stage."""
    mid = BX + BW / 2
    return [elbow([(mid, TOP - 1), (mid, LANE), (NEW, LANE), (NEW, GT - 3)], INK, 2),
            text((mid + NEW) / 2, LANE - 9, "① 入场", STEP, INK, "bold")]


def introduce():
    """② the new guest introduces themselves to the crew, the crew records a new note."""
    return [arrow(NEW + 21, MID, KX - 3, MID, INK, 2),
            text((SR + KX) / 2, MID - 9, "② 介绍", STEP, INK, "bold")] + blank(NOTES[-1],
                                                                               "new")


def read():
    """③ the crew reads every note by the new guest's interest into the conclusion."""
    fill, stroke = KINDS["new"]
    qy = BUS - CARD_H / 2
    rise = CX + CWD / 2
    out = [rect(QX, qy, QW, CARD_H, fill, stroke, rx=4, width=1.4),
           text(QX + QW / 2, BUS + 5, "兴趣", STEP, INK, "bold")]
    out += [line(cx, NT + NH, cx, BUS, INK, 2) for cx in NOTES]
    out.append(elbow([(QX + QW + 1, BUS), (rise, BUS), (rise, CY + CHT + 3)], INK, 2))
    out += paper(CX, CY, CWD, CHT, "new")
    out.append(text(CX + CWD / 2, CY + CHT / 2 + 6, "结论", 16, INK, "bold"))
    out.append(text((QX + QW + rise) / 2, BUS + 22, "③ 阅读", STEP, INK, "bold"))
    return out


def invite():
    """④ the crew invites the next guest from backstage."""
    kc, bc = CX + CWD / 2, BX + BW / 2
    return [elbow([(kc, BOTTOM + 1), (kc, RET), (bc, RET), (bc, TOP + BH + 3)], INK, 2),
            text(bc + 10, (TOP + BH + RET) / 2 + 6, "④ 邀请", STEP, INK, "bold", "start")]


def figure(stage):
    out = regions() + enter()
    if stage >= 2:
        out += introduce()
    if stage >= 3:
        out += read()
    if stage >= 4:
        out += invite()
    return out


if __name__ == "__main__":
    for stage in (1, 2, 3, 4):
        save(f"kv-party-flow-{stage}", W, H, figure(stage), left=LEFT)
