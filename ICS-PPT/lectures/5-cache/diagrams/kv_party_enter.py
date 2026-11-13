#!/usr/bin/env python3
"""B enters as guest 4 and the crew reads its notes by the interest of guest 4, in three
frames.

The columns are those of kv_party_read.py, one per guest: the glyph with the name below
it, and below them the note the crew recorded for the guest, holding from the top the
interest q, the feature k and the value v. The card q is narrower than k and v and leaves
a channel on its right inside the note, through which every line enters and leaves the
note. No number stands between the bus and the notes, so the notes start closer to the
bus than in kv_party_read.py.
Guests 1 to 3 are grey. Guest 4 is B, invited from the waiting list (index 3 of the
lists in kv_party_kit.py), and is orange, glyph and note.
Frame 1: the four columns only.
Frame 2: q4 on the note of guest 4 gets the strong outline. A line rises from the top
         edge of q4 to the bus, the bus drops through the channel of each note onto
         k1..k4, and lines from the bottom of v1..v4 merge into the conclusion c4
         (orange) below guest 4.
Frame 3: on the notes of guests 1 to 3 the paper, k and v are unchanged (blue) and the
         interests q1..q3 are no longer used (grey dashed). q4 keeps the strong outline.
         A legend left of c4 names the three colours.
The caption is one centred line under the row of the conclusion card. A later frame only
adds elements or changes a colour, nothing moves. The data and the shapes come from
kv_party_kit.py, the column layout from kv_party_read.py.
Run it to refresh ../assets/kv-party-enter-1.svg, kv-party-enter-2.svg and
kv-party-enter-3.svg.
"""

from kv_party_kit import *
from kv_party_read import (W, column, columns, compare, conclusion, conclusion_x, merge,
                           rows)

AT = rows(12)                              # nothing stands between the bus and the notes
H = AT.c + 53
CAPTION_Y = H - 6

CAPTIONS = [
    "① B 作为嘉宾 4 入场，节目组记下嘉宾 4 的纸条，此前已有嘉宾 1 至 3 的纸条",
    "② 节目组用兴趣 q4 阅读 4 张纸条，写成结论 c4",
    "③ 嘉宾 1 至 3 的纸条与之前相同，纸条上的兴趣不再使用",
]
KEYS = [("new", "新入场的嘉宾"), ("cache", "与之前相同"), ("gone", "不再使用")]


def figure(stage):
    x0s = columns(4)
    out = []
    for i, x0 in enumerate(x0s):
        if i == 3:
            out += column(AT, x0, i, "嘉宾 4（B）", "new", strong=stage >= 2)
        elif stage == 3:
            out += column(AT, x0, i, f"嘉宾 {i + 1}", "mid", interest="gone", note="cache")
        else:
            out += column(AT, x0, i, f"嘉宾 {i + 1}")
    if stage >= 2:
        out += compare(AT, x0s)
        out += merge(AT, x0s)
        out += conclusion(AT, x0s, 3)
    if stage == 3:
        x = conclusion_x(x0s) - 40 - legend_width(KEYS)
        out += legend(x, AT.c + CARD_H / 2 + 6, KEYS)
    out.append(text(W / 2, CAPTION_Y, CAPTIONS[stage - 1], 17, INK, "bold"))
    return out


if __name__ == "__main__":
    for stage in (1, 2, 3):
        save(f"kv-party-enter-{stage}", W, H, figure(stage))
