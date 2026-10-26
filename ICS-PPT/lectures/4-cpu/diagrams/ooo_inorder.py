#!/usr/bin/env python3
"""The five instructions of ooo.py executed in program order.

Same rows, columns and canvas as ooo.py (cycles 3-12, the read of w[i] three
cycles wide), and the same processor: decoded instructions wait inside it,
so fetch and decode go on while an older instruction waits. The one
difference is the order of execution: here an instruction executes only
after every instruction before it has. imull waits for w[i] in cycles 4-6
and executes in cycle 7; addl %ecx,%r9d, decoded in cycle 4, executes in
cycle 8 with the forwarded product; addl $1,%eax, decoded in cycle 5 with its
operand ready, still waits until cycle 9; cmpl follows in cycle 10 and
writes back in cycle 12. The green elbow is w[i] forwarded from the end of
the read into imull's execute, the orange ones the product forwarded into
addl's execute and the new %eax forwarded from addl $1,%eax's execute (cycle
9) into cmpl's (cycle 10), as in ooo.py. The red "停" cells are decoded
instructions waiting to execute.

An animation, four frames: cycle 3, cycles 4-6 (the wait), cycle 7 (w[i]
arrives), cycles 8-12. The caption over the grid explains each frame; on the
last frame it is the page's question, set larger, because the code block
above the grid leaves no room for a highlight under it.
grid() is also the grid of ooo_idle.py.
Run it to refresh ../assets/ooo-inorder-1.svg .. ooo-inorder-4.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from ooo import COLS, H, LEFT, R, ROWS, W, handoff, numbers, stages, wide
from svgkit import GREEN, INK, ORANGE, RED, save, text
from timegrid import CW, LX, TONE

FRAMES = [4, 7, 8, 13]              # cycles shown in each frame: up to (exclusive)
CAPTION_X = LX + COLS * CW / 2      # the middle of the ten columns
CAPTIONS = {                        # frame -> (caption, colour, emphasised cycle)
    4: ("周期 3：movl 算出 w[i] 的地址，imull 译码", INK, 3),
    7: ("周期 4 至 6：w[i] 还在读，imull 等它，后面的指令按程序顺序排在它后面", RED, None),
    8: ("周期 7：w[i] 读出，转发进 imull 的执行阶段", GREEN, 7),
    13: ("周期 4 至 6，流水线的五级在做什么？", INK, None),
}
QUESTION = 13                       # the last frame's caption is the page's question


def grid(upto):
    """The five rows and the forwarding elbows, cycles before `upto`."""
    out = []
    # movl: execute in cycle 3, a three-cycle read, write-back in cycle 7
    out += stages(ROWS[0], R[0], 3, ["执行"], upto)
    out += wide(4, 3, R[0], "访存", upto, *TONE["访存"])
    out += stages(ROWS[0], R[0], 7, ["写回"], upto)[1:]
    # imull: waits for w[i]
    out += stages(ROWS[1], R[1], 3, ["译码", "停", "停", "停", "执行", "访存", "写回"], upto)
    # addl %ecx,%r9d: waits for the product
    out += stages(ROWS[2], R[2], 3, ["取指", "译码", "停", "停", "停", "执行", "访存", "写回"],
                  upto)
    # addl $1,%eax: operand ready, waits for its turn
    out += stages(ROWS[3], R[3], 4, ["取指", "译码", "停", "停", "停", "执行", "访存", "写回"],
                  upto)
    # cmpl: after addl $1,%eax
    out += stages(ROWS[4], R[4], 5, ["取指", "译码", "停", "停", "停", "执行", "访存", "写回"],
                  upto)
    if upto > 7:                     # w[i] read: into imull's execute
        out.append(handoff(0, 6, GREEN))
    if upto > 8:                     # the product: into addl's execute
        out.append(handoff(1, 7, ORANGE))
    if upto > 10:                    # the new %eax: into cmpl's execute
        out.append(handoff(3, 9, ORANGE))
    return out


def build(upto):
    caption, color, emph = CAPTIONS[upto]
    size = 20 if upto == QUESTION else 16
    out = [text(CAPTION_X, 22, caption, size, color, "bold")]
    out += numbers(emph=emph, color=color)
    return out + grid(upto)


if __name__ == "__main__":
    for k, upto in enumerate(FRAMES, 1):
        save(f"ooo-inorder-{k}", W, H, build(upto), left=LEFT)
