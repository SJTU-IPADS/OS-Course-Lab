#!/usr/bin/env python3
"""The three kinds of parts of the single-cycle processor, built up step by step.

After the "SEQ 组件" animation of ICS1 lecture 14 (pages 25-32): a legend of the
three kinds on the left, the parts on the right. Combinational logic is one
rounded block holding the ALU; the clocked registers are PC and the flags (CC
in the original); the memories are the memory and the register file. The last
frame adds the reads into the combinational logic and the writes out of it.
Original page 29 repeats page 28 and has no frame of its own.
components-full.svg is the finished parts without the legend, with the paths
inside the block (PC into the logic, the ALU through the flags), for the two
pages that list the kinds beside it (original pages 33 and 34).
Kinds take their own fills, after the original's cyan, beige and yellow: logic
blue, clocked registers orange, memories green.
Run it to refresh ../assets/components-1.svg .. components-7.svg and
../assets/components-full.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, ORANGE, WHITE, arrow,
                    elbow, rect, save, text)

W, H = 1120, 400
OX = 440                      # the parts start here in the animation
FW = 620                      # width of components-full.svg
KIND = {                      # kind -> (fill, stroke)
    "logic": (FILL_BLUE, BLUE),
    "reg": (FILL_ORANGE, ORANGE),
    "mem": (FILL_GREEN, GREEN),
}
LEGEND = [("组合逻辑", "logic", 80), ("时钟寄存器", "reg", 170), ("存储器", "mem", 260)]


def box(x, y, w, h, label, kind, size=17, rx=6):
    fill, stroke = KIND[kind]
    return [rect(x, y, w, h, fill, stroke, rx=rx, width=1.6),
            text(x + w / 2, y + h / 2 + size * 0.36, label, size, INK, "bold")]


def legend(kinds):
    out = []
    for name, kind, y in LEGEND:
        if kind in kinds:
            out.append(text(40, y + 7, name, 19, INK, "bold", anchor="start"))
            fill, stroke = KIND[kind]
            out.append(rect(210, y - 17, 72, 34, fill, stroke,
                            rx=14 if kind == "logic" else 3, width=1.6))
    return out


def logic(ox):
    fill, stroke = KIND["logic"]
    return [rect(ox + 20, 20, 240, 280, fill, stroke, rx=24, width=1.6),
            text(ox + 140, 54, "组合逻辑", 18, INK, "bold"),
            rect(ox + 60, 78, 160, 158, WHITE, BLUE, rx=12, width=1.4),
            text(ox + 140, 266, "ALU", 17, INK, "bold")]


def memories(ox):
    return box(ox + 380, 80, 160, 56, "内存", "mem") + box(ox + 380, 180, 160, 56,
                                                          "寄存器堆", "mem")


def registers(ox):
    return box(ox + 95, 138, 90, 38, "标志位", "reg", 15) + box(ox + 90, 340, 100, 40,
                                                              "PC", "reg")


def paths(ox):
    """Reads into the logic, writes out of it to the memories and to PC."""
    out = [arrow(ox + 378, 118, ox + 264, 118, INK, 2),
           text(ox + 320, 110, "读", 15, INK),
           arrow(ox + 378, 202, ox + 264, 202, INK, 2),
           arrow(ox + 378, 222, ox + 264, 222, INK, 2),
           text(ox + 320, 194, "读", 15, INK),
           elbow([(ox + 260, 40), (ox + 600, 40), (ox + 600, 360), (ox + 194, 360)], INK, 2),
           arrow(ox + 600, 98, ox + 544, 98, INK, 2),
           text(ox + 572, 90, "写", 15, INK),
           arrow(ox + 600, 198, ox + 544, 198, INK, 2),
           text(ox + 572, 190, "写", 15, INK)]
    return out


def inner(ox):
    """PC into the logic; the ALU result into the flags and on to the logic."""
    return [arrow(ox + 140, 338, ox + 140, 302, INK, 2),
            arrow(ox + 140, 234, ox + 140, 178, INK, 2),
            arrow(ox + 140, 136, ox + 140, 80, INK, 2)]


FRAMES = [                    # (legend kinds, parts drawn)
    ({"logic"}, []),
    ({"logic"}, [logic]),
    ({"logic", "mem"}, [logic]),
    ({"logic", "mem"}, [logic, memories]),
    ({"logic", "mem", "reg"}, [logic, memories]),
    ({"logic", "mem", "reg"}, [logic, memories, registers]),
    ({"logic", "mem", "reg"}, [logic, memories, registers, paths]),
]


if __name__ == "__main__":
    for k, (kinds, parts) in enumerate(FRAMES, 1):
        out = legend(kinds)
        for part in parts:
            out += part(OX)
        save(f"components-{k}", W, H, out)
    out = []
    for part in (logic, memories, registers, paths, inner):
        out += part(0)
    save("components-full", FW, H, out)
