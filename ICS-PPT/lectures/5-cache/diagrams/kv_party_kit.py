"""Shared parts of the matchmaking-show figures (kv_party_*.py).

The story that opens part five: guests enter a matchmaking show one after another, and the
crew learns about a guest only from that guest's own introduction. Each guest has a
profile x, and from it a feature k (what kind of person they are), a value v (what they
can bring to another guest) and an interest q (whom they look for). After a guest introduces
themselves, the crew records q, k and v on a note and keeps it. The crew then reads every note
by the interest of the new guest, writes a conclusion c, and from the profile and the
conclusion invites the next guest from backstage. Guests do not read notes themselves.
A later page finds that the interest of a guest is used only when that guest enters, so
the note need hold only k and v. The data of the story lives here so every figure tells it
with the same words and numbers. All numbers are schematic.

A guest is drawn as a person glyph (head and shoulders) with the name below it. A thing a
guest holds is a card: a mono symbol (x1, k1, ...) followed by its content. A note is a
sheet of paper with a folded corner holding its cards one above the other. The notes of
kv_party_read.py and kv_party_enter.py hold q, k and v: the card q is Q_W wide, narrower
than k and v, and leaves a free channel on its right inside the note. The note of
kv_party_info.py (kv-party-note.svg) holds only k and v. Colours follow kv_kit.py: grey
for an ordinary card, orange for the guest who has just entered, blue for what is
unchanged, grey dashed for what is no longer used.
Importing this module draws nothing.
"""

from kv_kit import *
from svgkit import circle, mono_width, path

# guest i (0-based) -> its cards; guest 4 is B from the waiting list
PROFILE = ["教师，每周跑步", "编辑，爱读书", "工程师，爱摄影", "学生，爱跑步"]
FEATURE = ["爱跑步", "爱读书", "爱摄影", "爱跑步"]
INTEREST = ["爱读书的人", "爱运动的人", "爱户外活动的人", "爱摄影的人"]
VALUE = ["一起跑步", "推荐好书", "一起外出拍照", "一起跑马拉松"]

# ATTENTION[i] is the attention of guest i+1 to the notes of guests 1..i+1, each row sums to 1
ATTENTION = [[1.0], [0.8, 0.2], [0.6, 0.1, 0.3]]

# the waiting list: name, profile, score of the comparison; the highest score is invited
WAITING = [("A", "医生，爱读书", 1.1), ("B", "学生，爱跑步", 2.4), ("C", "会计，爱下棋", 0.4)]

CARD_H = 28
PERSON_H = 44
FOLD = 8                                   # the folded corner of a sheet of paper
SLIP_PAD, SLIP_GAP = 8, 6                  # inside a note: margin, gap between two cards
# the card q on a note: symbol and longest interest at size 14, with card()'s margins
Q_W = 8 + mono_width("q1", 14) + 8 + 14 * max(map(len, INTEREST)) + 8


def slip_height(rows=2):
    """Height of a note holding rows cards one above the other."""
    return rows * CARD_H + (rows - 1) * SLIP_GAP + 2 * SLIP_PAD


SLIP_H = slip_height(2)


def person(cx, top, kind="mid", name=None, size=15):
    """A head and shoulders centred on cx, PERSON_H tall; the name, if any, below it."""
    fill, stroke = KINDS[kind]
    head = 10
    out = [circle(cx, top + head, head, fill, stroke, 1.6)]
    y0, y1 = top + 2 * head + 4, top + PERSON_H
    d = (f"M {cx - 18:.1f} {y1:.1f} C {cx - 18:.1f} {y0:.1f} {cx + 18:.1f} {y0:.1f} "
         f"{cx + 18:.1f} {y1:.1f} Z")
    out.append(path(d, stroke, 1.6, fill=fill, head=False))
    if name:
        out.append(text(cx, y1 + size + 4, name, size, INK, "bold"))
    return out


def card(x, y, w, sym, content="", kind="mid", strong=False, size=14, h=CARD_H):
    """A card: the mono symbol, then its content; strong gives it the INK outline."""
    fill, stroke = KINDS[kind]
    dash = "5 4" if kind in ("empty", "gone") else None
    out = [rect(x, y, w, h, fill, INK if strong else stroke, rx=4,
                width=2.2 if strong else 1.4, dash=dash)]
    tone = MUTED if kind in ("empty", "gone") else INK
    base = y + h / 2 + size * 0.36
    out.append(mono(x + 8, base, sym, size, tone, "bold"))
    if content:
        out.append(text(x + 8 + mono_width(sym, size) + 8, base, content, size, tone,
                        anchor="start"))
    return out


def paper(x, y, w, h, kind="mid", strong=False):
    """A white sheet of paper with the top right corner folded, outlined in the colour of
    kind (dashed for empty and gone); strong gives it the INK outline."""
    _, stroke = KINDS[kind]
    color, width = (INK, 2.2) if strong else (stroke, 1.4)
    dash = "5 4" if kind in ("empty", "gone") else None
    d = (f"M {x:.1f} {y:.1f} L {x + w - FOLD:.1f} {y:.1f} L {x + w:.1f} {y + FOLD:.1f} "
         f"L {x + w:.1f} {y + h:.1f} L {x:.1f} {y + h:.1f} Z")
    fold = (f"M {x + w - FOLD:.1f} {y:.1f} L {x + w - FOLD:.1f} {y + FOLD:.1f} "
            f"L {x + w:.1f} {y + FOLD:.1f}")
    return [path(d, color, width, fill=WHITE, dash=dash, head=False),
            path(fold, color, 1.2, head=False)]


def slip(x, y, w, guest, kind="mid", strong=False, interest=None, q_strong=False):
    """The note guest (0-based) leaves on the stage: the feature k above the value v, both
    in the colour of kind, the paper outlined in that colour (INK if strong). Without
    interest the note holds only k and v and is SLIP_H tall. With interest (a kind) the
    interest q stands above k, Q_W wide at the left of the note and in the colour of
    interest (INK outline if q_strong), and the note is slip_height(3) tall."""
    rows = slip_rows(y, 3 if interest else 2)
    out = paper(x, y, w, slip_height(len(rows)), kind, strong)
    cx, cw = x + SLIP_PAD, w - 2 * SLIP_PAD
    if interest:
        out += card(cx, rows[0], Q_W, f"q{guest + 1}", INTEREST[guest], interest, q_strong)
    out += card(cx, rows[-2], cw, f"k{guest + 1}", FEATURE[guest], kind)
    out += card(cx, rows[-1], cw, f"v{guest + 1}", VALUE[guest], kind)
    return out


def slip_rows(y, rows=2):
    """Tops of the cards inside a note whose top is y, from the top: k and v, or q, k
    and v when rows is 3."""
    return [y + SLIP_PAD + i * (CARD_H + SLIP_GAP) for i in range(rows)]
