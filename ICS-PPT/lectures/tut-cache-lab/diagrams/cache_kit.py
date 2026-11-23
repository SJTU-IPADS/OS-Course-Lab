"""Shared parts of the figures of this lecture (lab 2 of mini-cpu, the cache).

Every figure draws the same entities in the same colours, those of the cache
lecture: the CPU (the interpreter) is a white box with a dark outline, a line
of the cache is light blue with a blue outline, DRAM is green, a hit is
green, the cost of a miss (a line moved to or from DRAM) is red. The three
fields of an address are tag purple, set index blue, offset orange. A dirty
line carries a red 1 in its dirty cell: it costs a transfer when it leaves.

A function of cache.c, the file the student writes, is a dark pill with
white mono text, placed where the function acts; cache_init, written out in
the skeleton, is a grey pill. A framework function is a white box with a
grey outline and dark mono text. A file is a panel headed by its path.
cache_chain.py also draws the hardware (frame/cpu/, the interpreter and the
cache) and the software (frame/os/) of the framework as two grey regions.

The figures print no number measured by a run: the states of the cache and
the counters of cache-hit and cache-miss are set for the example. The one
measured number of the deck, in the notes of cache-funcs-hit, comes from
the reference cache.c in build/scratch-cachedeck/mini-cpu (LAB=2); the
opening comment of pages.py describes the build.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import svgkit
from svgkit import _marker
from svgkit import (BLUE, FADE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE,  # noqa: F401
                    FILL_PURPLE, FILL_RED, GIVEN, GREEN, INK, LINE, MONO, MONO_EM, MUTED,
                    ORANGE, PURPLE, RED, WHITE, FONT, arrow, brace, elbow, line, mono, path,
                    rect, save, text, vbrace, circle)

TAG, FILL_TAG = PURPLE, FILL_PURPLE
IDX, FILL_IDX = BLUE, FILL_BLUE
OFF, FILL_OFF = ORANGE, FILL_ORANGE
FIELD = {"tag": (FILL_TAG, TAG), "idx": (FILL_IDX, IDX), "off": (FILL_OFF, OFF)}

LINE_FILL, LINE_EDGE = FILL_BLUE, BLUE          # a line of the cache
MEM_FILL, MEM_EDGE = FILL_GREEN, GREEN          # DRAM


def save(name, w, h, body, defs=()):
    """svgkit.save plus an arrow head in purple, the colour of the tag."""
    svgkit.save(name, w, h, body, [_marker(PURPLE)] + list(defs))


def width_of(s, size, font=FONT):
    if font == MONO:
        return len(s) * size * MONO_EM
    return sum(1.0 if ord(c) > 0x2e80 else 0.55 for c in s) * size


def _pill(x, y, name, size, anchor, h, fill, edge, tone, weight, rx):
    h = h or size + 11
    w = width_of(name, size, MONO) + 18
    x0 = {"start": x, "middle": x - w / 2, "end": x - w}[anchor]
    out = [rect(x0, y - h / 2, w, h, fill, edge, rx=rx if rx is not None else h / 2,
                width=1.3),
           mono(x0 + w / 2, y + size * 0.35, name, size, tone, weight, anchor="middle")]
    return out, x0, x0 + w


def fn(x, y, name, size=15, anchor="start", h=None):
    """A function of cache.c the student writes: a dark pill, centred on y.

    Returns (shapes, x0, x1) so arrows can meet its ends."""
    return _pill(x, y, name, size, anchor, h, INK, INK, WHITE, "bold", None)


def given_fn(x, y, name, size=15, anchor="start", h=None):
    """cache_init, written out in the skeleton: a grey pill."""
    return _pill(x, y, name, size, anchor, h, GIVEN, GIVEN, WHITE, "bold", None)


def frame_fn(x, y, name, size=15, anchor="start", h=None, fill=WHITE):
    """A function of the framework: a white box with a grey outline."""
    h = h or size + 11
    w = width_of(name, size, MONO) + 16
    x0 = {"start": x, "middle": x - w / 2, "end": x - w}[anchor]
    out = [rect(x0, y - h / 2, w, h, fill, MUTED, rx=4, width=1.3),
           mono(x0 + w / 2, y + size * 0.35, name, size, INK, "normal", anchor="middle")]
    return out, x0, x0 + w


def pill_width(name, size=15):
    return width_of(name, size, MONO) + 18


def chip(x, y, label, kind="frame", size=13):
    """A small tag naming whose file a panel is: 框架 or TODO, centred on x."""
    w = width_of(label, size) + 14
    if kind == "student":
        fill, edge, tone = INK, INK, WHITE
    elif kind == "lab1":
        fill, edge, tone = WHITE, INK, INK
    else:
        fill, edge, tone = FILL_GREY, MUTED, MUTED
    return [rect(x - w / 2, y - 10, w, 20, fill, edge, rx=10, width=1.1),
            text(x, y + size * 0.36, label, size, tone, "bold")]


def panel(x, y, w, h, path_, kind="frame", title=None, chip_label=None, title_size=18):
    """A file: a rounded box headed by its path in mono, an optional bold title.

    kind is "frame" (grey outline), "student" (dark, thick outline), "lab1"
    (dark outline) or "mem" (green). Returns the shapes."""
    edge, fill, width = {"frame": (MUTED, WHITE, 1.6), "student": (INK, WHITE, 2.6),
                         "lab1": (INK, WHITE, 1.8), "mem": (MEM_EDGE, WHITE, 2.0)}[kind]
    out = [rect(x, y, w, h, fill, edge, rx=8, width=width)]
    out.append(mono(x + w / 2, y + 30, path_, 14, MUTED if kind != "mem" else MEM_EDGE,
                    "normal", anchor="middle"))
    if title:
        out.append(text(x + w / 2, y + 56, title, title_size,
                        MEM_EDGE if kind == "mem" else INK, "bold"))
    if chip_label:
        ck = {"frame": "frame", "mem": "frame", "student": "student", "lab1": "lab1"}[kind]
        out += chip(x + w / 2, y, chip_label, ck)
    return out


def badge(cx, cy, label, kind, size=16, w=None):
    """A result tag: kind is "hit" (green) or "miss" (red)."""
    col, fill = (GREEN, FILL_GREEN) if kind == "hit" else (RED, FILL_RED)
    w = w or width_of(label, size) + 24
    return [rect(cx - w / 2, cy - 14, w, 28, fill, col, rx=14, width=1.6),
            text(cx, cy + size * 0.36, label, size, col, "bold")]


# ------------------------------------------------------------ cache lines

class LineGeo:
    """Cell widths of a drawn cache_line: valid | dirty | tag | last_use | data."""

    def __init__(self, vw=40, dw=40, tw=64, uw=78, dataw=320, h=34, size=16, cells=16):
        self.vw, self.dw, self.tw, self.uw, self.dataw = vw, dw, tw, uw, dataw
        self.h, self.size, self.cells = h, size, cells

    @property
    def width(self):
        return self.vw + self.dw + self.tw + self.uw + self.dataw

    def x_of(self, field, x):
        """Left edge of a field of the line that starts at x."""
        order = ["valid", "dirty", "tag", "last_use", "data"]
        widths = [self.vw, self.dw, self.tw, self.uw, self.dataw]
        for name, w in zip(order, widths):
            if name == field:
                return x
            x += w
        raise KeyError(field)

    def w_of(self, field):
        return {"valid": self.vw, "dirty": self.dw, "tag": self.tw, "last_use": self.uw,
                "data": self.dataw}[field]


def line_header(x, y, geo, size=14, names=None):
    """The field names above a column of lines, baseline at y."""
    names = names or {"valid": "valid", "dirty": "dirty", "tag": "tag",
                      "last_use": "last_use", "data": "data[0..63]"}
    out = []
    for field in ("valid", "dirty", "tag", "last_use", "data"):
        if field not in names:
            continue
        cx = geo.x_of(field, x) + geo.w_of(field) / 2
        tone = TAG if field == "tag" else MUTED
        out.append(mono(cx, y, names[field], size, tone, "bold" if field == "tag" else "normal",
                        anchor="middle"))
    return out


def cache_line(x, y, geo, valid=1, dirty=0, tag="", last_use="", dim=False,
               halo=None, tag_halo=None, use_halo=None, dirty_halo=None, span=None,
               span_color=ORANGE, data_label=None):
    """One cache_line. span = (first, last) byte of data to wash in span_color."""
    out = []
    live = valid and not dim
    fill = LINE_FILL if valid else WHITE
    edge = LINE_EDGE if valid else LINE
    if dim:
        fill, edge = (FILL_GREY if valid else WHITE), LINE
    cy = y + geo.h / 2 + geo.size * 0.36
    tone = INK if live else MUTED
    fields = [("valid", str(valid)), ("dirty", str(dirty) if valid else "0"),
              ("tag", tag if valid else ""), ("last_use", str(last_use) if valid else "")]
    for field, value in fields:
        fx = geo.x_of(field, x)
        out.append(rect(fx, y, geo.w_of(field), geo.h, fill, edge, rx=0, width=1.2))
        if value != "":
            col = tone
            weight = "normal"
            if field == "tag" and live:
                col, weight = TAG, "bold"
            if field == "dirty" and live and value == "1":
                col, weight = RED, "bold"
            out.append(mono(fx + geo.w_of(field) / 2, cy, value, geo.size, col, weight,
                            anchor="middle"))
    dx = geo.x_of("data", x)
    cw = geo.dataw / geo.cells
    for k in range(geo.cells):
        out.append(rect(dx + k * cw, y, cw, geo.h, fill, edge, rx=0, width=0.8))
    if span and valid:
        first, last = span
        sx = dx + first * geo.dataw / 64
        sw = (last - first + 1) * geo.dataw / 64
        out.append(rect(sx, y + 3, sw, geo.h - 6, FILL_ORANGE, span_color, rx=2, width=1.6))
    if data_label and valid:
        out.append(text(dx + geo.dataw / 2, cy, data_label, geo.size - 2, tone))
    out.append(rect(x, y, geo.width, geo.h, "none", edge, rx=0, width=1.6))
    if halo:
        out.append(rect(x - 4, y - 4, geo.width + 8, geo.h + 8, "none", halo, rx=5, width=3))
    if tag_halo:
        tx = geo.x_of("tag", x)
        out.append(rect(tx - 2, y - 2, geo.tw + 4, geo.h + 4, "none", tag_halo, rx=3, width=3))
    if use_halo:
        ux = geo.x_of("last_use", x)
        out.append(rect(ux - 2, y - 2, geo.uw + 4, geo.h + 4, "none", use_halo, rx=3, width=3))
    if dirty_halo:
        ddx = geo.x_of("dirty", x)
        out.append(rect(ddx - 2, y - 2, geo.dw + 4, geo.h + 4, "none", dirty_halo, rx=3,
                        width=3))
    return out


def addr_fields(x, y, widths, labels, h=40, size=17, font=MONO, dim=()):
    """An address as three boxes tag | set | offset of the given widths."""
    out, boxes = [], {}
    cx = x
    for name, w, label in zip(("tag", "idx", "off"), widths, labels):
        fill, edge = FIELD[name]
        if name in dim:
            fill, edge = WHITE, LINE
        out.append(rect(cx, y, w, h, fill, edge, rx=3, width=2))
        out.append(text(cx + w / 2, y + h / 2 + size * 0.36, label, size,
                        MUTED if name in dim else edge, "bold", font=font))
        boxes[name] = (cx, cx + w)
        cx += w
    return out, boxes


def dram_lines(x, y, w, rows, h=20, gap=4, marks=None, labels=None, size=13):
    """DRAM drawn as a stack of 64-byte lines; marks = {row: colour} frames a row."""
    out = []
    marks = marks or {}
    labels = labels or {}
    for r in range(rows):
        ry = y + r * (h + gap)
        out.append(rect(x, ry, w, h, MEM_FILL, MEM_EDGE, rx=2, width=1.2))
        if r in labels:
            out.append(mono(x + w / 2, ry + h / 2 + size * 0.36, labels[r], size, INK,
                            anchor="middle"))
        if r in marks:
            out.append(rect(x - 3, ry - 3, w + 6, h + 6, "none", marks[r], rx=4, width=2.6))
    return out


def legend(x, y, items, size=14, gap=22):
    """A row of (kind, label): kind is fn, given, frame, line, mem, hit, miss."""
    out = []
    for kind, label in items:
        if kind == "fn":
            out.append(rect(x, y - 12, 26, 17, INK, INK, rx=8.5, width=1.2))
        elif kind == "given":
            out.append(rect(x, y - 12, 26, 17, GIVEN, GIVEN, rx=8.5, width=1.2))
        elif kind == "frame":
            out.append(rect(x, y - 12, 26, 17, WHITE, MUTED, rx=3, width=1.2))
        elif kind == "line":
            out.append(rect(x, y - 12, 26, 17, LINE_FILL, LINE_EDGE, rx=2, width=1.4))
        elif kind == "mem":
            out.append(rect(x, y - 12, 26, 17, MEM_FILL, MEM_EDGE, rx=2, width=1.4))
        elif kind == "hit":
            out.append(rect(x, y - 12, 26, 17, FILL_GREEN, GREEN, rx=8.5, width=1.4))
        elif kind == "miss":
            out.append(rect(x, y - 12, 26, 17, FILL_RED, RED, rx=8.5, width=1.4))
        out.append(text(x + 34, y + 1, label, size, MUTED, anchor="start"))
        x += 34 + width_of(label, size) + gap
    return out


def legend_width(items, size=14, gap=22):
    return sum(34 + width_of(label, size) + gap for _, label in items) - gap
