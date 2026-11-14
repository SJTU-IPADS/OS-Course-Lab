#!/usr/bin/env python3
"""chat: one prediction loop, used for stories and for chat.

1: a seed text and the loop predict -> sample -> append; the appended
   tokens (green) make the text grow.
2: + a chat: the same loop continues a text made of a system prompt, the
   user's question and the start of the assistant's reply.

The examples are parallel, so the figure is drawn once, in its final state;
frame() keeps the step numbers only to say which parts are drawn.
"""
import math as m

from tk import *

H = 400
P1 = "What follows is a conversation between a user and a helpful, very knowledgeable AI assistant."
P2 = "User: Give me some ideas for what to do when visiting Paris."
P3 = "AI Assistant: "


def pill(cx, cy, s, color=INK, fill=WHITE):
    w = width(s, 18) + 28
    return [rect(cx - w / 2, cy - 17, w, 34, fill, color, rx=17, width=1.6),
            label(cx, cy + 6.5, s, 18, color, "bold")]


def arc(cx, cy, r, a0, a1, color=INK):
    """A clockwise arc of the circle from angle a0 to a1 (degrees, y down)."""
    x0, y0 = cx + r * m.cos(m.radians(a0)), cy + r * m.sin(m.radians(a0))
    x1, y1 = cx + r * m.cos(m.radians(a1)), cy + r * m.sin(m.radians(a1))
    return path(f"M {x0:.1f} {y0:.1f} A {r} {r} 0 0 1 {x1:.1f} {y1:.1f}", color, 2.2)


def frame(n):
    b = []
    # the growing text
    tx, ty, tw, th = 40, 20, 440, 108
    b.append(rect(tx, ty, tw, th, FILL_DATA, DATA, rx=6, width=1.4))
    b.append(label(tx + 16, ty + 30, "Behold, a wild pi creature,", 19, INK, anchor="start"))
    b.append(f'<text x="{tx + 16:.1f}" y="{ty + 60:.1f}" font-family="{FONT}" font-size="19" '
             f'fill="{INK}" text-anchor="start">foraging in its native'
             f'<tspan fill="{GREEN}" font-weight="bold"> woodland</tspan></text>')
    gx = tx + 16
    for w in (54, 38, 62):
        b.append(rect(gx, ty + 76, w, 22, FILL_GREEN, GREEN, rx=4, width=1.4, dash="4 3"))
        gx += w + 8
    b.append(label(gx + 10, ty + 93, "⋯", 19, GREEN))

    # the loop
    cx, cy, r = 260, 274, 80
    nodes = {"预测": -90, "采样": 30, "追加": 150}
    gap = 26
    b.append(arc(cx, cy, r, -90 + gap, 30 - gap))
    b.append(arc(cx, cy, r, 30 + gap, 150 - gap))
    b.append(arc(cx, cy, r, 150 + gap, 270 - gap))
    for s, a in nodes.items():
        px, py = cx + r * m.cos(m.radians(a)), cy + r * m.sin(m.radians(a))
        b += pill(px, py, s, GREEN if s == "追加" else INK,
                  FILL_GREEN if s == "追加" else WHITE)
    b.append(arrow(cx, ty + th + 4, cx, cy - r - 21, INK, 2))
    ax = cx + r * m.cos(m.radians(150)) - 34
    ay = cy + r * m.sin(m.radians(150))
    b.append(elbow([(ax, ay), (tx + 22, ay), (tx + 22, ty + th + 4)], GREEN, 2))

    if n >= 2:
        bx, by, bw = 540, 0, 340
        pad, size, lh = 14, 17, 24
        rows, y = [], by + pad + 16
        spans = []
        for para in (P1, P2):
            els, hgt = wrap(bx + pad, y, para, size, bw - 2 * pad, lh)
            rows += els
            spans.append((y - 17, y + hgt - lh + 7))
            y += hgt + 12
        rows.append(f'<text x="{bx + pad:.1f}" y="{y:.1f}" font-family="{FONT}" font-size="{size}" '
                    f'fill="{INK}" text-anchor="start">{esc(P3)}'
                    f'<tspan fill="{GREEN}" font-weight="bold">▍</tspan></text>')
        spans.append((y - 17, y + 7))
        bh = y + 7 + pad - by
        shift = (H - bh) / 2 - by          # centre the block vertically
        b.append(f'<g transform="translate(0 {shift:.1f})">')
        b.append(rect(bx, by, bw, bh, FILL_DATA, DATA, rx=6, width=1.4))
        b += rows
        for (y0, y1), s, c in zip(spans, ["系统提示词", "用户的问题", "模型从这里续写"],
                                  [INK, INK, GREEN]):
            b += vbrace(bx + bw + 10, y0, y1, c, s, size=17)
        b.append("</g>")
    save("chat", b, H)


frame(2)
