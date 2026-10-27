#!/usr/bin/env python3
"""What the stages do while imull waits for w[i], instructions executed in order.

The whole grid of ooo_inorder.py, a red dashed frame around cycles 4-6, and
under it the five stages in cycle 5, drawn with stall_bubble.py's snapshot as
on the stall-wait and load-use pages: fetch has cmpl, decode has addl
$1,%eax, execute holds a bubble, memory reads w[i] for movl, and write-back
holds a bubble because nothing left memory at the end of cycle 4. No pipeline
register stalls: imull and addl %ecx,%r9d wait inside the processor after
decode, named on the line under the stages. The execute stage has no
instruction in all three cycles.
Run it to refresh ../assets/ooo-idle.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from ooo import LEFT, R, W, col, numbers
from ooo_inorder import CAPTION_X, grid
from stall_bubble import BH, RW, bar_x, cycle_snapshot
from svgkit import GREEN, MUTED, RED, rect, save, text
from timegrid import CH, CW, LX, TOP

SNAP = R[4] + CH + 24               # box top of the snapshot under the grid
H = SNAP + 110
STATE = (["cmpl %edx,%eax", "addl $1,%eax", None, "movl", None], {},
         {2: ("没有指令可执行", MUTED), 3: ("读 w[i]", GREEN), 4: ("没有指令可写回", MUTED)})


def build():
    out = [text(CAPTION_X, 22, "周期 4 至 6：执行级没有指令可执行", 16, RED, "bold")]
    out += numbers(emph=5, color=RED)
    out += grid(13)
    out.append(rect(LX + col(4) * CW + 1, TOP - 5, 3 * CW - 2, R[4] + CH + 10 - TOP, "none",
                    RED, rx=6, width=2, dash="6 4"))
    out += cycle_snapshot(SNAP, 5, *STATE)
    out.append(text(bar_x(2) + RW / 2, SNAP + BH + 46,
                    "译码后在等执行：imull 等 w[i]，addl %ecx,%r9d 等乘积", 15, RED, "bold"))
    return out


if __name__ == "__main__":
    save("ooo-idle", W, H, build(), left=LEFT)
