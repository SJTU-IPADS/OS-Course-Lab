"""Figure toolkit of this lecture, on top of svgkit's primitives.

Style rules every figure follows (one deck, one style):

- Canvas: 1040 px wide (`W`), drawn at the size it is shown on the slide;
  height 240..420 px. A font size here is a size on the 1280x720 slide.
- Labels: Chinese, `FONT`, 20 px for the main labels, 17 px for secondary
  ones, 15 px at the least. English words of the examples stay English.
  Symbols use `math()`: italic serif letters, real subscripts.
- Colour code, the same entity in the same colour on every page:
    data (embeddings, activations, text)   grey   INK on FILL_GREY
    weights (matrix entries)               blue positive / red negative
    query                                  ORANGE
    key                                    BLUE
    value                                  PURPLE
    update, result, chosen token           GREEN
  A highlight that is none of these uses ORANGE, except in part three,
  where every page draws queries: there the repeated work of recompute.py
  is hatched in INK, a treatment no other figure uses.
- Part three (recompute, prefix-invariance, kv-cache, query-lifetime)
  takes its columns, rows, chips and legend from kv_grid.py.
- Lines 1.6..2 px, arrowheads from svgkit, corner radius 6 for boxes and 4
  for token chips.
- No caption inside a figure that repeats the slide text, and no remark
  about the figure itself ("illustrative", "not measured").

Every helper returns a list of SVG elements; a figure is a list `b` that
helpers are appended to, written by `save(name, b, h)`.
"""

import hashlib
import math as _m

from svgkit import *  # noqa: F401,F403  (palette, text, rect, arrow, box, ...)
from svgkit import _advance, esc, save as _save

W = 1040
MATH = "Times New Roman, Liberation Serif, Noto Serif, serif"

DATA, FILL_DATA = "#5a6b78", "#eef2f4"
Q, FILL_Q = ORANGE, FILL_ORANGE
K, FILL_K = BLUE, FILL_BLUE
V, FILL_V = PURPLE, FILL_PURPLE
UPD, FILL_UPD = GREEN, FILL_GREEN

ROLE = {
    "data": (DATA, FILL_DATA),
    "q": (Q, FILL_Q),
    "k": (K, FILL_K),
    "v": (V, FILL_V),
    "upd": (UPD, FILL_UPD),
    "hl": (ORANGE, FILL_ORANGE),
    "red": (RED, FILL_RED),
}


def save(name, b, h, w=W, left=0):
    _save(name, w, h, b, left=left)


def width(s, size, font=FONT):
    """Rough advance of a label: CJK one em, latin 0.55 em."""
    return _advance(str(s), size, font)


def label(x, y, s, size=20, fill=INK, weight="normal", anchor="middle"):
    return text(x, y, s, size, fill, weight, anchor)


def note(x, y, s, size=17, anchor="middle"):
    return text(x, y, s, size, MUTED, "normal", anchor)


# ---------------------------------------------------------------- math text

def _math_runs(s):
    """Split 'W_Q x_{i}^T' into (text, shift) runs; shift 'sub' / 'sup' / ''."""
    runs, i = [], 0
    while i < len(s):
        c = s[i]
        if c in "_^" and i + 1 < len(s):
            kind = "sub" if c == "_" else "sup"
            if s[i + 1] == "{":
                j = s.index("}", i + 2)
                runs.append((s[i + 2:j], kind))
                i = j + 1
            else:
                runs.append((s[i + 1], kind))
                i += 2
        else:
            j = i
            while j < len(s) and s[j] not in "_^":
                j += 1
            runs.append((s[i:j], ""))
            i = j
    return runs


def _italic(ch):
    return ch.isascii() and ch.isalpha()


def math(x, y, s, size=22, fill=INK, anchor="middle", weight="normal"):
    """One line of math: latin letters italic, digits and signs upright.

    Syntax: `_x` / `_{xy}` subscript, `^x` / `^{xy}` superscript. Words that
    should stay upright (softmax, Attention) go in `\\up{...}`.
    """
    upright = set()
    while "\\up{" in s:
        a = s.index("\\up{")
        b_ = s.index("}", a)
        word = s[a + 4:b_]
        upright.add(word)
        s = s[:a] + "\x00" + word + "\x01" + s[b_ + 1:]
    spans, est = [], 0.0
    for chunk, kind in _math_runs(s):
        fs = size if not kind else size * 0.68
        dy = {"": 0, "sub": size * 0.28, "sup": -size * 0.38}[kind]
        up = False
        for piece in _split_upright(chunk):
            if piece == "\x00":
                up = True
                continue
            if piece == "\x01":
                up = False
                continue
            for ch in piece:
                style = "italic" if (_italic(ch) and not up) else "normal"
                spans.append((ch, fs, dy, style))
                est += fs * (0.5 if ch.isascii() else 1.0)
    out = []
    # tspans with explicit dy keep sub/superscripts on their own baseline
    parts, cur = [], 0.0
    for ch, fs, dy, style in spans:
        shift = dy - cur
        cur = dy
        parts.append(f'<tspan font-size="{fs:.1f}" font-style="{style}" '
                     f'dy="{shift:.1f}">{esc(ch)}</tspan>')
    out.append(f'<text x="{x:.1f}" y="{y:.1f}" font-family="{MATH}" '
               f'font-size="{size}" font-weight="{weight}" fill="{fill}" '
               f'text-anchor="{anchor}">' + "".join(parts) + "</text>")
    return out


