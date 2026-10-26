#!/usr/bin/env python3
"""jl decides at the end of execute; two guessed instructions are already in.

Rows are jl and the two instructions fetched after it on a guess; columns are
cycles. The dashed orange line at the end of cycle 3 is the moment the branch
direction is known; the grey frame holds what the pipeline has fetched before
that moment. Its right side is that line, so it is drawn open. The grid comes
from timegrid.py so this figure lines up with pipeline / forward / stall.

Under the grid, the five stages in the cycle the frame shows, drawn with
stall_bubble.py's cycle_snapshot as on the data hazard pages and on
branch_wait.py's page. At the decision the snapshot frames the fetch and
decode stages the way the grid frames the two guessed rows. A right guess
changes nothing: every pipeline register loads as usual. A wrong guess is
undone with bubbles: the clock edge that ends cycle 3 loads a bubble into
the fetch/decode and decode/execute registers, where the two guessed
instructions would have gone (the red dashed "作废" cells of the grid), and
the PC loads the address of the right path.

Two series, one per page:
branch-ok-k    the guess is right: frames show cycles 1-2, cycle 3, the
               decision with the frame, then cycles 4-7 where the two guessed
               instructions run on to write-back; snapshots of cycles 2, 3,
               3 and 4;
branch-miss-k  the guess is wrong: the decision (cycles 1-3, as the third
               frame of branch-ok), cycle 4 where the two guessed instructions
               are squashed and the first instruction of the right path is
               fetched, then cycles 5-8; snapshots of cycles 3, 4 and 5.
No fixed number of squashed instructions is written: the figure is the
five-stage model, where two are in when jl decides. The decision is written
beside the jl row, so nothing is drawn above the grid and it sits UP px
higher, as in branch_wait.py. The grid has the same place in both series; the
snapshot sits under the last row, one row lower in branch-miss.
Run it to refresh ../assets/branch-ok-1.svg .. branch-miss-3.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, MUTED, ORANGE, RED, line, path, save, text
from stall_bubble import ADDL, BH, BW, NEXT, box_x, cycle_snapshot, group
from timegrid import CH, CW, LX, NUM_Y, TOP, VIEW_LEFT, cycle_numbers, row

W = 1120
UP = 30                             # no caption above the grid
T0 = TOP - UP                       # top of the jl row (34)
GAP = 12                            # looser than pipeline: 7 px each side of the frame
R2, R3, R4 = (T0 + k * (CH + GAP) for k in (1, 2, 3))
XD = LX + 3 * CW                    # end of cycle 3: jl has just executed
STAGES = ["取指", "译码", "执行", "访存", "写回"]
JL, CMPL = "jl .L3", "cmpl %edx,%eax"
G1, G2, G3 = (f"猜来的第 {k} 条" for k in (1, 2, 3))
C1, C2 = (f"正确路径第 {k} 条" for k in (1, 2))
DECIDE = {2: ("周期末才知道跳不跳", ORANGE)}
SNAP = {"branch-ok": R3 + CH + 32, "branch-miss": R4 + CH + 28}
TAIL = {"branch-ok": 27, "branch-miss": 22}   # canvas below the boxes: notes or only "PC"
SERIES = {                          # name -> (cycles on the grid, frames)
    # a frame: (cycles shown, decided, snapshot (cycle, stages, bar marks, notes))
    "branch-ok": (7, [
        (2, False, (2, [G1, JL, CMPL, NEXT, ADDL], {}, {0: ("按猜测取指", INK)})),
        (3, False, (3, [G2, G1, JL, CMPL, NEXT], {}, DECIDE)),
        (3, True, (3, [G2, G1, JL, CMPL, NEXT], {}, DECIDE)),
        (7, True, (4, [G3, G2, G1, JL, CMPL],
                   {1: ("照常装入", INK), 2: ("照常装入", INK)},
                   {2: ("猜对：照常执行", INK)})),
    ]),
    "branch-miss": (8, [
        (3, True, (3, [G2, G1, JL, CMPL, NEXT], {}, {})),
        (4, True, (4, [C1, None, None, JL, CMPL],
                   {0: ("装入正确地址", INK), 1: ("bubble", INK), 2: ("bubble", INK)}, {})),
        (8, True, (5, [C2, C1, None, None, JL], {}, {})),
    ]),
}


def guessed(name, upto, decided):
    """jl, the two guessed rows, the decision line and the grey frame."""
    miss = name == "branch-miss"
    out = row("jl .L3", T0, 0, STAGES, upto)
    out += row("猜来的第 1 条", R2, 1, ["取指", "译码", "作废"] if miss else STAGES, upto)
    out += row("猜来的第 2 条", R3, 2, ["取指", "作废"] if miss else STAGES, upto)
    if miss:
        out += row("正确路径第 1 条", R4, 3, STAGES, upto)
        if upto >= 4:
            out.append(text(LX + 4 * CW + 12, (R2 + R3 + CH) / 2 + 6,
                            "作废：级间寄存器装入 bubble", 16, RED, "bold", anchor="start"))
    if not decided:
        return out
    out.append(text(LX + 5 * CW + 12, T0 + CH / 2 + 6, "周期 3 结束：jl 执行完，才知道跳不跳",
                    16, ORANGE, "bold", anchor="start"))
    # what is already in the pipeline when jl decides: rows 2-3, cycles 2-3.
    # Top, left and bottom sides only: the orange line, drawn after it,
    # closes the frame on the right.
    x0, y0, y1, r = LX + CW + 1, R2 - 5, R3 + CH + 5, 6
    out.append(path(f"M {XD - 1} {y0} L {x0 + r} {y0} Q {x0} {y0} {x0} {y0 + r} "
                    f"L {x0} {y1 - r} Q {x0} {y1} {x0 + r} {y1} L {XD - 1} {y1}",
                    MUTED, 2, dash="6 4", head=False))
    bottom = (R4 if miss else R3) + CH
    out.append(line(XD, T0 - 8, XD, bottom + 8, ORANGE, 2, dash="6 4"))
    return out


def hardware(name, upto, decided, snap):
    top = SNAP[name]
    out = cycle_snapshot(top, *snap)
    under = top + BH + 21
    if decided and upto == 3:
        out.append(group(top, 0, 1))
        if name == "branch-ok":     # branch-miss repeats this frame without notes
            out.append(text((box_x(0) + box_x(1) + BW) / 2, under, "判定前按猜测取进来的两条", 15,
                            MUTED, "bold"))
    return out


def build(name, cycles, upto, decided, snap):
    cycle = snap[0]
    out = cycle_numbers(cycles, emph=cycle, color=INK, y=NUM_Y - UP)
    out += guessed(name, upto, decided)
    return out + hardware(name, upto, decided, snap)


if __name__ == "__main__":
    for name, (cycles, frames) in SERIES.items():
        h = SNAP[name] + BH + TAIL[name]
        for k, (upto, decided, snap) in enumerate(frames, 1):
            save(f"{name}-{k}", W, h, build(name, cycles, upto, decided, snap), left=VIEW_LEFT)
