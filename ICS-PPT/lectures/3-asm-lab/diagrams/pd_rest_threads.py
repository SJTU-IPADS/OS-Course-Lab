#!/usr/bin/env python3
"""Inside one thread block of matvec_q4_0_kernel: 256 threads stride over the row.

Top: the row's 2048 columns as 64 Q4_0 blocks, with x above it, cut into 8
strides of 256 columns. Middle: the first stride opened, 8 Q4_0 blocks of
32 columns each, and the 256 threads under it, thread t under column t:
the 32 threads of a warp stand under one Q4_0 block. Bottom: each thread's
partial sum in its own register. The same 256 threads come back for the
next stride, 8 strides in all. A block of weights is always written
Q4_0 块 and a block of threads 线程块, so that the two are not confused.
Run it to refresh ../assets/pd-rest-threads.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, arrow, brace, elbow, line,
                    mono, rect, save, text)

W, H = 1120, 400
RX, RC = 160, 13                       # the row: 64 cells of 13 px
XY, RY, RH = 30, 58, 22                # x strip, row strip
ZX, ZW = 200, 640                      # the stride opened: 256 columns at 2.5 px
ZXY, ZRY, ZTY, ZSY = 160, 190, 260, 330


def build():
    out = []
    # x and the row, 64 blocks, 8 strides
    out.append(text(RX - 8, XY + 16, "x：2048 个 float", 11.5, INK, "bold", anchor="end"))
    out.append(rect(RX, XY, 64 * RC, RH, FILL_GREY, MUTED, rx=2, width=1.2))
    out.append(text(RX - 8, RY + 16, "第 r 行：64 个 Q4_0 块", 11.5, INK, "bold", anchor="end"))
    for b in range(64):
        out.append(rect(RX + b * RC, RY, RC, RH, FILL_BLUE, BLUE, rx=1, width=0.8))
    for s in range(8):
        x0 = RX + s * 8 * RC
        out.append(line(x0, XY - 4, x0, RY + RH + 4, INK, 1.2, dash="3 3"))
        out.append(text(x0 + 4 * RC, RY + RH + 16, f"第 {s + 1} 轮", 11.5, INK if s == 0 else MUTED,
                        "bold" if s == 0 else "normal"))
    out.append(line(RX + 64 * RC, XY - 4, RX + 64 * RC, RY + RH + 4, INK, 1.2, dash="3 3"))
    out.append(text(RX + 60 * RC, RY + RH + 34, "8 轮 × 256 列 = 2048 列", 12, MUTED))

    # the first stride opened
    for a, b in ((RX, ZX), (RX + 8 * RC, ZX + ZW)):
        out.append(line(a, RY + RH + 22, b, ZXY, LINE, 1.1, dash="4 4"))
    out.append(text(ZX - 8, ZXY + 16, "x[0 .. 255]", 12.5, INK, "bold", anchor="end"))
    out.append(rect(ZX, ZXY, ZW, RH, FILL_GREY, MUTED, rx=2, width=1.2))
    out.append(text(ZX - 8, ZRY + 16, "第 1 轮：8 个 Q4_0 块", 12.5, INK, "bold", anchor="end"))
    for k in range(8):
        x = ZX + k * 80
        out.append(rect(x, ZRY, 80, RH, FILL_BLUE, BLUE, rx=2, width=1.2))
        out.append(text(x + 40, ZRY + 15, f"列 {32 * k}..{32 * k + 31}", 10.5, INK))
    # the 256 threads, thread t under column t
    out.append(text(ZX - 8, ZTY + 10, "线程块的 256 个线程", 12.5, INK, "bold", anchor="end"))
    out.append(text(ZX - 8, ZTY + 26, "线程 t 读第 t 列", 11.5, MUTED, anchor="end"))
    for t in range(256):
        out.append(rect(ZX + t * 2.5, ZTY, 2, 14, FILL_ORANGE, ORANGE, rx=0, width=0.4))
    for k in range(8):
        x = ZX + k * 80
        out.append(line(x, ZRY + RH, x, ZTY + 14, ORANGE, 1, dash="2 2"))
        out.append(text(x + 40, ZTY + 30, f"Warp {k}", 11.5, ORANGE, "bold"))
    out.append(line(ZX + 8 * 80, ZRY + RH, ZX + 8 * 80, ZTY + 14, ORANGE, 1, dash="2 2"))
    out.append(text(ZX + ZW + 10, ZTY + 10, "同一个 Warp 的 32 个线程", 12, INK, "bold", anchor="start"))
    out.append(text(ZX + ZW + 10, ZTY + 26, "读同一个 Q4_0 块：相邻的字节", 11.5, MUTED, anchor="start"))
    # the partial sums, one register per thread
    out.append(text(ZX - 8, ZSY + 16, "部分和：各自的寄存器", 12.5, INK, "bold", anchor="end"))
    out.append(rect(ZX, ZSY, ZW, RH, FILL_GREEN, GREEN, rx=2, width=1.2))
    for t in range(1, 256):
        if t % 32 == 0:
            out.append(line(ZX + t * 2.5, ZSY, ZX + t * 2.5, ZSY + RH, GREEN, 0.6))
    for t in (0, 128, 255):
        out.append(arrow(ZX + t * 2.5 + 1, ZTY + 16, ZX + t * 2.5 + 1, ZSY - 4, GREEN, 1.2))
    out.append(text(ZX + ZW + 10, ZSY + 10, "线程 t 累加 W[r][t+256s] · x[t+256s]", 12, INK, "bold",
                    anchor="start"))
    out.append(text(ZX + ZW + 10, ZSY + 26, "s = 0 .. 7：8 轮，8 个乘积", 11.5, MUTED, anchor="start"))
    # the next stride: the same threads, 256 columns further
    out.append(elbow([(ZX + ZW + 4, ZRY + RH / 2), (1040, ZRY + RH / 2), (1040, XY - 14),
                      (RX + 12 * RC, XY - 14), (RX + 12 * RC, XY - 3)], ORANGE, 1.6, dash="6 4"))
    out.append(text(ZX + ZW + 10, ZRY + RH / 2 + 22, "第 2 轮：同样的线程，列 256..511", 12, ORANGE, "bold",
                    anchor="start"))
    out.append(text(560, H - 14, "每个线程的部分和累加完 8 轮之后，线程块内 256 个部分和再相加为 y[r]", 14, INK, "bold"))
    return out


if __name__ == "__main__":
    save("pd-rest-threads", W, H, build(), left=15)
