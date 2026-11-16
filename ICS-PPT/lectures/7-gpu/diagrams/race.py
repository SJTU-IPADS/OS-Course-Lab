#!/usr/bin/env python3
"""The halving sum of 8 threads when a block has several warps and nothing
makes the warps wait for each other.

Eight threads, two to a warp: threads 0 and 1 are warp 0, threads 2 and 3 are
warp 1, and the warps are not in step. Warp 0 is ahead. Threads 0 and 1 finish
step s = 4 (cache[0] = 3 + 5, cache[1] = 1 + 9) and go on to s = 2. They read
cache[2] and cache[3] before threads 2 and 3 have written them, so they add the
old values 4 and 1 instead of 6 and 7. Threads 2 and 3 finish s = 4 later, too
late for the two sums already made. Thread 0 ends with cache[0] = 12 + 11 = 23
where the right sum is 31.
Above, each row is the state of cache[0..7] after one event. Below, one copy of
the loop per thread that adds (threads 0 to 3), with the line that thread has
reached washed orange and its own s. The loop has no barrier yet; fold.py adds
one. The geometry is syncfig.py's, so that the two animations have one size.
Frame k shows the first k rows and where each thread stands:
../assets/race-1.svg .. race-5.svg. Run it to refresh them.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import save
from syncfig import VALUES, H, W, copies, header, table

# (step, thread note, values, written cells, stale cells, dead cells)
ROWS = [
    (None, "每个线程存入一个值", VALUES, [], [], []),
    (4, "线程 0、1 执行加法", [8, 10, 4, 1, 5, 9, 2, 6], [0, 1], [2, 3], []),
    (2, "线程 0、1 不等待", [12, 11, 4, 1, 5, 9, 2, 6], [0, 1], [2, 3], []),
    (4, "线程 2、3 补做加法", [12, 11, 6, 7, 5, 9, 2, 6], [2, 3], [], [4, 5, 6, 7]),
    (1, "线程 0 执行加法", [23, 11, 6, 7, 5, 9, 2, 6], [0], [], [1, 2, 3, 4, 5, 6, 7]),
]
RIGHT_NOTES = [
    None,
    ["线程 2、3 还没有执行，", "它们的格子仍是旧值"],
    ["读到旧值 4、1，", "正确的值是 6、7"],
    ["线程 2、3 此时才写入，", "线程 0、1 已用了旧值"],
    ["cache[0] = 23", "正确的和是 31"],
]
CODE = [
    "for (s = 4; s > 0; s >>= 1) {",
    "  if (tid < s)",
    "    cache[tid] += cache[tid + s];",
    "}",
]
# Where each of threads 0..3 stands in each frame: (line of CODE, s, waiting).
# Line 1 is the test, line 2 the addition just done; a thread about to add
# stands on line 1.
STANDING = [
    [(0, 4, False)] * 4,
    [(2, 4, False), (2, 4, False), (1, 4, False), (1, 4, False)],
    [(2, 2, False), (2, 2, False), (1, 4, False), (1, 4, False)],
    [(2, 2, False), (2, 2, False), (2, 4, False), (2, 4, False)],
    [(2, 1, False), (1, 1, False), (2, 4, False), (2, 4, False)],
]


def frame(k):
    return (header() + table(ROWS, k, RIGHT_NOTES[k - 1], "bad" if k == len(ROWS) else None) + copies(CODE, STANDING[k - 1]))


if __name__ == "__main__":
    for k in range(1, len(ROWS) + 1):
        save(f"race-{k}", W, H, frame(k))
