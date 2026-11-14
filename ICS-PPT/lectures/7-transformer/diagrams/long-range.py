#!/usr/bin/env python3
"""long-range: the last vector has to gather the whole context.

1: most of a mystery novel as faint lines of text, from its opening line to
   the last words "Therefore, the murderer was"; a few positions far back
   (orange marks) carry information into the last token, was.
2: the last vector (green, after all the updates) gives the prediction of
   the next word; the top candidate is Derek, the video's answer.
"""
from tk import *

H = 330
BX, BY, BW, BH = 26, 14, 640, 300        # the page
LX0, LX1 = BX + 24, BX + BW - 24          # text column
LH = 26                                    # line pitch
FAINT = "#dfe6eb"
# lines below the opening line: (length as a share of the column) or "gap"
LINES = [1, 1, .94, 1, .58, "gap", 1, .86]
CLUES = [(1, .22), (3, .70), (4, .12)]    # (line index, position on the line)


def mix(c1, c2, t):
    a = [int(c1[i:i + 2], 16) for i in (1, 3, 5)]
    b_ = [int(c2[i:i + 2], 16) for i in (1, 3, 5)]
    return "#%02x%02x%02x" % tuple(round(x + (y - x) * t) for x, y in zip(a, b_))


def bar(x0, x1, y, fill=FAINT, h=7):
    return rect(x0, y - h / 2, x1 - x0, h, fill, "none", rx=3, width=0)


def line_y(i):
    return BY + 34 + (i + 1) * LH


LAST_Y = line_y(len(LINES)) + 4


def frame(n):
    b = []
    # a stack of pages
    for k in (2, 1):
        b.append(rect(BX + 5 * k, BY + 5 * k, BW, BH, WHITE, LINE, rx=6, width=1.2))
    b.append(rect(BX, BY, BW, BH, WHITE, LINE, rx=6, width=1.4))

    first = "It was a dark and stormy night."
    b.append(text(LX0, BY + 40, first, 18, INK, anchor="start"))
    b.append(bar(LX0 + width(first, 18) + 10, LX1, BY + 34))
    for i, share in enumerate(LINES):
        y = line_y(i)
        if share == "gap":
            b.append(text(LX0 + 4, y + 7, "⋯", 28, MUTED, anchor="start"))
            continue
        b.append(bar(LX0, LX0 + (LX1 - LX0) * share, y))

    # the last line ends with the words before the prediction
    words = ["Therefore,", "the", "murderer", "was"]
    rw = row_width(words, 18, gap=8, pad=8)
    els, spans = chip_row(LX1 - rw, LAST_Y - 17, words, size=18, gap=8, h=34, pad=8)
    b.append(bar(LX0, LX1 - rw - 12, LAST_Y))
    b += els
    was_l, was_c, was_r = spans[-1]

    # clues far back, and what they pass to the last token
    for i, at in CLUES:
        x = LX0 + (LX1 - LX0) * at
        y = line_y(i)
        d = f"M {x + 17:.1f} {y + 5:.1f} Q {was_c + 40:.1f} {y + 40:.1f} {was_c + 4:.1f} {LAST_Y - 22:.1f}"
        b.append(path(d, WHITE, 6, head=False))
        b.append(path(d, ORANGE, 2))
    for i, at in CLUES:
        x = LX0 + (LX1 - LX0) * at
        b.append(bar(x, x + 34, line_y(i), ORANGE, 8))

    if n >= 2:
        cx, cy = BX + BW + 46, BY + 80           # the last vector
        b.append(elbow([(was_r + 6, LAST_Y), (cx + 15, LAST_Y), (cx + 15, cy + 150)], INK, 2))
        stroke, fill = GREEN, FILL_GREEN
        for k in range(7):
            y = cy + k * 20
            f = WHITE if k == 5 else mix(fill, stroke, 0.12 + 0.25 * (pseudo("was", k, 0) + 1))
            b.append(rect(cx, y, 30, 20, f, WHITE, rx=0, width=1))
            if k == 5:
                b.append(text(cx + 15, y + 15, "⋮", 16, GREEN))
        b.append(rect(cx - 3, cy - 3, 36, 146, "none", GREEN, rx=4, width=1.8))
        b.append(arrow(cx + 42, cy + 70, cx + 82, cy + 70, INK, 2))

        bx = cx + 166                           # bars start
        b.append(text(bx + 30, cy - 20, "下一个词的预测", 19, INK, "bold"))
        lengths = [150, 64, 46, 34, 24, 16]
        for k, ln in enumerate(lengths):
            y = cy - 2 + k * 24
            top = k == 0
            b.append(rect(bx, y, ln, 18, FILL_GREEN if top else FILL_GREY,
                          GREEN if top else LINE, rx=2, width=1.4))
            if top:
                b.append(text(bx - 10, y + 15, "Derek", 19, GREEN, "bold", anchor="end"))
        b.append(text(bx + 8, cy - 2 + len(lengths) * 24 + 12, "⋮", 18, MUTED))
    save(f"long-range-{n}", b, H)


for n in (1, 2):
    frame(n)
