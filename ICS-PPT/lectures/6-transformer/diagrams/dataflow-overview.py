#!/usr/bin/env python3
"""dataflow-overview: the computation of the model, step by step.

Left, one column, level with the step it names: each step and its main
computation (embedding: a lookup in W_E; attention block: matrix products,
dot products, Softmax; MLP: matrix products; unembedding: W_U, then
Softmax), with a grey arrow giving the order, bottom to top. Middle, flowing
up as in layer-stack: the last tokens of the text, the embedding matrix, one
column vector per token, one layer, bracketed as repeated, and the vectors of
the top layer. In the layer, the attention block is one band across the
lanes, its arcs joining earlier positions to later ones; the MLP is one box
per lane, no line between them. Right: the last vector of the top layer
(orange, as in unembedding) through W_U and Softmax to a distribution over
all tokens, and the sampled token (green), level with the input tokens.

The example is the one of generation and chat; the tokens are GPT-3's
(" foraging" is " for" + "aging"). One frame.
"""
import hashlib

from tk import *

H = 400

# the stack: lanes and mini vectors as in layer-stack.py
TOKS = ["aging", "in", "its", "native"]
LX0, LP = 322, 76
LANES = [LX0 + k * LP for k in range(len(TOKS))]
BL, BR = LANES[0] - 44, LANES[-1] + 44
VW, VC, NV = 24, 9, 4
V3Y = 20                          # top layer
DOTS_Y = 84                       # the elided layers
MLP_Y, MLP_W, MLP_H = 106, 48, 36
ATT_Y, ATT_H = 164, 60
V1Y = 244                         # after the embedding
EMB_Y, EMB_H, EMB_R = 294, 30, 2  # the embedding matrix: top, height, rows
CHIP_Y = 350

# the step labels: one column on the left, an arrow giving the order
FLOW_X, LAB_X = 22, 44
NAME, SUB = 20, 17

# what tells the two blocks apart, and the repeat bracket, right of the stack
DIFF_X = BR + 18
REP_X = DIFF_X + 148

# the output side: unembedding, distribution, sampled token
RC = 940                          # centre of this column
UW_C, UW_R, UC = 6, 4, 14         # unembedding matrix: columns, rows, cell
BASE, HMAX, NBAR = 214, 76, 28    # the distribution's bars


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


def runs(x, y, parts, size=SUB, fill=INK):
    """One line of text with math pieces in it: parts are ('t', s) or ('m', s),
    s like 'W_U'. One <text> element, so the browser sets the advances."""
    ms = size + 2
    drop = ms * 0.28
    spans, shift = [], 0.0
    for kind, s in parts:
        if kind == "t":
            spans.append(f'<tspan dy="{-shift:.1f}">{esc(s)}</tspan>')
            shift = 0.0
            continue
        base, sub = s.split("_")
        spans.append(f'<tspan font-family="{MATH}" font-style="italic" font-size="{ms}" '
                     f'dy="{-shift:.1f}">{esc(base)}</tspan>')
        spans.append(f'<tspan font-family="{MATH}" font-style="italic" '
                     f'font-size="{ms * 0.68:.1f}" dy="{drop:.1f}">{esc(sub)}</tspan>')
        shift = drop
    return [f'<text x="{x:.1f}" y="{y:.1f}" font-family="{FONT}" font-size="{size}" '
            f'fill="{fill}" text-anchor="start">' + "".join(spans) + "</text>"]


def step(cy, name, parts):
    """A step's name and its computation, two lines centred on y = cy."""
    return [label(LAB_X, cy - 3, name, NAME, INK, "bold", anchor="start")] + \
        runs(LAB_X, cy + 21, parts)


def labels():
    b = [arrow(FLOW_X, CHIP_Y + 30, FLOW_X, V3Y + 4, MUTED, 2)]
    b += step(V3Y + NV * VC / 2, "解嵌入与 Softmax",
              [("t", "乘 "), ("m", "W_U"), ("t", "，再做 Softmax")])
    b += step(MLP_Y + MLP_H / 2, "多层感知机", [("t", "矩阵乘法")])
    b += step(ATT_Y + ATT_H / 2, "注意力块", [("t", "矩阵乘法、点积、Softmax")])
    b += step(EMB_Y + EMB_H / 2, "嵌入",
              [("t", "查表，取 "), ("m", "W_E"), ("t", " 中对应的列")])
    return b


