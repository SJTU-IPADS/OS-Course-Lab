#!/usr/bin/env python3
"""Six 50 ps logic blocks, each followed by a 20 ps register, on one clock.

Element for element after ICS1 lecture 17 page 12 "Limitations: Register
Overhead" (CS:APP 4.4's six-stage pipeline): the time of every block above
it, an arrow into every block, the clock line joining the registers from
below with its label under the first register, and the two results on one
line at the lower right. Only the colours follow this deck: combinational logic and
clocked registers take the fills of components.py.
Run it to refresh ../assets/reg-overhead.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, FILL_BLUE, FILL_ORANGE, INK, ORANGE, arrow, line, rect, save, text

W, H = 1120, 248
X0, PITCH = 76, 175          # the first block starts here; one block + register per pitch
LOGIC_W, REG_W = 66, 24      # a logic block and the register after it
GAP = 42                     # the arrow between two boxes
TOP, BOX_H = 46, 136         # the boxes, as tall as pipe_3way.py's
LABEL_Y = 30                 # the times above the boxes
CLOCK_Y = 212                # the clock line under the registers


def unit(x):
    """One logic block with its register, the arrows into each, the clock tap."""
    mid = TOP + BOX_H / 2
    rx = x + LOGIC_W + GAP
    out = [text(x + LOGIC_W / 2, LABEL_Y, "50 ps", 17, INK),
           text(rx + REG_W / 2, LABEL_Y, "20 ps", 17, INK),
           arrow(x - GAP + 2, mid, x - 2, mid, INK, 2.4),
           rect(x, TOP, LOGIC_W, BOX_H, FILL_BLUE, BLUE, rx=4, width=1.6),
           text(x + LOGIC_W / 2, mid - 6, "组合", 16, INK, "bold"),
           text(x + LOGIC_W / 2, mid + 16, "逻辑", 16, INK, "bold"),
           arrow(x + LOGIC_W + 2, mid, rx - 2, mid, INK, 2.4),
           rect(rx, TOP, REG_W, BOX_H, FILL_ORANGE, ORANGE, rx=3, width=1.6),
           line(rx + REG_W / 2, TOP + BOX_H, rx + REG_W / 2, CLOCK_Y, INK, 1.6)]
    for k, c in enumerate("寄存器"):
        out.append(text(rx + REG_W / 2, mid - 12 + k * 20, c, 15, INK, "bold"))
    return out


def draw():
    out = []
    for i in range(6):
        out += unit(X0 + i * PITCH)
    first = X0 + LOGIC_W + GAP + REG_W / 2
    last = first + 5 * PITCH
    out.append(line(first, CLOCK_Y, last, CLOCK_Y, INK, 1.6))
    out.append(text(first + 8, CLOCK_Y + 30, "时钟", 18, INK, anchor="start"))
    out.append(text(560, CLOCK_Y + 30, "延迟 = 420 ps，吞吐率 = 14.29 GOPS", 18, INK,
                    anchor="start"))
    return out


if __name__ == "__main__":
    save("reg-overhead", W, H, draw())
