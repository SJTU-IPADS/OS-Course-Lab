"""Shared parts of the figures of this lecture (the kvcache lab).

A figure is read without the code at hand: it names no option, make target
or workload file, and gives the setting of every number it shows.

Every figure draws the same entities in the same colours: a cached row or
block of the KV cache is light blue, what the current step writes or changes
is orange, what it reads is green, a reserved block is white with a dashed
blue outline, a block not used yet grey. A repeated computation is red.
Other vectors are grey. A cell located in the current step (a row whose
address is taken, the block and the row that kv-locate looks for) keeps its
colour and gets a dark outline: orange is kept for writes and changes.

A function of src/kvcache.c that the student writes is a dark pill with
white mono text. compute, which is given, is a grey pill. A framework
function is a white box with a grey outline and dark mono text.
"""

import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, DIM, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE,  # noqa: F401
                    FILL_RED, GIVEN, GREEN, INK, LINE, MONO, MONO_EM, MUTED,
                    ORANGE, RED, WHITE, FONT, arrow, brace, elbow, esc, line, mono,
                    path, rect, save, text, vbrace)

MATH = "STIX Two Text, Times New Roman, Times, serif"
KV = ("k", "v")

# kind -> (fill, stroke)
KINDS = {
    "new": (FILL_ORANGE, ORANGE),
    "cache": (FILL_BLUE, BLUE),
    "reuse": (FILL_GREEN, GREEN),
    "waste": (FILL_RED, RED),
    "empty": (WHITE, LINE),
    "reserved": (WHITE, BLUE),
    "unused": (FILL_GREY, LINE),          # a block from capacity on
    "addr": (WHITE, INK),
    "output": (FILL_GREY, INK),          # a token that is output only
    "mid": (FILL_GREY, MUTED),
}
DASHED = ("empty", "reserved", "output")


def width_of(s, size, font=FONT):
    if font == MONO:
        return len(s) * size * MONO_EM
    return sum(1.0 if ord(c) > 0x2e80 else 0.55 for c in s) * size


def tagged(key, shapes):
    """Shapes that show one datum, named so that a program can read it back."""
    return [f'<g data-key="{key}">'] + list(shapes) + ["</g>"]


def faded(shapes, opacity=DIM):
    return [f'<g opacity="{opacity}">'] + list(shapes) + ["</g>"]


# ------------------------------------------------------------ rich text

def _pieces(markup):
    """`mono` and $math$ inside text: (string, font, italic, subscript)."""
    for part in re.split(r"(`[^`]*`|\$[^$]*\$)", markup):
        if part.startswith("`"):
            yield part[1:-1], MONO, False, False
        elif part.startswith("$"):
            for atom in re.findall(r"_\{[^}]*\}|_.|[A-Za-z]+|[^A-Za-z_]+", part[1:-1]):
                sub = atom.startswith("_")
                word = atom.lstrip("_").strip("{}") if sub else atom
                yield word, MATH, len(word) == 1 and word.isalpha(), sub
        elif part:
            yield part, FONT, False, False


def rich_width(markup, size):
    total = 0.0
    for word, font, _, sub in _pieces(markup):
        if font == MATH:
            total += len(word) * (0.40 if sub else 0.54) * size
        else:
            total += width_of(word, size if font != MONO else size - 1, font)
    return total


def rich(x, y, markup, size=16, fill=INK, weight="normal", anchor="start", room=None):
    if room is not None and rich_width(markup, size) > room:
        raise ValueError(f"wider than {room}: {markup}")
    spans, shift = [], 0.0
    for word, font, italic, sub in _pieces(markup):
        target = size * 0.22 if sub else 0.0
        attrs = f' dy="{target - shift:.1f}"' if target != shift else ""
        shift = target
        if font == MATH:
            attrs += f' font-family="{MATH}" font-size="{size * (0.8 if sub else 1.12):.1f}"'
            attrs += ' font-style="italic"' if italic else ""
        elif font == MONO:
            attrs += f' font-family="{MONO}" font-size="{size - 1}"'
        spans.append(f"<tspan{attrs}>{esc(word)}</tspan>")
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{FONT}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}">{"".join(spans)}</text>')


# ------------------------------------------------------------ cells and legends

def box_of(x, y, w, h, kind, width=1.4, rx=2):
    fill, stroke = KINDS[kind]
    return rect(x, y, w, h, fill, stroke, rx=rx, width=width,
                dash="5 4" if kind in DASHED else None)