def _split_upright(chunk):
    out, buf = [], ""
    for ch in chunk:
        if ch in "\x00\x01":
            if buf:
                out.append(buf)
                buf = ""
            out.append(ch)
        else:
            buf += ch
    if buf:
        out.append(buf)
    return out


def math_width(s, size=22):
    plain = s.replace("\\up{", "").replace("{", "").replace("}", "")
    w = 0.0
    for chunk, kind in _math_runs(plain):
        fs = size if not kind else size * 0.68
        w += sum(fs * (0.5 if ch.isascii() else 1.0) for ch in chunk)
    return w


# ---------------------------------------------------------------- tokens

def chip(x, y, word, role="data", size=20, pad=10, h=36, weight="normal",
         anchor="start", dash=None):
    """A token chip. Returns (elements, width). `anchor` places x at the
    chip's left edge ('start') or its centre ('middle')."""
    stroke, fill = ROLE[role]
    w = width(word, size) + 2 * pad
    left = x if anchor == "start" else x - w / 2
    out = [rect(left, y, w, h, fill, stroke, rx=4, width=1.6, dash=dash),
           text(left + w / 2, y + h / 2 + size * 0.36, word, size,
                INK if role == "data" else stroke, weight)]
    return out, w


def chip_row(x, y, words, roles=None, size=20, gap=10, h=36, pad=10):
    """Chips left to right from x. Returns (elements, [(left, centre, right)])."""
    out, spans = [], []
    roles = roles or ["data"] * len(words)
    for word, role in zip(words, roles):
        els, w = chip(x, y, word, role, size, pad, h)
        out += els
        spans.append((x, x + w / 2, x + w))
        x += w + gap
    return out, spans


def row_width(words, size=20, gap=10, pad=10):
    return sum(width(w, size) + 2 * pad for w in words) + gap * (len(words) - 1)


# ---------------------------------------------------------------- vectors

def _fmt(v):
    if isinstance(v, str):
        return v
    if isinstance(v, int):
        return str(v)
    return f"{v:.1f}" if abs(v) < 10 else f"{v:.0f}"


def vector(x, y, values, role="data", cw=58, ch=30, size=17, shade=None):
    """A column vector. `values` items are numbers, strings ('⋮' for an
    elision) or None (a blank cell, shaded by `shade` in [-1, 1] if given).
    Returns (elements, height)."""
    stroke, fill = ROLE[role]
    out = [rect(x - 3, y - 3, cw + 6, ch * len(values) + 6, "none", stroke,
                rx=4, width=1.8)]
    for i, v in enumerate(values):
        cy = y + i * ch
        cfill = WHITE
        if v is None and shade is not None:
            cfill = weight_fill(shade[i])
        out.append(rect(x, cy, cw, ch, cfill, LINE, rx=0, width=1))
        if v is not None:
            out.append(text(x + cw / 2, cy + ch / 2 + size * 0.36, _fmt(v), size,
                            INK, font=MONO if not isinstance(v, str) else FONT))
    return out, ch * len(values)


def hvector(x, y, values, role="data", cw=46, ch=30, size=16):
    """A row of cells (a vector laid flat). Returns (elements, width)."""
    stroke, fill = ROLE[role]
    out = [rect(x - 3, y - 3, cw * len(values) + 6, ch + 6, "none", stroke,
                rx=4, width=1.8)]
    for i, v in enumerate(values):
        cx = x + i * cw
        out.append(rect(cx, y, cw, ch, WHITE if v is not None else fill, LINE,
                        rx=0, width=1))
        if v is not None:
            out.append(text(cx + cw / 2, y + ch / 2 + size * 0.36, _fmt(v), size,
                            INK, font=MONO if not isinstance(v, str) else FONT))
    return out, cw * len(values)


# ---------------------------------------------------------------- weights

