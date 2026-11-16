#!/usr/bin/env python3
"""The two kernels of diverge.cu on two warps: the same branches, divided differently.

Left, branch_by_lane (threadIdx.x % 2): the lanes of every warp alternate between
the two branches, so each warp runs the first path with the odd lanes idle and then
the second path with the even lanes idle, and the two paths add up. Right,
branch_by_warp (threadIdx.x / 32 % 2): a warp is all on one branch and runs that
path only, so the paths of different warps overlap in time. Blue is the first
branch, orange the second. Under each pair of warps the bars are drawn to one time
scale: two paths against one, the measured 0.903 ms and 0.459 ms.
Run it to refresh ../assets/diverge-compare.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, MUTED, ORANGE, brace, mono, rect, save,
                    text)

W, H = 1120, 262
PW, PGAP = 520, 80
LABEL = 82                        # the column of the warp names
CW, CH = 13, 22                   # one lane
UNIT = 16 * CW                    # the time one path takes

ONE = (FILL_BLUE, BLUE)
TWO = (FILL_ORANGE, ORANGE)

PANELS = [
    ("按线程奇偶划分", "threadIdx.x % 2", "0.903 ms",
     [[ONE if lane % 2 == 0 else TWO for lane in range(32)]] * 2,
     [[(0, ONE, "第一条路径"), (1, TWO, "第二条路径")]] * 2, 2),
    ("按 Warp 奇偶划分", "threadIdx.x / 32 % 2", "0.459 ms",
     [[ONE] * 32, [TWO] * 32],
     [[(0, ONE, "第一条路径")], [(0, TWO, "第二条路径")]], 1),
]


def panel(x, title, test, ms, lanes, paths, units):
    out = [text(x + PW / 2, 24, title, 18, INK, "bold"),
           mono(x + PW / 2, 46, test, 14, MUTED, anchor="middle")]
    for warp in range(2):
        top = 66 + warp * 80
        out.append(text(x, top + 16, f"Warp {warp}", 15, INK, "bold", "start"))
        out.append(text(x, top + 36, f"线程 {32 * warp}–{32 * warp + 31}", 12, MUTED,
                        anchor="start"))
        for lane, (fill, stroke) in enumerate(lanes[warp]):
            out.append(rect(x + LABEL + lane * CW, top, CW, CH, fill, stroke, rx=2, width=1.2))
        for start, (fill, stroke), name in paths[warp]:
            bx = x + LABEL + start * UNIT
            out.append(rect(bx + 1, top + 32, UNIT - 2, 26, fill, stroke, rx=3, width=1.4))
            out.append(text(bx + UNIT / 2, top + 50, name, 14, INK))
    x0 = x + LABEL
    out += brace(x0, x0 + units * UNIT, 66 + 80 + 62, INK, f"耗时 {ms}", size=16)
    return out


def build():
    out = []
    for k, args in enumerate(PANELS):
        out += panel(k * (PW + PGAP), *args)
    return out


if __name__ == "__main__":
    save("diverge-compare", W, H, build(), left=-10)
