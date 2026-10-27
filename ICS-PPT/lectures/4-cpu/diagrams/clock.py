#!/usr/bin/env python3
"""What a clock cycle is, on the hardware of stage_units.py.

Two frames. 1: inside cycle 1, all of movslq's path is lit, PC still holds
0x20; 0x23 and the new %r8 wait at the inputs of PC and the write port.
2: the clock edge that ends cycle 1 writes both, PC shows 0x23 and fetch is
lit again for movl, the next instruction. On the right a clock waveform with
cycles 1 and 2 and what happens in this frame; the definition is the slide
text above the figure. The edge is drawn in ink, the stage colours stay
reserved for the stages.
Run it to refresh ../assets/clock-1.svg, clock-2.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from stage_units import H, ORDER, PX, W, hardware, rich
from svgkit import FILL_GREY, INK, LINE, MUTED, line, path, rect, save, text

EDGES = [830, 960, 1090]      # rising clock edges; cycle 1 is [830, 960)
HI, LO = 40, 70               # waveform levels
FRAMES = [                    # (lit stages, PC, heading, lines)
    (set(ORDER), "0x20", "周期 1 之内", [
        "PC 保持 `0x20`，`movslq` 依次经过",
        "五个阶段；`0x23` 到达 PC 的输入，",
        "`0x0000000000000005` 到达写端口，",
        "PC 与 `%r8` 仍是原来的值。"]),
    ({"取指"}, "0x23", "周期 1 结束的时钟沿", [
        "同时写入：PC ← `0x23`，",
        "`%r8` ← `0x0000000000000005`；",
        "周期 2 开始，指令内存按 PC = `0x23`",
        "读出下一条指令 `movl`。"]),
]


def waveform(frame):
    e0, e1, e2 = EDGES
    out = []
    if frame == 0:
        out.append(rect(e0, HI - 8, e1 - e0, LO - HI + 16, FILL_GREY, "none", rx=3, width=0))
    for e in EDGES:
        out.append(line(e, HI - 12, e, LO + 10, LINE, 1.2, "4 3"))
    d = (f"M {PX + 10} {LO} L {e0} {LO} L {e0} {HI} L {(e0 + e1) / 2} {HI} "
         f"L {(e0 + e1) / 2} {LO} L {e1} {LO} L {e1} {HI} L {(e1 + e2) / 2} {HI} "
         f"L {(e1 + e2) / 2} {LO} L {e2} {LO} L {e2} {HI} L {W - 14} {HI}")
    out.append(path(d, INK, 2, head=False))
    if frame == 1:
        out.append(line(e1, HI - 14, e1, LO + 12, INK, 3))
        out.append(text(e1, HI - 20, "时钟沿", 15, INK, "bold"))
    for k, (a, b) in enumerate([(e0, e1), (e1, e2)]):     # the current cycle in bold
        on = k == frame
        out.append(text((a + b) / 2, LO + 26, f"周期 {k + 1}", 15, INK if on else MUTED,
                        "bold" if on else "normal"))
    return out


def build(frame):
    lit, pc, heading, lines = FRAMES[frame]
    out = hardware(lit, pc) + waveform(frame)
    out.append(text(PX, 140, heading, 19, INK, "bold", anchor="start"))
    for k, s in enumerate(lines):
        out += rich(PX, 166 + 24 * k, s)
    return out


if __name__ == "__main__":
    for k in range(len(FRAMES)):
        save(f"clock-{k + 1}", W, H, build(k))