def pseudo(seed, i, j):
    """A fixed value in [-1, 1] for entry (i, j) of the matrix named seed."""
    h = hashlib.md5(f"{seed}:{i}:{j}".encode()).digest()
    return (h[0] / 255.0) * 2 - 1


def weight_fill(v):
    """Blue for positive, red for negative, paler near zero (3B1B's code)."""
    v = max(-1.0, min(1.0, v))
    base = (0x15, 0x60, 0x82) if v >= 0 else (0xc0, 0x39, 0x2b)
    a = 0.15 + 0.6 * abs(v)
    r, g, b_ = (round(255 + (c - 255) * a) for c in base)
    return f"#{r:02x}{g:02x}{b_:02x}"


def weights(x, y, rows, cols, cw=18, ch=18, seed="W", active=None, dim=None):
    """A weight matrix drawn as shaded cells. `active(i, j)` keeps a cell in
    colour; others are drawn pale when `dim` is true. Returns elements."""
    out = []
    for i in range(rows):
        for j in range(cols):
            on = active is None or active(i, j)
            f = weight_fill(pseudo(seed, i, j)) if on or not dim else "#f4f6f7"
            out.append(rect(x + j * cw, y + i * ch, cw, ch, f, WHITE, rx=0, width=1))
    out.append(rect(x, y, cols * cw, rows * ch, "none", INK, rx=2, width=1.6))
    return out


def matrix_label(x, y, s, size=24, fill=INK):
    return math(x, y, s, size, fill)


# ---------------------------------------------------------------- plots

def axes(x0, y0, w, h, xlabel=None, ylabel=None, color=LINE):
    """Axes with the origin at (x0, y0), growing right and up."""
    out = [arrow(x0, y0, x0 + w, y0, color, 1.6), arrow(x0, y0, x0, y0 - h, color, 1.6)]
    if xlabel:
        out.append(text(x0 + w, y0 + 26, xlabel, 17, MUTED, anchor="end"))
    if ylabel:
        out.append(text(x0 + 8, y0 - h + 4, ylabel, 17, MUTED, anchor="start"))
    return out


def dot(x, y, r=6, color=INK, fill=None):
    return circle(x, y, r, fill or color, color, 1.4)


def point(x, y, word, color=INK, dx=10, dy=-10, size=19, anchor="start", r=5):
    return [dot(x, y, r, color), text(x + dx, y + dy, word, size, color, "normal", anchor)]


def vec(x1, y1, x2, y2, color=INK, width=2.6, dash=None, shrink=7):
    """An arrow from point to point that stops `shrink` px short of the
    target, so a dot drawn there stays visible."""
    dx, dy = x2 - x1, y2 - y1
    d = _m.hypot(dx, dy) or 1
    return arrow(x1, y1, x2 - dx / d * shrink, y2 - dy / d * shrink, color, width, dash)


def hbars(x, y, items, bar_w=300, bar_h=24, gap=8, size=18, color=BLUE,
          pick=None, pick_color=GREEN, pmax=None, label_w=None):
    """Horizontal probability bars: items = [(word, p)], p in [0, 1].
    Words are right-aligned at x, bars start at x + 12, percentages follow.
    Returns (elements, height)."""
    out = []
    pmax = pmax or max(p for _, p in items)
    for i, (word, p) in enumerate(items):
        top = y + i * (bar_h + gap)
        c = pick_color if word == pick else color
        f = FILL_GREEN if word == pick else FILL_BLUE
        out.append(text(x, top + bar_h / 2 + size * 0.36, word, size, c if word == pick else INK,
                        "bold" if word == pick else "normal", "end"))
        bw = max(2.0, bar_w * p / pmax)
        out.append(rect(x + 12, top, bw, bar_h, f, c, rx=2, width=1.4))
        pct = f"{p * 100:.0f}%" if p >= 0.01 else f"{p * 100:.1f}%"
        out.append(text(x + 12 + bw + 8, top + bar_h / 2 + size * 0.36, pct, size - 1,
                        MUTED, anchor="start"))
    return out, len(items) * (bar_h + gap) - gap


def vbars(x, base, values, bw=40, gap=16, hmax=160, color=BLUE, fill=FILL_BLUE,
          labels=None, size=17, fmt=None, vmax=None):
    """Vertical bars standing on y = base. Returns elements."""
    out = []
    vmax = vmax or max(abs(v) for v in values) or 1
    for i, v in enumerate(values):
        left = x + i * (bw + gap)
        hgt = hmax * abs(v) / vmax
        top = base - hgt if v >= 0 else base
        out.append(rect(left, top, bw, max(hgt, 1.5), fill, color, rx=2, width=1.4))
        if fmt:
            ty = top - 8 if v >= 0 else top + hgt + size + 2
            out.append(text(left + bw / 2, ty, fmt(v), size - 1, INK))
        if labels:
            out.append(text(left + bw / 2, base + size + 8 if v >= 0 else base - 8,
                            labels[i], size, INK))
    return out


