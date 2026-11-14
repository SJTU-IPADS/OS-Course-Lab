#!/usr/bin/env python3
"""tokens: the input cut into pieces.

1: the sentence of the video cut into its tokens: whole words, pieces of a
   word (cle|ve|rest) and punctuation.
2: + an image cut into a grid of patches, a few patches taken out as tokens;
   a waveform cut into chunks, a few chunks taken out.

The examples are parallel, so the figure is drawn once, in its final state;
frame() keeps the step numbers only to say which parts are drawn.
"""
import math as m

from tk import *

H = 400
SENT = "To date, the cleverest thinker of all time was"
TOKS = ["To", "date", ",", "the", "cle", "ve", "rest", "thinker", "of", "all", "time", "was"]

# image geometry: 6 x 4 patches of 36 px
IX, IY, P, NC, NR = 60, 214, 36, 6, 4
# waveform geometry: 6 chunks of 40 px
WX, WY, WC, NW, WH = 560, 278, 40, 6, 96

clips = []


def landscape(ox, oy):
    """The picture with its top-left corner at (ox, oy), 216 x 144."""
    w, h = P * NC, P * NR
    out = [rect(ox, oy, w, h, FILL_DATA, "none", rx=0, width=0),
           circle(ox + 170, oy + 34, 17, "#dfe6eb", MUTED, 1.4),
           path(f"M {ox:.1f} {oy + 112:.1f} L {ox + 62:.1f} {oy + 48:.1f} L {ox + 118:.1f} {oy + 106:.1f} Z",
                MUTED, 1.4, fill=LINE, head=False),
           path(f"M {ox + 84:.1f} {oy + 112:.1f} L {ox + 140:.1f} {oy + 64:.1f} L {ox + 216:.1f} {oy + 118:.1f} Z",
                MUTED, 1.4, fill="#b9c9d4", head=False),
           rect(ox, oy + 108, w, h - 108, "#cbd6dd", "none", rx=0, width=0),
           rect(ox + 38, oy + 118, 5, 16, MUTED, "none", rx=0, width=0),
           path(f"M {ox + 26:.1f} {oy + 122:.1f} L {ox + 40.5:.1f} {oy + 88:.1f} L {ox + 55:.1f} {oy + 122:.1f} Z",
                MUTED, 1.2, fill="#7f97a6", head=False)]
    return out


def wave(x, y, w, h, color=DATA):
    out = [line(x, y, x + w, y, LINE, 1)]
    n = int(w / 3)
    for i in range(n):
        t = i / n
        env = 0.25 + 0.75 * abs(m.sin(t * 7.0 + 0.4)) * (0.6 + 0.4 * m.sin(t * 19 + 1))
        a = h / 2 * env * (0.55 + 0.45 * abs(m.sin(i * 1.7)))
        xi = x + 1.5 + i * 3
        out.append(line(xi, y - a, xi, y + a, color, 1.6))
    return out


def moved(cid, x, y, w, h, dx, dy, body):
    """The region (x, y, w, h) of `body`, shifted by (dx, dy)."""
    clips.append(f'<clipPath id="{cid}"><rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" '
                 f'height="{h:.1f}"/></clipPath>')
    return ([f'<g transform="translate({dx:.1f} {dy:.1f})"><g clip-path="url(#{cid})">']
            + body + ["</g></g>"])


def frame(n):
    clips.clear()
    b = []
    # 1: the sentence and its tokens
    sw = width(SENT, 22) + 36
    b.append(rect(520 - sw / 2, 14, sw, 44, FILL_DATA, DATA, rx=6, width=1.4))
    b.append(label(520, 44, SENT, 22, INK))
    b.append(arrow(520, 62, 520, 92, INK, 2))
    b.append(note(532, 82, "切分", anchor="start"))
    rw = row_width(TOKS, 20, 8)
    els, spans = chip_row(520 - rw / 2, 98, TOKS, size=20, gap=8)
    b += els
    b += brace(spans[4][0], spans[6][2], 140, MUTED, "cleverest", size=17)
    b.append(note(spans[-1][2], 170, "token 可以是词、词的一部分或标点", anchor="end"))

    if n >= 2:
        # an image cut into patches
        b += landscape(IX, IY)
        for c in range(1, NC):
            b.append(line(IX + c * P, IY, IX + c * P, IY + NR * P, WHITE, 2))
        for r in range(1, NR):
            b.append(line(IX, IY + r * P, IX + NC * P, IY + r * P, WHITE, 2))
        b.append(rect(IX, IY, NC * P, NR * P, "none", DATA, rx=2, width=1.6))
        b.append(arrow(IX + NC * P + 12, IY + NR * P / 2, IX + NC * P + 48, IY + NR * P / 2, INK, 2))
        tx0, ty = IX + NC * P + 60, IY + NR * P / 2 - P / 2
        for k, (r, c) in enumerate([(0, 4), (1, 1), (2, 3), (3, 0)]):
            tx = tx0 + k * (P + 10)
            sx, sy = IX + c * P, IY + r * P
            b += moved(f"tok-p{k}", sx, sy, P, P, tx - sx, ty - sy, landscape(IX, IY))
            b.append(rect(tx, ty, P, P, "none", DATA, rx=2, width=1.6))
        b.append(label(tx0 + 4 * (P + 10) + 8, ty + P / 2 + 6, "⋯", 20, MUTED))
        b.append(label(IX + 215, 386, "图像切成小块", 19, INK))

        # a waveform cut into chunks
        b += wave(WX, WY, NW * WC, WH)
        for c in range(1, NW):
            b.append(line(WX + c * WC, WY - WH / 2 - 6, WX + c * WC, WY + WH / 2 + 6, MUTED, 1.4,
                          dash="4 4"))
        b.append(arrow(WX + NW * WC + 10, WY, WX + NW * WC + 42, WY, INK, 2))
        cx0 = WX + NW * WC + 54
        for k, c in enumerate([0, 2, 5]):
            tx = cx0 + k * (WC + 8)
            sx = WX + c * WC
            b += moved(f"tok-w{k}", sx, WY - WH / 2, WC, WH, tx - sx, 0,
                       wave(WX, WY, NW * WC, WH))
            b.append(rect(tx, WY - WH / 2 - 4, WC, WH + 8, "none", DATA, rx=2, width=1.6))
        b.append(label(cx0 + 3 * (WC + 8) + 6, WY + 6, "⋯", 20, MUTED))
        b.append(label(WX + 212, 386, "声音切成片段", 19, INK))
    if clips:
        b.insert(0, "<defs>" + "".join(clips) + "</defs>")
    save("tokens", b, H)


frame(2)
