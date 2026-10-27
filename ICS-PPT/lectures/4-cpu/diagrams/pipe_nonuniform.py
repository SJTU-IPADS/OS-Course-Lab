#!/usr/bin/env python3
"""Three blocks of unequal time, 50, 150 and 100 ps, a 20 ps register after each.

ICS1 lecture 17, page 9, "Limitations: Nonuniform Delays", element for
element, in the units and scale of pipe_3way.py and pipe_diagram.py. Two
figures, because the original page has two groups:

pipe-nonuniform.svg is the circuit: the blocks A, B, C and their registers,
the times above them, the clock lines joined to one line, 「延迟」 and
「吞吐率」 to the right. A block's width is its time at the 74 points per
100 ps of pipe_3way.py; registers, gaps, heights, the clock line, the
canvas and INK_X are pipe_3way.py's, so a 100 ps block here is the size of
a block on that page, and since 50 + 150 = 2 x 100, C and the last two
registers fall where they fall there. A is narrow, so
「组合逻辑」 is written on two lines above the letter in all three blocks,
as the original writes "Comb. logic" on two.

pipe-nonuniform-time.svg is OP1..OP3 against time: each stage occupies one
170 ps clock cycle, drawn as an empty cell, and the block's own time is a
blue cell at the left of it, so the blank remainder is the time the block
waits for the clock edge. Widths are proportional at the 48 points per
120 ps of pipe_diagram.py's three-stage figure; OP labels, rows and the clock
row below them are drawn as there. The original's cells are only roughly
proportional (1/3, 1 and 2/3 of a cycle); these are exact. The clock, drawn
by pipe_diagram.clock_row where the original has its time axis, has one
period per 170 ps cycle from OP1's first cycle to OP3's last, its rising
edges (the registers load on them) on the cell boundaries, and a dashed line
on each edge up through the rows.

Colours as in components.py: combinational logic blue, clocked registers
orange. The text is translated. Each figure is centred on the canvas with
comp_example.centring.
Run it to refresh ../assets/pipe-nonuniform.svg and pipe-nonuniform-time.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from comp_example import centring, pt, register
from pipe_3way import ARROW_Y, BH, CLOCK_Y, H, INK_X, LOGIC_X, LW, REG_X, RW, TOP, W
from pipe_diagram import CELL, CLOCK_ROW, OPS, RH, ROW_X, clock_row, label
from svgkit import BLUE, FILL_BLUE, FONT, INK, WHITE, arrow, cells, line, rect, save, text

TIMES = (50, 150, 100)                     # ps of the blocks A, B, C
REG_PS = 20
CYCLE = max(TIMES) + REG_PS                # 170 ps
STAGES = "ABC"

# The circuit, in points; W, H and INK_X are pipe_3way.py's, whose
# 「吞吐率 = 8.33 GOPS」 is as wide as 「吞吐率 = 5.88 GOPS」.
PER_PS = LW / 100                          # 74 points per 100 ps
OUT_GAP = REG_X[0] - LOGIC_X[0] - LW       # block .. its register, as in pipe_3way.py
IN_GAP = LOGIC_X[1] - REG_X[0] - RW        # register .. the next block

# The time diagram, in points.
CELL_PS = CELL / 120                       # pipe_diagram.py: a 120 ps cycle is 48 points
SLOT = CYCLE * CELL_PS                     # one 170 ps cycle
TW = ROW_X + 5 * SLOT                      # OP1's first cycle .. OP3's last
TIME_INK_X = (42, pt(TW))                  # 「时钟」 .. the last cell, in px


def layout():
    """x of each block and of the register after it, left to right."""
    out, x = [], LOGIC_X[0]
    for ps in TIMES:
        w = ps * PER_PS
        out.append((x, w, x + w + OUT_GAP))
        x = x + w + OUT_GAP + RW + IN_GAP
    return out


def circuit():
    out = []
    before = 0
    for k, (lx, lw, rx) in enumerate(layout()):
        out.append(rect(pt(lx), pt(TOP), pt(lw), pt(BH), FILL_BLUE, BLUE, rx=6, width=1.6))
        cx, cy = pt(lx + lw / 2), pt(TOP + BH / 2)
        for dy, s in ((-16, "组合"), (6, "逻辑"), (28, STAGES[k])):
            out.append(text(cx, cy + dy, s, 17, INK, "bold"))
        out.append(text(cx, pt(24), f"{TIMES[k]} ps", 18, INK))
        out += register(rx, TOP, RW, BH)
        out.append(text(pt(rx + RW / 2), pt(24), f"{REG_PS} ps", 18, INK))
        out.append(arrow(pt(before) + (2 if k else 0), pt(ARROW_Y), pt(lx) - 2, pt(ARROW_Y),
                         INK, 2.2))
        out.append(arrow(pt(lx + lw) + 2, pt(ARROW_Y), pt(rx) - 2, pt(ARROW_Y), INK, 2.2))
        drop = 168 if k == 2 else CLOCK_Y
        out.append(line(pt(rx + RW / 2), pt(TOP + BH), pt(rx + RW / 2), pt(drop), INK, 1.6))
        before = rx + RW
    regs = [rx for _, _, rx in layout()]
    out.append(line(pt(regs[0] + RW / 2), pt(CLOCK_Y), pt(regs[2] + RW / 2), pt(CLOCK_Y),
                    INK, 1.6))
    out.append(text(pt(498), pt(186), "时钟", 18, INK))
    out.append(text(pt(512), pt(84), f"延迟 = {3 * CYCLE} ps", 18, INK, anchor="start"))
    out.append(text(pt(512), pt(110), f"吞吐率 = {1000 / CYCLE:.2f} GOPS", 18, INK,
                    anchor="start"))
    return out


def timeline():
    """Row k starts one cycle after row k - 1; each cycle is the block's time, then blank."""
    pad = 4                                # room for the cycle lines above the first row
    out = []
    for k, name in enumerate(OPS):
        y = pad + k * RH
        out.append(label(name, y))
        for j, ps in enumerate(TIMES):
            x = pt(ROW_X + (k + j) * SLOT)
            slot, _ = cells(x, pt(y), [""], pt(SLOT), pt(RH), WHITE, BLUE)
            work, _ = cells(x, pt(y), [STAGES[j]], pt(ps * CELL_PS), pt(RH), FILL_BLUE, BLUE,
                            size=17, font=FONT)
            out += slot + work
    out += clock_row([ROW_X + j * SLOT for j in range(6)], pad, pad + 3 * RH)
    return out, pad + 3 * RH + CLOCK_ROW


if __name__ == "__main__":
    save("pipe-nonuniform", pt(W), pt(H), circuit(), left=centring(INK_X, pt(W)))
    body, h = timeline()
    save("pipe-nonuniform-time", pt(TW), pt(h), body, left=centring(TIME_INK_X, pt(TW)))
