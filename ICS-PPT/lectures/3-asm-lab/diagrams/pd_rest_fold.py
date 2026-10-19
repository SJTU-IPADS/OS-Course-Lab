#!/usr/bin/env python3
"""The fold inside one block: 256 partial sums in shared memory become one
in 8 steps, halving at every step.

Each row is the shared array part[] after a step, one cell per element
still in play, 3 px each so that the halving is a proportion. At a step,
thread t < k adds part[t + k] to part[t]; the right half of a row lands
on the left half of the next. __syncthreads() separates the steps. Thread
0 writes part[0] into y[row].
Run it to refresh ../assets/pd-rest-fold.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_GREY, GREEN, INK, LINE, MONO, MUTED, WHITE,
                    arrow, box, brace, elbow, line, mono, rect, save, text)

W, H = 1120, 410
X0, PX, RH = 230, 2.5, 22              # the array starts at X0, 2.5 px per element
STEPS = [(256, 40), (128, 108), (64, 176), (32, 244)]


def row(y, n, label, shade_right):
    out = [text(X0 - 10, y + 15, label, 12.5, INK, "bold", anchor="end")]
    w = n * PX
    out.append(rect(X0, y, w, RH, FILL_GREEN, GREEN, rx=2, width=1.2))
    if shade_right:
        out.append(rect(X0 + w / 2, y, w / 2, RH, WHITE, GREEN, rx=2, width=1.2))
    for i in range(1, n):
        if i % 32 == 0 or n <= 32 and i % 4 == 0:
            out.append(line(X0 + i * PX, y, X0 + i * PX, y + RH, GREEN, 0.6))
    return out


def build():
    out = []
    out.append(text(X0, 22, "part[256]：共享内存中的数组，线程 t 先写入 part[t] = 自己的部分和；每一步之后 __syncthreads()", 14,
                    INK, "bold", anchor="start"))
    for i, (n, y) in enumerate(STEPS):
        out += row(y, n, f"{n} 个", True)
        half = n // 2
        if half * PX >= 120:
            out.append(text(X0 + half * PX / 2, y + 15, f"part[0 .. {half - 1}]", 11, INK))
            out.append(text(X0 + half * PX * 1.5, y + 15, f"part[{half} .. {n - 1}]", 11, MUTED))
        ny = y + 68
        # the right half lands on the left half of the next row
        out.append(arrow(X0 + half * PX * 1.5, y + RH + 2, X0 + half * PX / 2 + 10, ny - 4, GREEN, 1.6))
        out.append(line(X0 + half * PX / 2 - 10, y + RH + 2, X0 + half * PX / 2 - 10, ny - 4, GREEN, 1, "3 3"))
        out.append(text(X0 + n * PX + 14, y + 10, f"第 {i + 1} 步：线程 t < {half}", 12, INK, anchor="start"))
        out.append(text(X0 + n * PX + 14, y + 26, f"part[t] += part[t+{half}]", 11.5, MUTED, anchor="start"))
    y = STEPS[-1][1] + 68
    out.append(text(X0 + 48, y + 6, "…", 18, GREEN, "bold"))
    out.append(text(X0 + 120, y + 6, "第 5 到第 8 步：16、8、4、2 个，每步减半", 12, MUTED, anchor="start"))
    y += 30
    out += row(y, 1, "1 个", False)
    out.append(text(X0 + 10, y + 15, "part[0]：256 个部分和之和", 11.5, INK, anchor="start"))
    out.append(arrow(X0 + 200, y + RH / 2, X0 + 250, y + RH / 2, GREEN, 1.6))
    out += box(X0 + 256, y - 4, 110, 30, "y[row]", FILL_GREEN, GREEN, 14, font=MONO)
    out.append(text(X0 + 380, y + 15, "线程 0 写入", 12, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("pd-rest-fold", W, H, build(), left=29)
