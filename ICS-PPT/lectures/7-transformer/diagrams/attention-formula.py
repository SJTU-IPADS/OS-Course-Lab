#!/usr/bin/env python3
"""attention-formula: the attention formula of the paper (1 frame).

Large, in the middle: Attention(Q, K, V) = softmax(Q K^T / √d_k) V, every
token a row. Thin leader lines tie each part to a note: the dot products,
the scaling, the softmax per query, the values. Below, smaller: the same
computation in the layout of this part's grids (queries as columns), its
transpose V softmax(K^T Q / √d_k). The video writes softmax(K^T Q / √d_k) V,
which mixes the two layouts.

Text is laid out run by run with the advances of Liberation Serif (metric
compatible with Times New Roman), so the leader lines meet the symbols.
"""
from PIL import ImageFont

from tk import *

H = 340
FONTS = "/usr/share/fonts/truetype/liberation/LiberationSerif-{}.ttf"
_cache = {}


def advance(s, size, italic):
    key = (size, italic)
    if key not in _cache:
        _cache[key] = ImageFont.truetype(FONTS.format("Italic" if italic else "Regular"), 100)
    return _cache[key].getlength(s) * size / 100


def run_row(x, y, items, size):
    """items: (text, colour, italic, shift) with shift '', 'sub' or 'sup'.
    Returns (elements, [(left, right)] per item, right edge)."""
    out, spans = [], []
    for s, col, it, sh in items:
        fs = size * 0.68 if sh else size
        yy = y + {"": 0, "sub": size * 0.28, "sup": -size * 0.4}[sh]
        style = ' font-style="italic"' if it else ""
        out.append(f'<text x="{x:.1f}" y="{yy:.1f}" font-family="{MATH}" font-size="{fs:.1f}"'
                   f'{style} fill="{col}" text-anchor="start" xml:space="preserve">{esc(s)}</text>')
        w = advance(s, fs, it)
        spans.append((x, x + w))
        x += w
    return out, spans, x


def width_of(items, size):
    return run_row(0, 0, items, size)[2]


def paren(x, y0, y1, right=False, size=36):
    """A tall parenthesis from y0 to y1 with its outer side at x."""
    d = size * 0.22
    s = -1 if right else 1
    return (f'<path d="M {x + s * d:.1f} {y0:.1f} Q {x - s * d * 0.4:.1f} {(y0 + y1) / 2:.1f} '
            f'{x + s * d:.1f} {y1:.1f}" fill="none" stroke="{INK}" stroke-width="{size / 20:.1f}"/>')


def formula(cx, base, size, num, den_items, lhs=True):
    """softmax(num / √den) V centred on cx; returns elements and anchor points."""
    left = []
    if lhs:
        left = [("Attention", INK, False, ""), ("(", INK, False, ""), ("Q", Q, True, ""),
                (", ", INK, False, ""), ("K", K, True, ""), (", ", INK, False, ""),
                ("V", V, True, ""), (")", INK, False, ""), (" = ", INK, False, "")]
    left += [("softmax", INK, False, "")]
    bar_y = base - size * 0.3
    num_w = width_of(num, size)
    rad_w = size * 0.55
    den_w = rad_w + width_of(den_items, size)
    frac_w = max(num_w, den_w) + size * 0.4
    pw = size * 0.45                               # room taken by each parenthesis
    v_items = [("V", V, True, "")]
    total = width_of(left, size) + 2 * pw + frac_w + width_of(v_items, size) + 4
    x = cx - total / 2
    out = []
    els, spans_l, x = run_row(x, base, left, size)
    out += els
    num_base = bar_y - size * 0.3
    den_base = bar_y + size * 0.95
    top, bot = num_base - size * 0.78, den_base + size * 0.3
    out.append(paren(x + pw * 0.3, top, bot, size=size))
    fx = x + pw
    out.append(line(fx, bar_y, fx + frac_w, bar_y, INK, size / 24))
    els, spans_n, _ = run_row(fx + (frac_w - num_w) / 2, num_base, num, size)
    out += els
    dx = fx + (frac_w - den_w) / 2
    els, spans_d, dend = run_row(dx + rad_w, den_base, den_items, size)
    out += els
    rt = den_base - size * 0.8                     # the radical
    out.append(path(f"M {dx:.1f} {den_base - size * 0.3:.1f} L {dx + size * 0.1:.1f} "
                    f"{den_base - size * 0.36:.1f} L {dx + size * 0.24:.1f} {den_base + 2:.1f} "
                    f"L {dx + size * 0.46:.1f} {rt:.1f} L {dend + 2:.1f} {rt:.1f}", INK,
                    size / 22, head=False))
    x = fx + frac_w
    out.append(paren(x + pw * 0.7, top, bot, right=True, size=size))
    els, spans_v, _ = run_row(x + pw + 2, base, v_items, size)
    out += els
    pts = {"softmax": ((spans_l[-1][0] + spans_l[-1][1]) / 2, base - size * 0.72),
           "num": ((spans_n[0][0] + spans_n[-1][1]) / 2, num_base - size * 0.78),
           "den": ((dx + dend) / 2, den_base + size * 0.3),
           "V": ((spans_v[0][0] + spans_v[0][1]) / 2, base - size * 0.72)}
    return out, pts


def note_above(b, pt, x, y, s):
    b.append(line(pt[0], pt[1] - 4, x, y + 8, MUTED, 1.2))
    b.append(text(x, y, s, 18, INK))


def build():
    b = []
    size = 38
    # the paper's form, one token per row: rows of QK^T are queries
    num = [("Q", Q, True, ""), ("K", K, True, ""), ("T", K, False, "sup")]
    den = [("d", INK, True, ""), ("k", INK, True, "sub")]
    els, p = formula(W / 2, 160, size, num, den)
    b += els
    note_above(b, p["softmax"], p["softmax"][0] - 24, 70, "逐个查询归一化")
    note_above(b, p["num"], p["num"][0], 32, "所有查询与键的点积")
    note_above(b, p["V"], p["V"][0] + 48, 70, "值，用于更新嵌入")
    d = p["den"]
    b.append(line(d[0], d[1] + 4, d[0], d[1] + 22, MUTED, 1.2))
    b.append(text(d[0], d[1] + 42, "除以键查询空间维数的平方根，保持数值稳定", 18, INK))

    # the layout of the grids in this part: queries as columns, the transpose
    b.append(line(W / 2 - 300, 252, W / 2 + 300, 252, "#dbe4ea", 1.2))
    b.append(text(W / 2 + 20, 292, "以查询为列的表是它的转置：", 17, MUTED, anchor="end"))
    b += math(W / 2 + 32, 292, "V \\up{softmax}(K^T Q / √d_k)", 22, INK, anchor="start")
    b.append(text(W / 2, 326, "论文中每个 token 占一行。本讲的表键为行、查询为列，softmax 逐列进行，"
                  "V 的每一列是一个值向量", 17, MUTED))
    save("attention-formula", b, H)


build()
