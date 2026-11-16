#!/usr/bin/env python3
"""The halving sum of race.py with __syncthreads() between its steps, so the
warp that is ahead waits for the warp that is behind.

Same 8 threads, two to a warp, same table and same four copies of the loop as
race.py (the geometry is syncfig.py's, so the animations change over without a
jump). The loop now has the barrier. Warp 0 is ahead in step s = 4: it adds,
reaches the barrier and waits, red, while warp 1 adds; when warp 1 arrives all
threads go on. In step s = 2 threads 2 and 3 do not add, reach the barrier
first and wait for warp 0. In step s = 1 only thread 0 adds. Threads 4 to 7 add
nothing in these steps and are taken to be at the barrier already; they are not
drawn. Each frame shows the first rows of the table and where each thread
stands: ../assets/fold-1.svg .. fold-7.svg. Run it to refresh them.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import save
from syncfig import ASSUMPTION, VALUES, H, W, copies, header, table

# (step, thread note, values, written cells, stale cells, dead cells)
ROWS = [
    (None, "每个线程存入一个值", VALUES, [], [], []),
    (4, "线程 0、1 执行加法", [8, 10, 4, 1, 5, 9, 2, 6], [0, 1], [], [4, 5, 6, 7]),
    (4, "线程 2、3 执行加法", [8, 10, 6, 7, 5, 9, 2, 6], [2, 3], [], [4, 5, 6, 7]),
    (2, "线程 0、1 执行加法", [14, 17, 6, 7, 5, 9, 2, 6], [0, 1], [], [2, 3, 4, 5, 6, 7]),
    (1, "线程 0 执行加法", [31, 17, 6, 7, 5, 9, 2, 6], [0], [], [1, 2, 3, 4, 5, 6, 7]),
]
CODE = [
    "for (s = 4; s > 0; s >>= 1) {",
    "  if (tid < s)",
    "    cache[tid] += cache[tid + s];",
    "  __syncthreads();",
    "}",
]
# (rows shown, note at the newest row, where threads 0..3 stand: (line, s, waiting))
# Line 1 is the test, line 2 the addition just done, line 3 the barrier.
FRAMES = [
    (1, None, [(0, 4, False)] * 4),
    (2, ["线程 2、3 还没有执行，", "Warp 0 先执行"],
     [(2, 4, False), (2, 4, False), (1, 4, False), (1, 4, False)]),
    (2, ["Warp 0 到达屏障，", "等待 Warp 1"],
     [(3, 4, True), (3, 4, True), (1, 4, False), (1, 4, False)]),
    (3, ["Warp 1 执行加法，", "Warp 0 仍在等待"],
     [(3, 4, True), (3, 4, True), (2, 4, False), (2, 4, False)]),
    (3, ["全部线程到达屏障，", "继续执行下一轮"],
     [(3, 4, False)] * 4),
    (4, ["线程 2、3 不执行加法，", "先到达屏障，等待 Warp 0"],
     [(2, 2, False), (2, 2, False), (3, 2, True), (3, 2, True)]),
    (5, ["cache[0] = 31", "8 个值的和"],
     [(2, 1, False), (3, 1, True), (3, 1, True), (3, 1, True)]),
]
NOTE = "线程 4–7 不执行加法，先到达屏障，图中不画出"


def frame(k):
    shown, right, standing = FRAMES[k - 1]
    return header(ASSUMPTION + [NOTE]) + table(ROWS, shown, right, "good" if k == len(FRAMES) else None) + copies(CODE, standing)


if __name__ == "__main__":
    for k in range(1, len(FRAMES) + 1):
        save(f"fold-{k}", W, H, frame(k))
