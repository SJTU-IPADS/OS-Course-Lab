#!/usr/bin/env python3
"""A DRAM read brings out a whole row into the row buffer, and later reads of the same row are
served from the buffer, in five frames.

Main memory is the memory module of dram_module.py inside a dashed box: a row is four 8-byte
words, each word two 4-byte ints (w[0..7] in row 0, w[8..15] in row 1), and the row buffers of
the eight chips together hold one row. A read
that opens a row takes 100 ns, a read of a cell already in the row buffer takes 10 ns. The
timeline under the figure is drawn at 2.5 px per ns.
Frame 1: rows and the row buffer. Frame 2: w[0] is read, row 0 goes into the buffer, 100 ns.
Frame 3: w[1] comes from the buffer, 10 ns. Frame 4: w[2]..w[7] come from the buffer, 170 ns in
all for eight reads. Frame 5: w[8] lies in row 1, which replaces row 0 in the buffer, 270 ns.
Drawn at the width it is shown on the slide (1100 px).
Run it to refresh ../assets/memory-row-1.svg .. memory-row-5.svg .
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_RED, GREEN, INK, MUTED, ORANGE,
                    RED, WHITE, elbow, line, rect, save, text)

W, H = 1100, 420
X0, CW, CH = 216, 100, 52
ROW_Y = (46, 106)                         # tops of main-memory row 0 and row 1
BUF_Y = 184                               # top of the row buffer
BAR_X, BAR_Y, BAR_H, PX = 216, 330, 28, 2.5
OPEN_NS, HIT_NS = 100, 10
CAPTIONS = ["① 内存的一行是 4 个 8 字节的字，8 个芯片的行缓冲合起来放得下一整行",
            "② 读 w[0]：8 个芯片各读出自己的整行，放入行缓冲，100 ns",
            "③ 读 w[1]：数据已在行缓冲中，10 ns",
            "④ w[2] 至 w[7] 也在行缓冲中，各 10 ns，8 次读取共 170 ns，逐次读取需要 800 ns",
            "⑤ 读 w[8]：它在下一行，再读出一整行，100 ns"]


def frame(k):
    out = []
    out += [rect(4, BUF_Y, 100, CH, FILL_GREY, MUTED, rx=6, width=1.6),
            text(54, BUF_Y + CH / 2 + 7, "处理器", 21, INK, "bold")]
    right = X0 + 8 * CW
    out.append(rect(X0 - 100, 2, right - X0 + 100 + 82, BUF_Y + CH + 24 - 2, "none", MUTED, rx=8,
                    width=1.6, dash="7 5"))
    out.append(text(X0, 30, "内存：8 个 DRAM 芯片并联，每行 4 个超单元，每个字 8 字节，存放 2 个 int", 18, MUTED,
                    "bold", "start"))
    open_row = {1: None, 2: 0, 3: 0, 4: 0, 5: 1}[k]     # the row held in the row buffer
    hot = {2: 0, 3: 1, 5: 0}.get(k)                      # index in the buffer read this frame

    for r in range(2):
        lit = r == open_row
        out.append(text(X0 - 12, ROW_Y[r] + CH / 2 + 7, f"行 {r}", 19,
                        ORANGE if lit else INK, "bold", "end"))
        for c in range(8):
            out.append(rect(X0 + c * CW, ROW_Y[r], CW, CH, FILL_ORANGE if lit else FILL_GREEN,
                            ORANGE if lit else GREEN, rx=3, width=1.4))
            out.append(text(X0 + c * CW + CW / 2, ROW_Y[r] + CH / 2 + 7, f"w[{8 * r + c}]", 20,
                            INK, font="monospace"))

    out.append(text(X0 - 12, BUF_Y + CH / 2 + 7, "行缓冲", 19, INK, "bold", "end"))
    for c in range(8):
        if open_row is None:
            out.append(rect(X0 + c * CW, BUF_Y, CW, CH, WHITE, GREEN, rx=3, width=1.4))
            continue
        is_hot = c == hot
        out.append(rect(X0 + c * CW, BUF_Y, CW, CH, ORANGE if is_hot else FILL_ORANGE, ORANGE,
                        rx=3, width=2.4 if is_hot else 1.4))
        out.append(text(X0 + c * CW + CW / 2, BUF_Y + CH / 2 + 7, f"w[{8 * open_row + c}]", 20,
                        WHITE if is_hot else INK, font="monospace"))

    for y in (ROW_Y[0], ROW_Y[1], BUF_Y):                  # a thicker outline per 8-byte word
        for wd in range(4):
            out.append(rect(X0 + 2 * wd * CW, y, 2 * CW, CH, "none", INK, rx=3, width=4))
    if k in (2, 5):                                        # the row is copied into the buffer
        r = open_row
        y0 = ROW_Y[r] + CH / 2
        out.append(elbow([(right + 2, y0), (right + 26, y0), (right + 26, BUF_Y + CH / 2),
                          (right + 2, BUF_Y + CH / 2)], ORANGE, 2.8))
        out.append(text(right + 34, (y0 + BUF_Y + CH / 2) / 2 + 6, "整行", 17, ORANGE, "bold",
                        "start"))
    if hot is not None:                                    # the element goes to the processor
        cx = X0 + hot * CW + CW / 2
        out.append(elbow([(cx, BUF_Y + CH), (cx, BUF_Y + CH + 34), (54, BUF_Y + CH + 34),
                          (54, BUF_Y + CH + 2)], ORANGE, 2.8))
        out.append(text((cx + 54) / 2, BUF_Y + CH + 56, "4 字节", 17, ORANGE, "bold"))

    # timeline, 2.5 px per ns
    reads = [(0, OPEN_NS, "读 w[0]：100 ns")] if k >= 2 else []
    if k >= 3:
        reads.append((OPEN_NS, HIT_NS, ""))
    if k >= 4:
        reads += [(OPEN_NS + HIT_NS * i, HIT_NS, "") for i in range(1, 7)]
    if k == 5:
        reads.append((170, OPEN_NS, "读 w[8]：100 ns"))
    out.append(text(110, BAR_Y + 4, "累计时间", 17, MUTED, "bold"))
    total = {1: 0, 2: 100, 3: 110, 4: 170, 5: 270}[k]
    out.append(text(110, BAR_Y + 32, f"{total} ns", 22, INK, "bold"))
    out.append(rect(BAR_X, BAR_Y, 300 * PX, BAR_H, WHITE, MUTED, rx=2, width=1))
    for start, dur, label in reads:
        slow = dur == OPEN_NS
        out.append(rect(BAR_X + start * PX, BAR_Y, dur * PX, BAR_H, FILL_RED if slow else FILL_GREEN,
                        RED if slow else GREEN, rx=2, width=1.4))
        if label:
            out.append(text(BAR_X + (start + dur / 2) * PX, BAR_Y + 21, label, 16, INK,
                            font="monospace"))
    if k >= 3:
        out.append(text(BAR_X + OPEN_NS * PX, BAR_Y - 8,
                        "w[1]：10 ns" if k == 3 else "w[1] 至 w[7]：7 × 10 ns", 16, GREEN, "bold",
                        "start"))
    ay = BAR_Y + BAR_H + 8
    out.append(line(BAR_X, ay, BAR_X + 300 * PX, ay, MUTED, 1.2))
    for ns in range(0, 301, 100):
        out.append(line(BAR_X + ns * PX, ay, BAR_X + ns * PX, ay + 6, MUTED, 1.2))
        out.append(text(BAR_X + ns * PX, ay + 24, f"{ns} ns", 15, MUTED))

    out.append(text(W / 2, H - 6, CAPTIONS[k - 1], 20, INK, "bold"))
    return out


if __name__ == "__main__":
    for k in range(1, 6):
        save(f"memory-row-{k}", W, H, frame(k))