def attention():
    """One band across the lanes; arcs carry each position to the later ones."""
    b = [rect(BL, ATT_Y, BR - BL, ATT_H, FILL_BLUE, BLUE, rx=6, width=1.6)]
    base = ATT_Y + ATT_H - 12
    for x in LANES:
        b.append(line(x, ATT_Y, x, ATT_Y + ATT_H, LINE, 1.6))
    pairs = [(i, j) for i in range(len(LANES)) for j in range(i + 1, len(LANES))]
    for i, j in pairs:
        outs = [t for s_, t in pairs if s_ == i]
        ins = [s_ for s_, t in pairs if t == j]
        # the longer an arc, the nearer its ends to the lanes: arcs of one
        # lane nest instead of crossing
        sx = LANES[i] + 3 + 10 * (len(outs) - 1 - outs.index(j))
        tx = LANES[j] - 3 - 10 * ins.index(i)
        b.append(curve(sx, base, tx, base, bend=-18 - 18 * (j - i), color=INK,
                       width=1.5))
    b.append(label(DIFF_X, ATT_Y + ATT_H / 2 + 6, "位置之间传递信息", SUB, INK,
                   anchor="start"))
    return b


def mlp():
    """One box per lane, nothing between them."""
    b = []
    for x in LANES:
        b.append(rect(x - MLP_W / 2, MLP_Y, MLP_W, MLP_H, FILL_BLUE, BLUE, rx=6, width=1.6))
    b.append(label(DIFF_X, MLP_Y + MLP_H / 2 + 6, "每个位置单独计算", SUB, INK,
                   anchor="start"))
    return b


def stack():
    b = []
    for x in LANES:
        b.append(line(x, V3Y + NV * VC, x, CHIP_Y - 4, LINE, 1.6))
    b.append(label(LANES[0] - 48, CHIP_Y + 23, "⋯", 20, MUTED))
    for x, t in zip(LANES, TOKS):
        els, _ = chip(x, CHIP_Y, t, size=18, h=32, anchor="middle")
        b += els
    # the embedding matrix, every lane passes through it
    b += weights(BL, EMB_Y, EMB_R, 22, cw=(BR - BL) / 22, ch=EMB_H / EMB_R, seed="ov-WE")
    b += math(BR + 12, EMB_Y + EMB_H / 2 + 8, "W_E", 22, anchor="start")
    b += attention()
    b += mlp()
    # one layer, repeated
    lo, hi = MLP_Y, ATT_Y + ATT_H
    b.append(path(f"M {REP_X} {lo} L {REP_X + 8} {lo} L {REP_X + 8} {hi} L {REP_X} {hi}",
                  MUTED, 1.4, head=False))
    b.append(text(REP_X + 16, (lo + hi) / 2 + 6, "重复多层", SUB, MUTED, anchor="start"))
    # the vectors
    for y in (V1Y, V3Y):
        for k, x in enumerate(LANES):
            last = y == V3Y and k == len(LANES) - 1
            b += mini(x, y, f"ov{y}:{k}", ORANGE if last else DATA, 2.4 if last else 1.4)
    for x in LANES:
        b.append(rect(x - 8, DOTS_Y - 14, 16, 22, WHITE, "none", rx=0, width=0))
        b.append(label(x, DOTS_Y + 4, "⋮", 18, MUTED))
    return b


def output():
    b = []
    ux = RC - UW_C * UC / 2
    vy = V3Y + NV * VC / 2
    uy = vy - UW_R * UC / 2
    b.append(arrow(LANES[-1] + VW / 2 + 6, vy, ux - 6, vy, ORANGE, 2.2))
    b += weights(ux, uy, UW_R, UW_C, cw=UC, ch=UC, seed="ov-WU")
    b += math(ux + UW_C * UC + 10, vy + 8, "W_U", 22, anchor="start")
    top = BASE - HMAX - 8
    b.append(arrow(RC, uy + UW_R * UC + 4, RC, top, INK, 2))
    b.append(text(RC + 12, (uy + UW_R * UC + top) / 2 + 6, "Softmax", SUB, INK,
                  anchor="start"))
    # the distribution over all tokens: one bar per token, a few of them shown
    pitch = 5
    x0 = RC - NBAR * pitch / 2
    vals = []
    for i in range(NBAR):
        h = hashlib.md5(f"ov-vocab:{i}".encode()).digest()[0] / 255.0
        vals.append(0.25 * h ** 8)
    vals[5], vals[11], vals[18], vals[23] = 1.0, 0.45, 0.3, 0.18
    b.append(line(x0 - 6, BASE, x0 + NBAR * pitch + 2, BASE, LINE, 1.4))
    for i, v in enumerate(vals):
        hgt = max(1.5, HMAX * v)
        b.append(rect(x0 + i * pitch, BASE - hgt, 4, hgt, BLUE, "none", rx=0, width=0))
    b.append(label(RC, BASE + 26, "所有 token 的概率分布", SUB, INK))
    b.append(arrow(RC, BASE + 38, RC, CHIP_Y - 6, INK, 2))
    b.append(label(RC + 12, (BASE + 38 + CHIP_Y) / 2 + 6, "采样", SUB, INK, anchor="start"))
    els, _ = chip(RC, CHIP_Y, "woodland", "upd", size=18, h=32, weight="bold",
                  anchor="middle")
    b += els
    return b


b = labels() + stack() + output()
save("dataflow-overview", b, H)
