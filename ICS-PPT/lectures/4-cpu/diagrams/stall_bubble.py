#!/usr/bin/env python3
"""What stall and bubble do to the pipeline registers, cycle 3 to cycle 4.

Three snapshots of the five stages, one row each, with the pipeline registers
drawn as narrow bars in front of every stage (the first one is the PC). A stage
box names the instruction it works on in that cycle.

  cycle 3                 addl in decode reads the old %ecx (4); imull executes.
  cycle 4, stall only     PC and the fetch/decode register keep their contents,
                          so addl stays in decode; the decode/execute register
                          loads decode's output as usual and a copy of addl,
                          carrying the old value, enters execute (red).
  cycle 4, stall+bubble   the same, except that the decode/execute register
                          loads an empty instruction: execute does nothing.

The stage colours are timegrid.py's; the bubble is drawn like timegrid's dashed
"?" cell, empty and white. Run it to refresh ../assets/stall-bubble.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_RED, INK, MONO, MUTED, RED, WHITE, arrow, rect, save, text)
from timegrid import TONE

W, H = 1120, 304
LEFT = -44                          # centres the ink (x 24..1008) on the canvas
RW, BW, BH = 14, 128, 54            # register bar width, stage box, their height
X0 = 180                            # the PC bar
STEP = RW + 6 + BW + 22             # one register + one stage + the arrow after it
TOPS = [8, 114, 220]                # box tops of the three snapshots
STAGES = ["取指", "译码", "执行", "访存", "写回"]
ADDL, NEXT = "addl %ecx,%r9d", "addl $1,%eax"
HOLD = {0: ("stall", INK), 1: ("stall", INK), 2: ("bubble", INK)}   # one stall's marks

SNAPSHOTS = [                       # (label lines, instruction per stage, bar labels)
    (["周期 3"], [NEXT, ADDL, "imull", "…", "…"], {}),
    (["周期 4", "只 stall"], [NEXT, ADDL, ADDL + "!", "imull", "…"],
     {0: ("stall", INK), 1: ("stall", INK), 2: ("照常装入", RED)}),
    (["周期 4", "stall + bubble"], [NEXT, ADDL, None, "imull", "…"],
     {0: ("stall", INK), 1: ("stall", INK), 2: ("bubble", INK)}),
]


def bar_x(k):
    return X0 + k * STEP


def box_x(k):
    return bar_x(k) + RW + 6


def stage(k, top, insn):
    """One stage box: the stage name, then the instruction (None: a bubble,
    '?': a stage that does not know which instruction it holds, a trailing
    '!': the copy of addl that should not be there)."""
    name = STAGES[k]
    fill, stroke, tone = TONE[name]
    x, mid = box_x(k), box_x(k) + BW / 2
    if insn is None or insn == "?":  # white box: 写回's white label would vanish
        return [rect(x, top, BW, BH, WHITE, MUTED, rx=4, width=1.6, dash="5 4"),
                text(mid, top + 21, name, 15, INK if tone == WHITE else tone, "bold"),
                text(mid, top + 42, "bubble", 15, MUTED, "bold") if insn is None
                else text(mid, top + 45, "?", 20, MUTED, "bold")]
    if insn.endswith("!"):
        return [rect(x, top, BW, BH, FILL_RED, RED, rx=4, width=2.4),
                text(mid, top + 21, name, 15, tone, "bold"),
                text(mid, top + 42, insn[:-1], 13, RED, "bold", font=MONO)]
    ink = WHITE if name == "写回" else (MUTED if insn == "…" else INK)
    return [rect(x, top, BW, BH, fill, stroke, rx=4, width=1.4),
            text(mid, top + 21, name, 15, tone, "bold"),
            text(mid, top + 42, insn, 13, ink, font=MONO)]


def snapshot(top, lines, insns, marks):
    out = []
    y0 = top + BH / 2 + 6 - 11 * (len(lines) - 1)
    for i, s in enumerate(lines):
        out.append(text(24, y0 + 22 * i, s, 16, INK, "bold" if i == 0 else "normal",
                        anchor="start"))
    for k in range(5):
        held = k in marks and marks[k][0] == "stall"
        out.append(rect(bar_x(k), top, RW, BH, WHITE, INK if held else MUTED, rx=2,
                        width=2.4 if held else 1.4))
        if k in marks:
            label, color = marks[k]
            out.append(text(bar_x(k) + RW / 2, top - 9, label, 15, color, "bold"))
        out += stage(k, top, insns[k])
        if k < 4:
            out.append(arrow(box_x(k) + BW + 1, top + BH / 2, bar_x(k + 1) - 2, top + BH / 2,
                             MUTED, 1.6))
    out.append(text(bar_x(0) + RW / 2, top + BH + 18, "PC", 13, MUTED))
    return out


def group(top, k0, k1):
    """A grey dashed frame around stages k0..k1 and the bars between them,
    like the frame branch.py draws on the grid."""
    x0, x1 = box_x(k0) - 4, box_x(k1) + BW + 4      # clear of the PC label at the left
    return rect(x0, top - 5, x1 - x0, BH + 10, "none", MUTED, rx=6, width=2, dash="6 4")


def cycle_snapshot(top, cycle, insns, marks, notes):
    """The snapshot of one cycle under a time-space grid (no_forward.py,
    stall.py); `notes` maps a stage to (text, colour) written under its box."""
    out = snapshot(top, [f"周期 {cycle}"], insns, marks)
    for k, (s, color) in notes.items():
        out.append(text(box_x(k) + BW / 2, top + BH + 21, s, 15, color, "bold"))
    return out


def build():
    out = []
    for top, (lines, insns, marks) in zip(TOPS, SNAPSHOTS):
        out += snapshot(top, lines, insns, marks)
    out.append(text(box_x(1) + BW / 2, TOPS[0] + BH + 21, "读到 %ecx 的旧值 4", 15, RED, "bold"))
    out.append(text(box_x(2) + BW / 2, TOPS[1] + BH + 21, "addl 带着旧值 4 进入执行级", 15, RED,
                    "bold"))
    out.append(text(box_x(2) + BW / 2, TOPS[2] + BH + 21, "执行级不做任何操作", 15, MUTED))
    return out


if __name__ == "__main__":
    save("stall-bubble", W, H, build(), left=LEFT)
