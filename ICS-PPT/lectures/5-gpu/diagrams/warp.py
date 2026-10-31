#!/usr/bin/env python3
"""The halving sum of 8 threads when the whole block is one warp.

Drawn to sit under the text of its page: the one copy of the loop on the left,
because the 8 threads of the warp share one instruction stream and one program
counter, and the rows of cache[0..7] on the right, one per step. Every step is
one instruction for all 8 threads, and the next step starts after it has been
written, so the sum is right without any barrier: cache[0] = 31. Frame k shows
the first k rows and the line the warp has reached:
../assets/warp-1.svg .. warp-4.svg. Run it to refresh them.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE, listing, mono, rect, save,
                    text)
from syncfig import VALUES, Geo, table

W, H = 1120, 292
GEO = Geo(x0=500, cw=56, ch=36, top=34, pitch=60, right=0)
# (step, thread note, values, written cells, stale cells, dead cells)
ROWS = [
    (None, "每个线程存入一个值", VALUES, [], [], []),
    (4, "线程 0–3 写入", [8, 10, 6, 7, 5, 9, 2, 6], [0, 1, 2, 3], [], [4, 5, 6, 7]),
    (2, "线程 0、1 写入", [14, 17, 6, 7, 5, 9, 2, 6], [0, 1], [], [2, 3, 4, 5, 6, 7]),
    (1, "线程 0 写入", [31, 17, 6, 7, 5, 9, 2, 6], [0], [], [1, 2, 3, 4, 5, 6, 7]),
]
RIGHT_NOTES = [
    None,
    ["8 个线程执行同一条指令，", "其中线程 0–3 写入"],
    ["读到的 cache[2]、cache[3] ", "已是 6 和 7"],
    ["cache[0] = 31，", "8 个值的和"],
]
CODE = [
    "for (s = 4; s > 0; s >>= 1) {",
    "  if (tid < s)",
    "    cache[tid] += cache[tid + s];",
    "}",
]
CODE_X, CODE_W, CODE_SIZE, LH = 12, 304, 14, 28
# (line of CODE, s): the line the warp has reached
STANDING = [(0, 4), (2, 4), (2, 2), (2, 1)]


# the ink is the code box and the table, so it does not change from frame to frame;
# the window is moved, not the drawing, to put it in the middle of the canvas
INK_LEFT, INK_RIGHT = CODE_X, GEO.x0 + 8 * GEO.cw
LEFT = INK_LEFT - (W - (INK_RIGHT - INK_LEFT)) / 2


def frame(k):
    at, s_now = STANDING[k - 1]
    out = [mono(GEO.x0 - 20, GEO.top - 12, "cache[i]", 13, MUTED, anchor="end")]
    for t in range(8):
        out.append(mono(GEO.x0 + t * GEO.cw + GEO.cw / 2, GEO.top - 12, str(t), 13, MUTED,
                        anchor="middle"))
    out += table(ROWS, k, None, None, GEO)
    note = RIGHT_NOTES[k - 1]
    if note:   # under the table, where the next row is not yet
        last = k == len(ROWS)
        out.append(text(GEO.x0, GEO.top + 3 * GEO.pitch + GEO.ch + 26, "".join(note), 17,
                        INK, "bold" if last else "normal", "start"))
    top = GEO.top + GEO.pitch - 8   # beside rows 1 to 3, clear of the label of row 0
    out.append(rect(CODE_X, top, CODE_W, 36 + len(CODE) * LH + 10, WHITE, LINE, rx=6))
    out.append(text(CODE_X + 12, top + 24, "线程 0–7 · Warp 0", 14, INK, "bold", "start"))
    out.append(mono(CODE_X + CODE_W - 12, top + 24, f"s = {s_now}", 14, ORANGE, "bold", "end"))
    out += listing(CODE_X + 12, top + 34, CODE, size=CODE_SIZE, lh=LH, marks={at: FILL_ORANGE},
                   width=CODE_W - 14)
    return out


if __name__ == "__main__":
    for k in range(1, len(ROWS) + 1):
        save(f"warp-{k}", W, H, frame(k), left=LEFT)