# ---------------------------------------------------------------- attention grid

def grid(x, y, rows, cols, cell=44, row_labels=None, col_labels=None, size=17,
         row_color=K, col_color=Q, fills=None, texts=None, text_color=None,
         label_rotate=False):
    """A table of cells: keys down the left, queries across the top (the
    video's layout). fills[i][j] / texts[i][j] give each cell's content.
    Returns elements."""
    out = []
    for i in range(rows):
        for j in range(cols):
            f = fills[i][j] if fills else WHITE
            out.append(rect(x + j * cell, y + i * cell, cell, cell, f, LINE, rx=0, width=1))
            if texts and texts[i][j] not in (None, ""):
                tc = text_color[i][j] if text_color else INK
                out.append(text(x + (j + .5) * cell, y + (i + .5) * cell + size * 0.36,
                                texts[i][j], size, tc, font=MONO))
    if row_labels:
        for i, s in enumerate(row_labels):
            out.append(text(x - 10, y + (i + .5) * cell + size * 0.36, s, size + 1,
                            row_color, anchor="end"))
    if col_labels:
        for j, s in enumerate(col_labels):
            cx, cy = x + (j + .5) * cell, y - 12
            if label_rotate:
                out.append(f'<text x="{cx:.1f}" y="{cy:.1f}" font-family="{FONT}" '
                           f'font-size="{size + 1}" fill="{col_color}" text-anchor="start" '
                           f'transform="rotate(-40 {cx:.1f} {cy:.1f})">{esc(s)}</text>')
            else:
                out.append(text(cx, cy, s, size + 1, col_color))
    return out


def dots_in_grid(x, y, values, cell=44, rmax=None, color=INK, fill=None, neg=RED):
    """Circles in a grid: radius grows with the value, negative values are
    drawn as hollow red rings."""
    out = []
    rmax = rmax or cell * 0.42
    vmax = max(abs(v) for r_ in values for v in r_ if v is not None) or 1
    for i, r_ in enumerate(values):
        for j, v in enumerate(r_):
            if v is None:
                continue
            r = max(2.0, rmax * _m.sqrt(abs(v) / vmax))
            cx, cy = x + (j + .5) * cell, y + (i + .5) * cell
            if v >= 0:
                out.append(circle(cx, cy, r, fill or color, color, 1.2))
            else:
                out.append(circle(cx, cy, r, WHITE, neg, 1.6))
    return out


# ---------------------------------------------------------------- misc

def panel(x, y, w, h, title=None, fill=WHITE, stroke=LINE, size=18):
    """A plain rounded frame with an optional title inside its top edge."""
    out = [rect(x, y, w, h, fill, stroke, rx=8, width=1.4)]
    if title:
        out.append(text(x + 14, y + size + 8, title, size, MUTED, "bold", anchor="start"))
    return out


def wrap(x, y, s, size=19, max_w=400, lh=None, fill=INK, anchor="start", weight="normal"):
    """Break `s` into lines no wider than max_w (CJK by character, latin by
    word). Returns (elements, height)."""
    lh = lh or size * 1.45
    lines, cur = [], ""
    for tok in _tokens(s):
        if width(cur + tok, size) > max_w and cur:
            lines.append(cur.rstrip())
            cur = tok.lstrip()
        else:
            cur += tok
    if cur:
        lines.append(cur.rstrip())
    out = [text(x, y + i * lh, ln, size, fill, weight, anchor) for i, ln in enumerate(lines)]
    return out, lh * len(lines)


def _tokens(s):
    out, buf = [], ""
    for ch in s:
        if ord(ch) > 0x2e80:
            if buf:
                out.append(buf)
                buf = ""
            out.append(ch)
        elif ch == " ":
            buf += ch
            out.append(buf)
            buf = ""
        else:
            buf += ch
    if buf:
        out.append(buf)
    return out


def curve(x1, y1, x2, y2, bend=-60, color=INK, width=2, head=True, dash=None):
    """A quadratic arc from (x1, y1) to (x2, y2); bend < 0 arcs upward."""
    mx, my = (x1 + x2) / 2, (y1 + y2) / 2 + bend
    return path(f"M {x1:.1f} {y1:.1f} Q {mx:.1f} {my:.1f} {x2:.1f} {y2:.1f}", color,
                width, dash=dash, head=head)
