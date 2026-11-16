#!/usr/bin/env python3
"""dataflow-overview: the whole model, and its inside magnified.

Left: the whole model, bottom to top: a text goes into the Transformer box
(orange: the box that is magnified) and the completed text comes out, the
appended token in green. Right: the inside of that box, joined to it by two
dashed projection lines, flowing bottom to top like layer-stack: the last
tokens of the text, the embedding matrix, one column vector per token, an
attention block and an MLP (one layer, repeated), the last vector of the top
layer (orange) through the unembedding matrix to a distribution over all
tokens, and the sampled token.

The example is the one of generation and chat; the tokens are GPT-3's
(" foraging" is " for" + "aging"). One frame.
"""
import hashlib

from tk import *

H = 400

# the whole model (left column)
TX, TW = 16, 190                  # text boxes and the Transformer box
CX = TX + TW / 2
FS, LH = 18, 24                   # text size, line height
LINES = ["Behold, a wild", "pi creature,", "foraging in its", "native"]
NEW = "woodland"
TBH = len(LINES) * LH + 16        # text box height
OUT_Y, IN_Y = 12, H - 12 - TBH    # output box on top, input box at the bottom
MY, MH = 168, 64                  # the Transformer box

# the magnified panel
PX, PY, PW, PH = 258, 10, 766, 380

# the stack inside it: lanes, mini vectors and bands as in layer-stack.py
TOKS = ["aging", "in", "its", "native"]
LX0, LP = 440, 82
LANES = [LX0 + k * LP for k in range(len(TOKS))]
BL, BR = LANES[0] - 44, LANES[-1] + 44
VW, VC, NV = 24, 9, 4
BAND_H = 30
V3Y, V2Y, V1Y = 34, 116, 242      # top layer, after one layer, after the embedding
MLP_Y, ATT_Y = 162, 202           # the bands of one layer
EMB_Y, EMB_R = 288, 2             # the embedding matrix: top, rows
CHIP_Y = 332
FLOW_X = 284                      # the arrow giving the direction of flow

# the output side: unembedding, distribution, sampled token
RC = 850                          # centre of this column
UW_C, UW_R, UC = 6, 4, 14         # unembedding matrix: columns, rows, cell
UW_Y = V3Y + VC * NV / 2 - UW_R * UC / 2
BASE, HMAX, NBAR = 214, 80, 32    # the distribution's bars


def shade(seed, i, j):
    """The grey of a data cell, as in layer-stack.py."""
    h = hashlib.md5(f"{seed}:{i}:{j}".encode()).digest()[0] / 255.0
    g = round(238 - 120 * h)
    return f"#{g - 10:02x}{g - 2:02x}{g + 6 if g + 6 < 256 else 255:02x}"


def mini(cx, y, seed, stroke=DATA, width=1.4):
    """A small column vector, as in layer-stack.py."""
    out = []
    for i in range(NV):
        out.append(rect(cx - VW / 2, y + i * VC, VW, VC, shade(seed, i, 0), WHITE, rx=0,
                        width=0.8))
    out.append(rect(cx - VW / 2 - 2, y - 2, VW + 4, NV * VC + 4, "none", stroke, rx=3,
                    width=width))
    return out


def textbox(y, appended):
    out = [rect(TX, y, TW, TBH, FILL_DATA, DATA, rx=6, width=1.4)]
    for i, s in enumerate(LINES):
        ty = y + 8 + LH * (i + 0.5) + FS * 0.36
        if appended and i == len(LINES) - 1:
            out.append(f'<text x="{TX + 14:.1f}" y="{ty:.1f}" font-family="{FONT}" '
                       f'font-size="{FS}" fill="{INK}" text-anchor="start">{esc(s)}'
                       f'<tspan fill="{GREEN}" font-weight="bold"> {NEW}</tspan></text>')
        else:
            out.append(label(TX + 14, ty, s, FS, INK, anchor="start"))
    return out


def whole():
    b = textbox(OUT_Y, True)
    b += textbox(IN_Y, False)
    b.append(rect(TX, MY, TW, MH, FILL_BLUE, ORANGE, rx=6, width=2.4))
    b.append(label(CX, MY + MH / 2 + 7, "Transformer", 20, INK, "bold"))
    b.append(arrow(CX, IN_Y - 4, CX, MY + MH + 4, INK, 2))
    b.append(arrow(CX, MY - 4, CX, OUT_Y + TBH + 4, INK, 2))
    return b


