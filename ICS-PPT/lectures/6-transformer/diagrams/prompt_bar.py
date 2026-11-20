"""A prompt drawn as a bar of segments, for the section on prefix caching.

request-recompute.py, prefix-tree.py and prompt-order.py draw prompts as
rows of segments: the system prompt, a question, an answer, the tool
descriptions. One kind of segment
has one width (WIDTH) in every figure, the system prompt the longest. A
segment named by its own text, a user's question written out, is as wide
as its label plus PAD on each side. The
segments of a prompt sit GAP apart from its left end, so the same offset
in two prompts is the same x, and the nodes of the prefix tree, laid out
the same way, stand under the segments of a request. The current time
written into a segment is a small white chip inside it (stamp).

Each treatment has one meaning in all three figures:
- grey (data): input whose K, V this request computes;
- green: an answer the model generates in this request, as in chat.py;
- hatched in INK, the strokes of recompute.py: content an earlier request
  already computed and this request computes again; the label sits on a
  grey plate over the strokes;
- a band under the segment, a key strip (blue) over a value strip
  (purple): K, V reused from the cache; a figure labels the run 「复用」;
- red outline: the first segment, or time chip, that differs from the
  cached prompt;
- dashed outline on white: a node evicted from the cache.
Not a figure: render.sh skips it.
"""
from tk import *
from kv_grid import tint

NAME_X, X0 = 30, 180               # left end of the row names, of the prompts
SEG_H, GAP, SIZE = 40, 12, 18      # segment height, gap between segments, label size
STRIP, BAND_UP = 6, 4              # height of one strip of the band, gap above it
BAND_H = BAND_UP + 2 * STRIP       # segment bottom to band bottom
HATCH_GAP = 6                      # hatching: distance between two strokes
PAD = 12                           # label to edge of a segment named by its text

STAMP_SIZE, STAMP_PAD, STAMP_H = 16, 6, 26     # the time chip inside a segment

WIDTH = {"系统提示词": 240, "问题": 104, "回答": 150, "工具说明": 170}


def seg_width(name):
    """Width of a segment; the kind is the first word of its name. A name
    that is no kind is the segment's text, and the segment fits it."""
    kind = name.split()[0]
    return WIDTH[kind] if kind in WIDTH else width(name, SIZE) + 2 * PAD


def hatch(left, top, w, h, gap=HATCH_GAP):
    """Strokes at 45 degrees across the rectangle, clipped to it (recompute.py)."""
    out = []
    c = left + top + gap / 2
    while c < left + w + top + h:
        x0, x1 = max(left, c - top - h), min(left + w, c - top)
        if x1 - x0 > 3:                 # no stub in a corner
            out.append(line(x0, c - x0, x1, c - x1, INK, 1.2))
        c += gap
    return out


def segment(x, y, name, gen=False, hatched=False, diff=False, evicted=False):
    """One segment with its left end at x. Returns (elements, width)."""
    w = seg_width(name)
    stroke, fill, ink, dash = DATA, FILL_DATA, INK, None
    if gen:
        stroke, fill, ink = UPD, FILL_UPD, UPD
    if evicted:
        stroke, fill, ink, dash = LINE, WHITE, MUTED, "5 4"
    sw = 1.6
    if diff:
        stroke, sw = RED, 2.4
    out = [rect(x, y, w, SEG_H, fill, stroke, rx=4, width=sw, dash=dash)]
    if hatched:
        out += hatch(x + 1, y + 1, w - 2, SEG_H - 2)
        pw = width(name, SIZE) + 14
        out.append(rect(x + (w - pw) / 2, y + SEG_H / 2 - 13, pw, 26, fill, "none",
                        rx=3, width=0))
    out.append(text(x + w / 2, y + SEG_H / 2 + SIZE * 0.36, name, SIZE, ink))
    return out, w


def stamp(x, y, s, diff=False):
    """The current time written inside a segment whose top is at y: a white
    chip with its left end at x, red-outlined when it is the first
    difference. Returns (elements, width)."""
    w = width(s, STAMP_SIZE) + 2 * STAMP_PAD
    top = y + (SEG_H - STAMP_H) / 2
    stroke, sw = (RED, 2.4) if diff else (DATA, 1.4)
    return [rect(x, top, w, STAMP_H, WHITE, stroke, rx=3, width=sw),
            text(x + w / 2, top + STAMP_H / 2 + STAMP_SIZE * 0.36, s, STAMP_SIZE,
                 INK)], w


def band(x, y, w):
    """K, V reused from the cache: a key strip over a value strip, under a
    segment whose bottom is at y."""
    top = y + BAND_UP
    return [rect(x, top, w, STRIP, tint(K, 0.55), K, rx=1, width=1),
            rect(x, top + STRIP, w, STRIP, tint(V, 0.55), V, rx=1, width=1)]


def bar(x, y, names, gen=(), hatched=(), reused=(), diff=(), evicted=()):
    """A prompt from x; the keyword sets hold indices of segments. Returns
    (elements, [(left, right)] of every segment)."""
    out, spans = [], []
    for i, name in enumerate(names):
        els, w = segment(x, y, name, i in gen, i in hatched, i in diff, i in evicted)
        out += els
        if i in reused:
            out += band(x, y + SEG_H, w)
        spans.append((x, x + w))
        x += w + GAP
    return out, spans


def row_name(mid, title, sub=None, x=NAME_X):
    """A row's name level with its segments, an optional note below."""
    out = [label(x, mid + 7, title, 19, INK, "bold", "start")]
    if sub:
        out.append(note(x, mid + 30, sub, 16, "start"))
    return out


def under(left, right, y, s, fill=INK, size=17):
    """A short label centred under the run [left, right], baseline y."""
    return [label((left + right) / 2, y, s, size, fill, "bold")]


def hatch_key(x, y, s):
    """Legend entry for the hatching: a small hatched segment, then s.
    y is the baseline. Returns (elements, width)."""
    out = [rect(x, y - 16, 34, 22, FILL_DATA, DATA, rx=3, width=1.4)]
    out += hatch(x + 1, y - 15, 32, 20)
    out.append(note(x + 44, y, s, 17, "start"))
    return out, 44 + width(s, 17)


def gen_key(x, y, s):
    """Legend entry for a generated answer: a small green segment, then s."""
    out = [rect(x, y - 16, 34, 22, FILL_UPD, UPD, rx=3, width=1.4),
           note(x + 44, y, s, 17, "start")]
    return out, 44 + width(s, 17)
