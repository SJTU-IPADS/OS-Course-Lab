"""Shared layout of the cache-organisation figures.

A cache is drawn as sets of lines; a line is a row of cells [valid | tag | bytes].
An address is drawn as a bar of three fields. Every figure of the third part
uses these shapes, so the same entity has the same colour everywhere:
tag purple, set index blue, block offset orange, main memory green, a line of
the cache light blue with a dark outline, a hit green, a miss red.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_PURPLE,
                    FILL_RED, GREEN, INK, LINE, MONO, MUTED, ORANGE, PURPLE, RED, WHITE,
                    arrow, box, brace, circle, elbow, line, listing, mono, rect, save, text, vbrace,
                    path, FONT)

from svgkit import _marker
import svgkit


def _top(shape):
    """Mark a frame that has to lie over every cell, whatever is drawn after it."""
    return shape.replace("<rect ", '<rect data-top="1" ', 1)


def save(name, w, h, body, defs=(), left=0):
    """svgkit.save plus the arrow head in purple, the colour of the tag.

    Shapes marked by _top (the halos of lines and cells) are moved behind the
    last shape, so the next row of a stack of lines never covers a frame.
    """
    body = [s for s in body if 'data-top="1"' not in s] + [s for s in body if 'data-top="1"' in s]
    svgkit.save(name, w, h, body, [_marker(PURPLE)] + list(defs), left)


TAG, FILL_TAG = PURPLE, FILL_PURPLE
IDX, FILL_IDX = BLUE, FILL_BLUE
OFF, FILL_OFF = ORANGE, FILL_ORANGE
# one wash per set, light to dark; the set number is always printed on it
SET_FILLS = ["#e3f0f7", "#c3deec", "#9fc9de", "#7fb4d1"]
SET_STROKES = ["#8fb9d0", "#5a9fc2", "#2f7ea8", "#156082"]
FIELD_FILL = {"tag": FILL_TAG, "idx": FILL_IDX, "off": FILL_OFF}
FIELD_COLOR = {"tag": TAG, "idx": IDX, "off": OFF}


class Geo:
    """Cell sizes of a cache line."""

    def __init__(self, vw=26, tw=46, bw=36, lh=30, size=15):
        self.vw, self.tw, self.bw, self.lh, self.size = vw, tw, bw, lh, size

    def width(self, nbytes):
        return self.vw + self.tw + nbytes * self.bw


def cache_line(x, y, geo, nbytes, v=0, tag="", data=None, halo=None, req=None,
               tag_halo=None, v_halo=None, dim=False, sym=False):
    """One line. data is a list of nbytes strings (ignored when v == 0).

    halo colours a frame around the whole line; req is the index of the byte
    the CPU asked for (orange); tag_halo and v_halo frame those cells.
    """
    out = []
    valid = bool(v) or sym
    fill = FILL_BLUE if valid else WHITE
    edge = INK if valid else LINE
    if dim:
        fill, edge = (FILL_GREY if valid else WHITE), LINE
    cells = [("v", x, geo.vw), ("t", x + geo.vw, geo.tw)]
    cells += [("b", x + geo.vw + geo.tw + i * geo.bw, geo.bw) for i in range(nbytes)]
    for k, (kind, cx, cw) in enumerate(cells):
        f = fill
        if kind == "b" and req is not None and valid and k - 2 == req:
            f = FILL_ORANGE
        out.append(rect(cx, y, cw, geo.lh, f, edge, rx=0, width=1.2))
    if sym:
        base = y + geo.lh / 2 + geo.size * 0.36 - 1
        out.append(text(x + geo.vw / 2, base, "v", geo.size, INK, font=MONO))
        out.append(text(x + geo.vw + geo.tw / 2, base, "tag", geo.size, TAG, "bold", font=MONO))
        for i in range(nbytes):
            out.append(text(x + geo.vw + geo.tw + (i + 0.5) * geo.bw, base, f"B{i}",
                            geo.size - 2, MUTED, font=MONO))
    else:
        out.append(text(x + geo.vw / 2, y + geo.lh / 2 + geo.size * 0.36, str(v), geo.size,
                        INK if valid else MUTED, font=MONO))
    if valid and not sym:
        out.append(text(x + geo.vw + geo.tw / 2, y + geo.lh / 2 + geo.size * 0.36, tag,
                        geo.size, TAG, "bold", font=MONO))
        for i in range(nbytes):
            s = data[i] if data else ""
            on = req == i
            out.append(text(x + geo.vw + geo.tw + (i + 0.5) * geo.bw,
                            y + geo.lh / 2 + geo.size * 0.36 - 1, s, geo.size - 2,
                            ORANGE if on else INK, "bold" if on else "normal", font=MONO))
    w = geo.width(nbytes)
    out.append(rect(x, y, w, geo.lh, "none", edge if not dim else LINE, rx=0, width=1.8))
    if halo:
        out.append(_top(rect(x - 4, y - 4, w + 8, geo.lh + 8, "none", halo, rx=5, width=3.2)))
    if tag_halo:
        out.append(_top(rect(x + geo.vw - 2, y - 2, geo.tw + 4, geo.lh + 4, "none", tag_halo,
                             rx=3, width=3)))
    if v_halo:
        out.append(_top(rect(x - 2, y - 2, geo.vw + 4, geo.lh + 4, "none", v_halo, rx=3,
                             width=3)))
    return out


def cache_sets(x, y, geo, nbytes, sets, set_gap=12, labels=True, set_fills=None,
               headers=True, label_dx=14, group=None, lines_kw=None, set_halo=None):
    """A stack of sets; sets is a list (one per set) of lists of line dicts.

    A line dict holds the keyword arguments of cache_line. Returns
    (shapes, pos, bottom) with pos[(set, k)] = (x, y) of every line.
    set_halo = {set: colour} frames a whole set; group draws a dashed frame
    around the lines of a set (default: when a set has more than one line).
    """
    out, pos = [], {}
    cy = y
    w = geo.width(nbytes)
    if headers:
        out += column_headers(x, y - 8, geo, nbytes)
    for s, lines in enumerate(sets):
        top = cy
        for k, kw in enumerate(lines):
            out += cache_line(x, cy, geo, nbytes, **kw)
            pos[(s, k)] = (x, cy)
            cy += geo.lh
        if (group if group is not None else len(lines) > 1):
            out.append(rect(x - 9, top - 8, w + 18, cy - top + 16, "none", LINE, rx=7,
                            width=1.4, dash="5 3"))
        if set_halo and s in set_halo:
            out.append(rect(x - 8, top - 6, w + 16, cy - top + 12, "none", set_halo[s],
                            rx=6, width=3.4))
        if labels:
            lab = f"组 {s}"
            out.append(text(x - label_dx - (9 if len(lines) > 1 else 0), (top + cy) / 2 + 6,
                            lab, geo.size + 2, IDX, "bold", "end"))
        cy += set_gap
    return out, pos, cy - set_gap


def column_headers(x, y, geo, nbytes, size=14):
    return [text(x + geo.vw / 2, y, "有效位", size, MUTED),
            text(x + geo.vw + geo.tw / 2, y, "标记", size, TAG, "bold"),
            text(x + geo.vw + geo.tw + nbytes * geo.bw / 2, y,
                 f"块（B = {nbytes} 字节）", size, MUTED)]


def addr_bits(x, y, bits, t, s, b, cw=34, ch=36, size=20, labels=True, label_size=15,
              names=("标记", "组索引", "块偏移"), dim=()):
    """An address as a bar of bit cells; bits is a string of len t + s + b.

    Fields with t, s or b == 0 are skipped. dim lists fields ("tag", "idx", "off")
    drawn washed out. Returns (shapes, boxes) with boxes[field] = (x0, x1).
    """
    out, boxes, outlines = [], {}, []
    fields = [("tag", t), ("idx", s), ("off", b)]
    pos = 0
    cx = x
    for name, n in fields:
        if n == 0:
            continue
        x0 = cx
        for i in range(n):
            f = FIELD_FILL[name] if name not in dim else WHITE
            e = FIELD_COLOR[name] if name not in dim else LINE
            out.append(rect(cx, y, cw, ch, f, e, rx=0, width=1.4))
            if bits:
                out.append(text(cx + cw / 2, y + ch / 2 + size * 0.36, bits[pos], size,
                                FIELD_COLOR[name] if name not in dim else MUTED, "bold",
                                font=MONO))
            pos += 1
            cx += cw
        boxes[name] = (x0, cx)
        outlines.append(_top(rect(x0, y, cx - x0, ch, "none", FIELD_COLOR[name] if name not in dim
                                  else LINE, rx=3, width=2.4)))
    out += outlines                      # the outline of a field lies over its neighbour's cells
    if labels:
        for (name, n), word in zip(fields, names):
            if n == 0:
                continue
            x0, x1 = boxes[name]
            cnt = {"tag": "t", "idx": "s", "off": "b"}[name]
            out.append(text((x0 + x1) / 2, y - 10, word, label_size,
                            FIELD_COLOR[name] if name not in dim else MUTED, "bold"))
    return out, boxes


def addr_fields(x, y, wt, ws, wb, h=44, size=18, names=("标记 t 位", "组索引 s 位",
                "块偏移 b 位")):
    """The abstract bar: three boxes of the given widths, labelled inside."""
    out, boxes = [], {}
    cx = x
    for name, w, label in zip(("tag", "idx", "off"), (wt, ws, wb), names):
        if w == 0:
            continue
        out.append(rect(cx, y, w, h, FIELD_FILL[name], FIELD_COLOR[name], rx=3, width=2.2))
        out.append(text(cx + w / 2, y + h / 2 + size * 0.36, label, size, FIELD_COLOR[name],
                        "bold"))
        boxes[name] = (cx, cx + w)
        cx += w
    return out, boxes


def badge(cx, cy, label, kind, size=16, w=None):
    """A small result tag: kind is 'hit' or 'miss'."""
    col, fill = (GREEN, FILL_GREEN) if kind == "hit" else (RED, FILL_RED)
    w = w or max(54, len(label) * size * 1.05 + 18)
    return [rect(cx - w / 2, cy - 14, w, 28, fill, col, rx=14, width=1.6),
            text(cx, cy + size * 0.36, label, size, col, "bold")]


def memory_block(x, y, w, h, label, fill=FILL_GREEN, stroke=GREEN, size=15, color=INK,
                 weight="normal", dash=None, font=MONO):
    return [rect(x, y, w, h, fill, stroke, rx=2, width=1.4, dash=dash),
            text(x + w / 2, y + h / 2 + size * 0.36, label, size, color, weight, font=font)]


def lines_state(entries, nbytes):
    """Turn [(v, tag, [data...]) | None, ...] into line dicts."""
    out = []
    for e in entries:
        if e is None:
            out.append(dict(v=0))
        else:
            v, tag, data = e
            out.append(dict(v=v, tag=tag, data=data))
    return out


def split_table(x, y, m, t, s, b, cw=54, rh=38, label_w=92, set_wash=True, size=18):
    """The split of every address of an m-bit address space into tag | index | offset.

    One column per address. Rows: block (washed by set), address, its bits in the
    three field colours, tag, set index, block offset. Returns (shapes, height).
    """
    out = []
    n = 1 << m
    rows = ["块", "地址", "二进制", "标记", "组索引", "块偏移"]
    colors = [INK, INK, INK, TAG, IDX, OFF]
    for r, (name, col) in enumerate(zip(rows, colors)):
        out.append(text(x + label_w - 14, y + r * rh + rh / 2 + 6, name, 17, col, "bold",
                        "end"))
    bw = 1 << b
    for blk in range(n // bw):
        st = blk % (1 << s)
        out.append(rect(x + label_w + blk * bw * cw, y, bw * cw, rh,
                        SET_FILLS[st] if set_wash else FILL_GREEN, SET_STROKES[st], rx=2,
                        width=1.4))
        out.append(text(x + label_w + (blk * bw + bw / 2) * cw, y + rh / 2 + 6,
                        f"块 {blk} · 组 {st}" if bw > 1 else f"{blk}", 15, INK, "bold"))
    for a in range(n):
        cx = x + label_w + a * cw
        bits = format(a, f"0{m}b")
        tag, idx, off = bits[:t], bits[t:t + s], bits[t + s:]
        out.append(rect(cx, y + rh, cw, rh, WHITE, LINE, rx=0, width=1))
        out.append(text(cx + cw / 2, y + rh + rh / 2 + 6, str(a), size, INK, "bold",
                        font=MONO))
        out.append(rect(cx, y + 2 * rh, cw, rh, WHITE, LINE, rx=0, width=1))
        # the three fields of the bits, each in its own colour
        digits = [(tag, TAG), (idx, IDX), (off, OFF)]
        txt_x = cx + cw / 2 - m * size * MONO_EM / 2
        for part, col in digits:
            for ch in part:
                out.append(text(txt_x + size * MONO_EM / 2, y + 2 * rh + rh / 2 + 6, ch, size,
                                col, "bold", font=MONO))
                txt_x += size * MONO_EM
        for r, (val, col) in enumerate(((tag, TAG), (idx, IDX), (off, OFF))):
            out.append(rect(cx, y + (3 + r) * rh, cw, rh, WHITE, LINE, rx=0, width=1))
            if val:
                out.append(text(cx + cw / 2, y + (3 + r) * rh + rh / 2 + 6, val, size, col,
                                "bold", font=MONO))
    return out, 6 * rh


MONO_EM = 0.6