def cell(x, y, w, h, label="", kind="cache", size=14, width=1.6, weight="bold", rx=4,
         tone=None):
    """One block in the colour of its kind, with an optional mono label."""
    out = [box_of(x, y, w, h, kind, width, rx)]
    if label:
        tone = tone or (MUTED if kind in ("empty", "unused", "reserved") else INK)
        out.append(mono(x + w / 2, y + h / 2 + size * 0.35, label, size, tone, weight,
                        anchor="middle"))
    return out


def legend(x, y, items, size=15, gap=22):
    """A row of (kind, label) swatches starting at x. A kind "fn" is a pill of the
    student, "given" a pill of a given function, "frame" a framework function."""
    out = []
    for kind, label in items:
        if kind in ("fn", "given"):
            fill = INK if kind == "fn" else GIVEN
            out.append(rect(x, y - 13, 24, 17, fill, fill, rx=8.5, width=1.2))
        elif kind == "frame":
            out.append(rect(x, y - 13, 24, 17, WHITE, MUTED, rx=3, width=1.2))
        else:
            out.append(box_of(x, y - 13, 24, 17, kind, 2.4 if kind == "addr" else 1.4, 3))
        out.append(text(x + 32, y + 1, label, size, MUTED, anchor="start"))
        x += 32 + width_of(label, size) + gap
    return out


def legend_width(items, size=15, gap=22):
    return sum(32 + width_of(label, size) + gap for _, label in items) - gap


def centred_legend(cx, y, items, size=15, gap=22):
    return legend(cx - legend_width(items, size, gap) / 2, y, items, size, gap)


def caption(cx, y, s, size=18):
    return text(cx, y, s, size, INK, "bold")


# ------------------------------------------------------------ functions

def fn_width(name, size=15):
    return width_of(name, size, MONO) + 18


def frame_fn_width(name, size=15):
    return width_of(name, size, MONO) + 16


def fn(x, y, name, size=15, anchor="start", h=None, fill=INK):
    """A function of src/kvcache.c: a pill, text baseline-centred on y.

    Returns (shapes, x0, x1) so arrows can meet its ends."""
    h = h or size + 11
    w = fn_width(name, size)
    x0 = {"start": x, "middle": x - w / 2, "end": x - w}[anchor]
    out = [rect(x0, y - h / 2, w, h, fill, fill, rx=h / 2, width=1.2),
           mono(x0 + w / 2, y + size * 0.35, name, size, WHITE, "bold", anchor="middle")]
    return out, x0, x0 + w


def given_fn(x, y, name, size=15, anchor="start", h=None):
    """compute, written out in src/kvcache.c: a grey pill."""
    return fn(x, y, name, size, anchor, h, GIVEN)


def frame_fn(x, y, name, size=15, anchor="start", h=None, fill=WHITE):
    """A framework function: a white box with a grey outline."""
    h = h or size + 11
    w = frame_fn_width(name, size)
    x0 = {"start": x, "middle": x - w / 2, "end": x - w}[anchor]
    out = [rect(x0, y - h / 2, w, h, fill, MUTED, rx=4, width=1.3),
           mono(x0 + w / 2, y + size * 0.35, name, size, INK, "normal", anchor="middle")]
    return out, x0, x0 + w


SHAPE = {"fn": (fn, fn_width), "given": (given_fn, fn_width), "fw": (frame_fn, frame_fn_width)}


def _line_width(items, size):
    x = 0
    for item in items:
        if item == ">":
            x += 26
        elif item[0] == "text":
            x += rich_width(item[1], size) + 7
        else:
            x += SHAPE[item[0]][1](item[1], size) + 5
    return x


def call_line(x, y, items, right, size=16):
    """Functions, ">" arrows and words on one line, at the largest size that fits."""
    while x + _line_width(items, size) > right:
        size -= 1
        if size < 14:
            raise ValueError(f"calls too wide: {items}")
    out = []
    for item in items:
        if item == ">":
            out.append(arrow(x, y, x + 21, y, MUTED, 1.7))
            x += 26
        elif item[0] == "text":
            out.append(rich(x + 1, y + size * 0.36, item[1], size, INK))
            x += rich_width(item[1], size) + 7
        else:
            shapes, _, x1 = SHAPE[item[0]][0](x, y, item[1], size, h=min(size + 10, 26))
            out += shapes
            x = x1 + 5
    return out
