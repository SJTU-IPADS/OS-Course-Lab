#!/usr/bin/env python3
"""The storage levels of the hardware as a stack, from registers to remote storage.

Each row gives the access time and the capacity of the level, with the numbers of
the storage technology table where that table has the level. The fills are
registers and SRAM blue, DRAM green, SSD grey. Remote storage is on
another machine, so its outline is dashed. The brackets say where each level sits.
Run it to refresh ../assets/storage-levels.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import FILL_BLUE, FILL_GREEN, FILL_GREY, GREEN, INK, MUTED, rect, save, text, vbrace

W, H = 980, 384
CX = 380                                  # centre line of the stack
ROW_H, GAP, TOP = 60, 10, 36
X_BRACKET, X_TIME, X_SIZE = 130, 660, 840
LEVELS = [
    ("寄存器", FILL_BLUE, INK, None, ("约 0.2 ns",), "数百字节"),
    ("SRAM：L1 至 L3 缓存", FILL_BLUE, INK, None, ("1 ns",), "KB 至 MB"),
    ("DRAM：内存", FILL_GREEN, GREEN, None, ("10–100 ns",), "GB"),
    ("SSD", FILL_GREY, MUTED, None, ("10–100 µs",), "256 GB 至 2 TB"),
    ("远端存储：RDMA、互联网等", FILL_GREY, MUTED, "7 5", ("RDMA：数 µs", "互联网：数十 ms"), "PB"),
]
GROUPS = [(0, 1, "处理器芯片内"), (2, 3, "本机"), (4, 4, "其他机器")]


def row_top(k):
    return TOP + k * (ROW_H + GAP)


def build():
    out = [text(X_TIME, 24, "访问时间", 16, MUTED, "bold", "start"),
           text(X_SIZE, 24, "容量", 16, MUTED, "bold", "start")]
    for k, (label, fill, stroke, dash, times, size) in enumerate(LEVELS):
        width = 190 + 72 * k
        y = row_top(k)
        out.append(rect(CX - width / 2, y, width, ROW_H, fill, stroke, rx=6, width=1.8, dash=dash))
        out.append(text(CX, y + ROW_H / 2 + 6, label, 18, INK, "bold"))
        middle = y + ROW_H / 2 + 6
        for i, line in enumerate(times):
            out.append(text(X_TIME, middle + (i - (len(times) - 1) / 2) * 22, line, 17, INK,
                            anchor="start"))
        out.append(text(X_SIZE, middle, size, 17, INK, anchor="start"))
    for first, last, label in GROUPS:
        out += vbrace(X_BRACKET, row_top(first) + 2, row_top(last) + ROW_H - 2, MUTED, label, 16,
                      right=False)
    return out


if __name__ == "__main__":
    save("storage-levels", W, H, build())
