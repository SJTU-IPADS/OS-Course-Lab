#!/usr/bin/env python3
"""Why the cycle is not five times shorter: the slowest stage, and the registers.

Left: three stages of unequal work inside one dashed cycle frame; the cycle is
as long as the middle one, the other two finish early and idle. Right: a
pipeline register after every stage, then 3 stages against 6: the work per
stage halves, the register time does not. The proportions illustrate CS:APP
4.4's arithmetic pipeline, not a measured core. The conclusions are said by
the page's bullets and highlight, so the figure carries only what can be read
off it. Run it to refresh ../assets/pipe-limit.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, LINE, ORANGE, WHITE, box, brace,
                    rect, save, text)

W, H = 1120, 290
CH = 44                      # height of every bar and box
STAGE_W, REG_W = 90, 60      # a stage box and the register after it (chain 645..1095)
STAGES = "甲乙丙"


def left():
    """Three stages of unequal work; the cycle waits for the slowest."""
    out = [text(290, 30, "三级的工作量不一样", 18, INK, "bold")]
    x0, cycle, work = 100, 360, (147, 360, 147)
    for i, w in enumerate(work):
        y = 62 + i * 56
        slow = w == cycle
        out.append(text(76, y + CH / 2 + 6, STAGES[i], 18, INK, "bold"))
        out.append(rect(x0, y, cycle, CH, WHITE, LINE, rx=3, width=1.4, dash="4 3"))
        out.append(rect(x0, y, w, CH, FILL_ORANGE if slow else FILL_BLUE,
                        ORANGE if slow else BLUE, rx=3, width=1.6))
        if not slow:
            out.append(text(x0 + w + (cycle - w) / 2, y + CH / 2 + 6, "空等", 16, INK))
    out += brace(x0, x0 + cycle, 228, ORANGE, "一个周期 = 乙的时间", 16, depth=8)
    return out


def right():
    """A register after every stage; doubling the stages halves the work, not the register."""
    out = [text(850, 30, "级与级之间要加寄存器", 18, INK, "bold")]
    x, y = 645, 62
    for name in STAGES:
        out += box(x, y, STAGE_W, CH, name, FILL_BLUE, BLUE, 18)
        x += STAGE_W
        out += box(x, y + 4, REG_W, CH - 8, "寄存器", FILL_ORANGE, ORANGE, 15, "bold", ORANGE)
        x += REG_W
    for k, (label, work) in enumerate((("3 级", 240), ("6 级", 120))):
        y = 140 + k * 56
        out.append(text(690, y + CH / 2 + 6, label, 16, INK, "bold", anchor="end"))
        out.append(rect(700, y, work, CH, FILL_BLUE, BLUE, rx=3, width=1.6))
        out.append(text(700 + work / 2, y + CH / 2 + 6, "每级的工作", 15, BLUE))
        out.append(rect(700 + work, y, REG_W, CH, FILL_ORANGE, ORANGE, rx=3, width=1.6))
        out.append(text(700 + work + REG_W / 2, y + CH / 2 + 6, "寄存器", 15, ORANGE, "bold"))
    out.append(text(850, 268, "级数翻倍，每级的工作减半，寄存器的时间不变", 16, ORANGE, "bold"))
    return out


if __name__ == "__main__":
    # the drawing starts at x=67 and ends at 1095: move the window 21 px right
    # so the slide shows it centred
    save("pipe-limit", W, H, left() + right(), left=21)
