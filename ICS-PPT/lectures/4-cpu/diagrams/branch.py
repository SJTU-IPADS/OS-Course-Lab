#!/usr/bin/env python3
"""jl decides at the end of execute; two guessed instructions are already in.

Rows are jl and the two instructions fetched after it on a guess. Columns are
cycles. The dashed orange line at the end of cycle 3 is the moment the branch
direction is known; the red frame holds what the pipeline has fetched before
that moment. The frame's right side is that line, so it is drawn open. Cycle 4 shows "?" cells: whether they continue depends on the
guess. No fixed number of squashed instructions is given. The grid comes from
timegrid.py so this figure lines up with pipeline / forward / stall.

An animation in four frames: cycles 1-2; cycle 3 (jl executes, two guessed
instructions are in); the decision at the end of cycle 3 with the red frame
round what was fetched before it; cycles 4-5 with the "?" cells and the two
outcomes. The last frame is the whole figure.
Run it to refresh ../assets/branch-1.svg .. branch-4.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import MUTED, ORANGE, RED, line, path, save, text
from timegrid import CH, CW, LX, TOP, VIEW_LEFT, cycle_numbers, row

W, H = 1120, 350
GAP = 22                            # looser than pipeline: three rows, two notes
R2, R3 = TOP + CH + GAP, TOP + 2 * (CH + GAP)
XD = LX + 3 * CW                    # end of cycle 3: jl has just executed


FRAMES = [(2, False), (3, False), (3, True), (5, True)]   # (cycles shown, decided)


def build(upto, decided):
    out = cycle_numbers(5)
    out += row("jl .L3", TOP, 0, ["取指", "译码", "执行", "访存", "写回"], upto)
    out += row("猜来的第 1 条", R2, 1, ["取指", "译码", "?"], upto)
    out += row("猜来的第 2 条", R3, 2, ["取指", "?"], upto)
    if not decided:
        return out
    out.append(text(XD, 22, "周期 3 结束：jl 执行完，才知道跳不跳", 16, ORANGE, "bold"))
    # what is already in the pipeline when jl decides: rows 2-3, cycles 2-3.
    # Top, left and bottom sides only: the right side would sit 1 px from the
    # decision line with the same dash and read as red-orange speckle, so
    # the orange line, drawn after it, closes the frame.
    x0, y0, y1, r = LX + CW + 1, R2 - 5, R3 + CH + 5, 6
    out.append(path(f"M {XD - 1} {y0} L {x0 + r} {y0} Q {x0} {y0} {x0} {y0 + r} "
                    f"L {x0} {y1 - r} Q {x0} {y1} {x0 + r} {y1} L {XD - 1} {y1}",
                    RED, 2, dash="6 4", head=False))
    # the decision point
    out.append(line(XD, TOP - 8, XD, R3 + CH + 8, ORANGE, 2, dash="6 4"))
    out.append(text(LX + 2 * CW, R3 + CH + 40,
                    "jl 判定前，后面几级里已经取进来的指令：猜错就作废", 16, RED, "bold"))
    if upto < 5:
        return out
    # the two outcomes, centred between the two guessed rows
    out.append(text(LX + 4 * CW + 10, 195, "猜对：继续执行", 15, MUTED, anchor="start"))
    out.append(text(LX + 4 * CW + 10, 215, "猜错：全部作废", 15, RED, "bold", anchor="start"))
    return out


if __name__ == "__main__":
    for k, (upto, decided) in enumerate(FRAMES, 1):
        save(f"branch-{k}", W, H, build(upto, decided), left=VIEW_LEFT)
