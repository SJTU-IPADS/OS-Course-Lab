#!/usr/bin/env python3
"""The figure part-5.md asks for on its page 4: the G, R, S bits and the gates.

Left, where the bits sit: the kept significand ends in LSB; the first bit cut
off is the guard bit G, the next the round bit R, and S is the OR of every bit
after that.  Right, the decision as gates: G and not R and not S is the exact
midpoint; the midpoint with an odd LSB, or G with R or S set (past the
midpoint), drives the carry into the significand.  Together that is
G and (R or S or LSB).  Run it to refresh ../assets/round-grs.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, MONO, MUTED, ORANGE, brace,
                    line, rect, svg, text)

W, H = 940, 216
RED, GREEN = "#c0392b", "#196b24"
GW, GH = 46, 40        # gate body
WIRE = INK


# --------------------------------------------------------------------- gates

def _inputs(x, y, n, h):
    return [(x, y + h * (i + 1) / (n + 1)) for i in range(n)]


def and_gate(x, y, n, inverted=(), h=GH):
    r = h / 2
    d = (f"M {x:.1f} {y:.1f} L {x + GW - r:.1f} {y:.1f} "
         f"A {r:.1f} {r:.1f} 0 0 1 {x + GW - r:.1f} {y + h:.1f} L {x:.1f} {y + h:.1f} Z")
    out = [f'<path d="{d}" fill="#fff" stroke="{INK}" stroke-width="1.6"/>']
    pins = []
    for i, (px, py) in enumerate(_inputs(x, y, n, h)):
        if i in inverted:
            out.append(f'<circle cx="{px - 4:.1f}" cy="{py:.1f}" r="4" fill="#fff" '
                       f'stroke="{INK}" stroke-width="1.4"/>')
            pins.append((px - 8, py))
        else:
            pins.append((px, py))
    return out, pins, (x + GW, y + h / 2)


def or_gate(x, y, n, h=GH):
    d = (f"M {x:.1f} {y:.1f} Q {x + GW * 0.3:.1f} {y + h / 2:.1f} {x:.1f} {y + h:.1f} "
         f"Q {x + GW * 0.72:.1f} {y + h:.1f} {x + GW:.1f} {y + h / 2:.1f} "
         f"Q {x + GW * 0.72:.1f} {y:.1f} {x:.1f} {y:.1f} Z")
    out = [f'<path d="{d}" fill="#fff" stroke="{INK}" stroke-width="1.6"/>']
    pins = [(px + GW * 0.13, py) for px, py in _inputs(x, y, n, h)]
    return out, pins, (x + GW, y + h / 2)


def wire(*pts, color=WIRE):
    d = " ".join(f"{x:.1f},{y:.1f}" for x, y in pts)
    return f'<polyline points="{d}" fill="none" stroke="{color}" stroke-width="1.6"/>'


def pin_label(pin, s, color=INK, length=26):
    x, y = pin
    return [wire((x - length, y), (x, y), color=color),
            text(x - length - 5, y + 4.5, s, 13, color, "bold", "end", font=MONO)]


# ------------------------------------------------------------------ register

def register():
    cw, ch, x0, y0 = 34, 30, 30, 62
    kept = ["", "", "", "…", "", "LSB"]
    cut = ["G", "R", "S"]
    out = []
    for i, v in enumerate(kept):
        x = x0 + i * cw
        out.append(rect(x, y0, cw, ch, FILL_BLUE, BLUE))
        if v:
            out.append(text(x + cw / 2, y0 + ch / 2 + 5, v, 12 if v == "LSB" else 13,
                            BLUE, "bold", font=MONO))
    split = x0 + len(kept) * cw
    for i, v in enumerate(cut):
        x = split + 8 + i * cw
        out.append(rect(x, y0, cw, ch, FILL_ORANGE, ORANGE))
        out.append(text(x + cw / 2, y0 + ch / 2 + 5, v, 14, ORANGE, "bold", font=MONO))
    out.append(line(split + 4, y0 - 12, split + 4, y0 + ch + 12, RED, 2, dash="4 3"))
    out.append(text(split + 4, y0 + ch + 28, "舍入位置", 12.5, RED, "bold"))
    out += brace(x0, split, y0 - 6, BLUE, "保留的尾数", 13, below=False)
    out += brace(split + 8, split + 8 + 3 * cw, y0 - 6, ORANGE, "被截断的部分", 13,
                 below=False)
    notes = [("LSB：保留部分的最低有效位", BLUE), ("G：保护位　R：舍入位", ORANGE),
             ("S：粘滞位，R 之后所有位的或", ORANGE)]
    for i, (s, c) in enumerate(notes):
        out.append(text(x0, y0 + ch + 58 + i * 19, s, 12.5, c, anchor="start"))
    return out


# ---------------------------------------------------------------------- logic

def logic():
    out = []
    # exact midpoint: G and not R and not S
    g1, p1, o1 = and_gate(500, 36, 3, inverted=(1, 2), h=54)
    out += g1
    for pin, s in zip(p1, ("G", "R", "S")):
        out += pin_label(pin, s)
    out.append(text(o1[0] + 6, 40, "精确中点", 13, GREEN, "bold", "start"))
    out.append(text(o1[0] + 6, 56, "G=1, R=S=0", 11.5, GREEN, anchor="start", font=MONO))
    # midpoint and odd LSB
    g2, p2, o2 = and_gate(680, 60, 2)
    out += g2
    out.append(wire(o1, (o1[0] + 20, o1[1]), (o1[0] + 20, p2[0][1]), p2[0]))
    out += pin_label(p2[1], "LSB", length=40)
    out.append(text(o2[0] - GW / 2, 52, "中点且末位为 1", 12, MUTED))
    # past the midpoint: G and (R or S)
    g3, p3, o3 = or_gate(500, 150, 2)
    out += g3
    for pin, s in zip(p3, ("R", "S")):
        out += pin_label(pin, s)
    g4, p4, o4 = and_gate(680, 150, 2)
    out += g4
    out += pin_label(p4[0], "G", length=40)
    out.append(wire(o3, (o3[0] + 20, o3[1]), (o3[0] + 20, p4[1][1]), p4[1]))
    out.append(text(o4[0] - GW / 2, 150 + GH + 18, "大于中点", 13, GREEN, "bold"))
    # either one carries
    g5, p5, o5 = or_gate(790, 105, 2)
    out += g5
    out.append(wire(o2, (o2[0] + 30, o2[1]), (o2[0] + 30, p5[0][1]), p5[0]))
    out.append(wire(o4, (o4[0] + 30, o4[1]), (o4[0] + 30, p5[1][1]), p5[1]))
    out.append(wire(o5, (o5[0] + 18, o5[1]), color=RED))
    out.append(f'<polygon points="{o5[0] + 26:.1f},{o5[1]:.1f} {o5[0] + 17:.1f},{o5[1] - 5:.1f} '
               f'{o5[0] + 17:.1f},{o5[1] + 5:.1f}" fill="{RED}"/>')
    out.append(text(o5[0] + 34, o5[1] - 4, "进位", 13, RED, "bold", "start"))
    out.append(text(o5[0] + 34, o5[1] + 13, "尾数 +1", 13, RED, "bold", "start"))
    return out


def build():
    return svg(W, H, register() + logic())


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "round-grs.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
