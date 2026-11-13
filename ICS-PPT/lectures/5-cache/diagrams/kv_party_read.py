#!/usr/bin/env python3
"""Right after guest 3 enters, the crew reads its notes by the interest of guest 3, in
three frames.

Each guest is a column: the person glyph with the name below it, and below them the note
the crew recorded for the guest. The note holds three cards, from the top the interest q,
the feature k and the value v. The card q is narrower than k and v (kv_party_kit.Q_W) and
leaves a channel on its right inside the note, through which every line enters and leaves
the note. Guests 1 and 2 are grey, guest 3 has just entered and is orange, glyph and
note. The caption stands in the bottom left corner, on the row of the conclusion card, so
the figure stays low.
Frame 1: the three columns only.
Frame 2: q3 gets the strong outline. A line rises from the top edge of q3 to a bus that
         runs between the names and the notes, and the bus drops through the channel of
         each note onto k1, k2 and k3, k3 included. The attention of guest 3 to each
         note (kv_party_kit.ATTENTION[2]) stands right of each drop, between the bus and
         the note.
Frame 3: lines from the bottom of v1, v2 and v3, under the drops and 1 + 6 x the
         attention wide, merge into the conclusion c3 (orange) below guest 3. The
         weighted sum is the second line of the caption.
A later frame only adds elements, nothing moves. The column layout is defined here once
and shared with kv_party_enter.py: rows(gap) gives the heights below the bus when the
notes start gap below it. Importing this module draws nothing. The data and the shapes
come from kv_party_kit.py.
Run it to refresh ../assets/kv-party-read-1.svg, kv-party-read-2.svg and
kv-party-read-3.svg.
"""

from collections import namedtuple

from kv_party_kit import *

W = 960
CW = 190                                   # width of a guest column, the width of a note
PITCH = 236                                # distance between the left edges of two columns
GX = 18                                    # glyph centre, from the left edge of the column
GTOP = 2                                   # top of the glyphs
NAME_Y = GTOP + PERSON_H + 17              # baseline of the names
BUS = NAME_Y + 10                          # the comparison runs along here
RISE = SLIP_PAD + Q_W / 2                  # x of the line from q to the bus, under q
DROP = (SLIP_PAD + Q_W + CW - SLIP_PAD) / 2  # x of the drops and the value lines (channel)
C_W = 96                                   # width of the conclusion card
RIGHT = max(CW, DROP + C_W / 2)            # right end of the drawing, from the last column

# the heights below the bus: top of the notes, tops of the cards q, k and v on a note,
# the lane where the value lines merge, top of the conclusion card
Rows = namedtuple("Rows", "slip q k v lane c")


def rows(gap):
    """The heights below the bus when the notes start gap below it."""
    slip = BUS + gap
    q, k, v = slip_rows(slip, 3)
    lane = slip + slip_height(3) + 8
    return Rows(slip, q, k, v, lane, lane + 16)


AT = rows(18)                              # the attention numbers stand in the gap
H = AT.c + 30
CAPTION_Y = [AT.c + 5, AT.c + 25]          # baselines of the caption lines

CAPTIONS = [
    ["① 嘉宾 3 入场时，节目组已有嘉宾 1、2 的纸条，", "又记下嘉宾 3 的纸条"],
    ["② 节目组用嘉宾 3 的兴趣 q3 对照每张纸条上的特点，得到注意程度"],
    ["③ 节目组按注意程度记录每张纸条上的价值，写成结论："],
]


def columns(n):
    """Left edges of n guest columns, the drawing centred on the canvas."""
    left = (W - RIGHT - PITCH * (n - 1)) / 2
    return [left + PITCH * i for i in range(n)]


def column(at, x0, guest, name, kind="mid", interest=None, note=None, strong=False):
    """One guest (0-based) at x0: glyph and name, and below them the note with q, k and v
    at the heights of at. The paper, k and v take the colour of note, q the colour of
    interest (outlined if strong). Both colours default to kind, the colour of the
    glyph."""
    out = person(x0 + GX, GTOP, kind)
    out.append(text(x0, NAME_Y, name, 15, INK, "bold", "start"))
    out += slip(x0, at.slip, CW, guest, note or kind, interest=interest or kind,
                q_strong=strong)
    return out


def drops(x0s):
    return [x0 + DROP for x0 in x0s]


def compare(at, x0s):
    """The interest q of the last column rises from the top edge of its card to the bus,
    and the bus drops through the channel of every note onto the feature k, the own note
    included."""
    xs = drops(x0s)
    rise = x0s[-1] + RISE
    tip = at.k - 2
    out = [elbow([(rise, at.q - 1), (rise, BUS), (xs[0], BUS), (xs[0], tip)], INK, 2),
           line(rise, BUS, xs[-1], BUS, INK, 2)]
    out += [arrow(x, BUS, x, tip, INK, 2) for x in xs[1:]]
    return out


def merge(at, x0s, widths=None):
    """The value v of every note runs down from the bottom of the card, under the drop,
    to the lane and into the conclusion card of the last column; widths sets the width
    of each line (2 by default)."""
    xs = drops(x0s)
    widths = widths or [2] * len(xs)
    out = [line(x, at.v + CARD_H, x, at.lane + 1, MUTED, w) for x, w in zip(xs, widths)]
    out.append(line(xs[0], at.lane, xs[-1], at.lane, MUTED, 2))
    out.append(arrow(xs[-1], at.lane, xs[-1], at.c - 2, MUTED, 2))
    return out


def conclusion_x(x0s):
    """Left edge of the conclusion card, centred under the last drop."""
    return drops(x0s)[-1] - C_W / 2


def conclusion(at, x0s, guest):
    return card(conclusion_x(x0s), at.c, C_W, f"c{guest + 1}", "结论", "new")


def figure(stage):
    x0s = columns(3)
    share = ATTENTION[2]
    out = []
    for i, x0 in enumerate(x0s):
        new = i == 2
        out += column(AT, x0, i, f"嘉宾 {i + 1}", "new" if new else "mid",
                      strong=new and stage >= 2)
    if stage >= 2:
        out += compare(AT, x0s)
        for x, weight in zip(drops(x0s), share):
            out.append(mono(x + 8, BUS + 14, f"{weight:.1f}", 14, INK, "bold"))
    if stage >= 3:
        out += merge(AT, x0s, [1 + 6 * weight for weight in share])
        out += conclusion(AT, x0s, 2)
    left = x0s[0]
    for y, words in zip(CAPTION_Y, CAPTIONS[stage - 1]):
        out.append(text(left, y, words, 17, INK, "bold", "start"))
    if stage >= 3:
        formula = " + ".join(f"{weight:.1f}·v{i + 1}" for i, weight in enumerate(share))
        out.append(mono(left + 22, CAPTION_Y[1], f"c3 = {formula}", 15, INK, "bold"))
    return out


if __name__ == "__main__":
    for stage in (1, 2, 3):
        save(f"kv-party-read-{stage}", W, H, figure(stage))
