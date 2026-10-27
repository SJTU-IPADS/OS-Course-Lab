#!/usr/bin/env python3
"""One block of combinational logic and one clocked register, with their times.

ICS1 lecture 17, page 4, "Computational Example", shape for shape: the
original is PowerPoint shapes, so every box, label, arrow and line is redrawn
here at the coordinates the slide gives (points, group origin at the top
left), scaled to the slide's width. Only the colours follow this deck:
combinational logic blue and the clocked register orange, as in
components.py. Nothing is added or dropped; the text is translated.
The canvas is the original group, which is wider than its ink (the text boxes
on the right are wider than their text), and the page centres the canvas; so
save(..., left=centring(INK_X, ...)) moves the window to put the ink in the
middle of the canvas. pipe_3way.py and pipe_diagram.py do the same.
Run it to refresh ../assets/comp-example.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, FILL_BLUE, FILL_ORANGE, INK, ORANGE, arrow, line, rect, save, text

S = 1.35                                   # px per point of the original group
W, H = 645, 208                            # the original group, in points
LOGIC = (44, 31, 237, 116)                 # (x, y, w, h), points
REG = (325, 31, 20, 116)
ARROW_Y = 85
REG_LABEL = "寄存器"                       # "Reg", one character per line
INK_X = (0, 642)                           # x extent of the ink in px, measured on the
                                           # rendered slide: the input arrow .. the end
                                           # of "吞吐率 = 3.12 GOPS"


def pt(v):
    return v * S


def centring(ink, w):
    """The `left` of save() that puts the ink [ink[0], ink[1]] in the middle of a w-px canvas."""
    return (ink[0] + ink[1]) / 2 - w / 2


def register(x, y, w, h, size=15):
    """A narrow orange box with its label written downwards."""
    out = [rect(pt(x), pt(y), pt(w), pt(h), FILL_ORANGE, ORANGE, rx=3, width=1.6)]
    cy = pt(y + h / 2) - (len(REG_LABEL) - 1) * (size + 2) / 2
    for k, ch in enumerate(REG_LABEL):
        out.append(text(pt(x + w / 2), cy + k * (size + 2) + size * 0.36, ch, size, INK,
                        "bold"))
    return out


def build():
    x, y, w, h = LOGIC
    out = [rect(pt(x), pt(y), pt(w), pt(h), FILL_BLUE, BLUE, rx=6, width=1.6),
           text(pt(x + w / 2), pt(y + h / 2) + 6, "组合逻辑", 19, INK, "bold"),
           text(pt(x + w / 2), pt(24), "300 ps", 18, INK)]                 # above the logic
    rx, ry, rw, rh = REG
    out += register(rx, ry, rw, rh)
    out.append(text(pt(rx + rw / 2), pt(24), "20 ps", 18, INK))             # above the register
    out.append(arrow(pt(0), pt(ARROW_Y), pt(x) - 2, pt(ARROW_Y), INK, 2.2))
    out.append(arrow(pt(x + w) + 2, pt(ARROW_Y), pt(rx) - 2, pt(ARROW_Y), INK, 2.2))
    cx = pt(rx + rw / 2)
    out.append(line(cx, pt(ry + rh), cx, pt(168), INK, 1.6))                # the clock input
    out.append(text(cx, pt(197), "时钟", 18, INK))
    out.append(text(pt(358), pt(86), "延迟 = 320 ps", 18, INK, anchor="start"))
    out.append(text(pt(358), pt(112), "吞吐率 = 3.12 GOPS", 18, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("comp-example", pt(W), pt(H), build(), left=centring(INK_X, pt(W)))