def stack():
    b = []
    for x in LANES:
        b.append(line(x, V3Y + NV * VC, x, CHIP_Y - 4, LINE, 1.6))
    b.append(label(LANES[0] - 50, CHIP_Y + 23, "⋯", 20, MUTED))
    for x, t in zip(LANES, TOKS):
        els, _ = chip(x, CHIP_Y, t, size=18, h=32, anchor="middle")
        b += els
    # the embedding matrix, every lane passes through it
    b += weights(BL, EMB_Y, EMB_R, 22, cw=(BR - BL) / 22, ch=BAND_H / EMB_R, seed="ov-WE")
    b.append(label(BL - 12, EMB_Y + BAND_H / 2 + 6.5, "词嵌入", 18, INK, "bold", anchor="end"))
    # one layer
    for y, name in ((ATT_Y, "注意力块"), (MLP_Y, "多层感知机")):
        b.append(rect(BL, y, BR - BL, BAND_H, FILL_BLUE, BLUE, rx=6, width=1.6))
        b.append(label((BL + BR) / 2, y + 21, name, 18, INK, "bold"))
    lo, hi, x = MLP_Y, ATT_Y + BAND_H, BL - 10
    b.append(path(f"M {x} {lo} L {x - 7} {lo} L {x - 7} {hi} L {x} {hi}", MUTED, 1.4,
                  head=False))
    b.append(text(x - 14, (lo + hi) / 2 + 6, "重复多层", 17, MUTED, anchor="end"))
    # the vectors
    for y in (V1Y, V2Y, V3Y):
        for k, x in enumerate(LANES):
            last = y == V3Y and k == len(LANES) - 1
            b += mini(x, y, f"ov{y}:{k}", ORANGE if last else DATA, 2.4 if last else 1.4)
    dy = (V3Y + NV * VC + V2Y) / 2
    for x in LANES:
        b.append(rect(x - 8, dy - 12, 16, 22, WHITE, "none", rx=0, width=0))
        b.append(label(x, dy + 6, "⋮", 18, MUTED))
    b.append(arrow(FLOW_X, CHIP_Y + 30, FLOW_X, V3Y + 10, MUTED, 2))
    return b


def output():
    b = []
    ux = RC - UW_C * UC / 2
    vy = V3Y + NV * VC / 2
    b.append(arrow(LANES[-1] + VW / 2 + 6, vy, ux - 6, vy, ORANGE, 2.2))
    b += weights(ux, UW_Y, UW_R, UW_C, cw=UC, ch=UC, seed="ov-WU")
    b.append(label(ux + UW_C * UC + 14, vy + 6.5, "解嵌入", 18, INK, "bold", anchor="start"))
    b.append(arrow(RC, UW_Y + UW_R * UC + 4, RC, BASE - HMAX - 8, INK, 2))
    # the distribution over all tokens, as layer-stack drew it
    pitch = 5
    x0 = RC - NBAR * pitch / 2
    vals = []
    for i in range(NBAR):
        h = hashlib.md5(f"ov-vocab:{i}".encode()).digest()[0] / 255.0
        vals.append(0.25 * h ** 8)
    vals[5], vals[12], vals[20], vals[26] = 1.0, 0.45, 0.3, 0.18
    b.append(line(x0 - 6, BASE, x0 + NBAR * pitch + 2, BASE, LINE, 1.4))
    for i, v in enumerate(vals):
        hgt = max(1.5, HMAX * v)
        b.append(rect(x0 + i * pitch, BASE - hgt, 4, hgt, BLUE, "none", rx=0, width=0))
    b.append(label(RC, BASE + 26, "所有 token 的概率分布", 17, INK))
    b.append(arrow(RC, BASE + 38, RC, CHIP_Y - 6, INK, 2))
    b.append(label(RC + 12, (BASE + 38 + CHIP_Y) / 2 + 6, "采样", 17, INK, anchor="start"))
    els, _ = chip(RC, CHIP_Y, NEW, "upd", size=18, h=32, weight="bold", anchor="middle")
    b += els
    return b


b = whole()
# the projection lines: the Transformer box, magnified
b.append(line(TX + TW + 2, MY, PX, PY + 8, ORANGE, 1.4, dash="5 5"))
b.append(line(TX + TW + 2, MY + MH, PX, PY + PH - 8, ORANGE, 1.4, dash="5 5"))
b.append(rect(PX, PY, PW, PH, WHITE, ORANGE, rx=8, width=1.6))
b += stack()
b += output()
save("dataflow-overview", b, H)
