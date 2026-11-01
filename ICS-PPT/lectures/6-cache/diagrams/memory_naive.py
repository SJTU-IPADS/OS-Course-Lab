#!/usr/bin/env python3
"""The simplest memory model: main memory is one row of cells, and a read returns exactly the
bytes asked for, in five frames.

The memory strip holds w[0..3] and x[0..3], 4 bytes per cell. Every read takes 100 ns and the
multiply-add of an iteration takes 0.2 ns. The timeline under the strip is drawn at 1 px per ns.
Frame 1: the model. Frame 2: iteration 0 reads w[0]. Frame 3: it reads x[0]. Frame 4: the
multiply-add, a sliver on the timeline. Frame 5: four iterations, 800.8 ns, of which 0.8 ns is
computation.
Drawn at the width it is shown on the slide (1100 px).
Run it to refresh ../assets/memory-naive-1.svg .. memory-naive-5.svg .
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_RED, GREEN, INK, MUTED, ORANGE,
                    RED, WHITE, brace, elbow, line, rect, save, text)

W, H = 1100, 372
READ_NS, MAC_NS = 100, 0.2
X0, CW, CY, CH = 240, 106, 74, 60         # memory strip: first cell, cell width, top, height
BAR_X, BAR_Y, BAR_H = 240, 232, 30        # timeline origin, 1 px per ns
NAMES = [f"w[{i}]" for i in range(4)] + [f"x[{i}]" for i in range(4)]
CAPTIONS = ["① 内存是一个字节序列，读多少字节就返回多少字节，一次读取需要 100 ns",
            "② 迭代 0：读 w[0]，返回 4 字节，100 ns",
            "③ 迭代 0：读 x[0]，又是 100 ns",
            "④ 迭代 0：乘加 0.2 ns，一次迭代共 200.2 ns",
            "⑤ 4 次迭代共 800.8 ns，其中乘加共 0.8 ns"]


def total_ns(k):
    return [0, 100, 200, 200.2, 800.8][k - 1]


def frame(k):
    out = []
    out += [rect(10, 74, 140, 60, FILL_GREY, MUTED, rx=6, width=1.6),
            text(80, 111, "处理器", 21, INK, "bold")]
    out.append(text(X0, 30, "内存：字节序列，每个单元 4 字节", 18, MUTED, "bold", "start"))

    # which cells have been read, and which is read in this frame
    done, now = set(), None
    if k == 2:
        now = 0
    elif k == 3:
        now, done = 4, {0}
    elif k == 4:
        done = {0, 4}
    elif k == 5:
        done = set(range(8))
    for i, name in enumerate(NAMES):
        hot = i == now
        fill = ORANGE if hot else FILL_ORANGE if i in done else FILL_GREEN
        stroke = ORANGE if hot or i in done else GREEN
        out.append(rect(X0 + i * CW, CY, CW, CH, fill, stroke, rx=3, width=2.4 if hot else 1.4))
        out.append(text(X0 + i * CW + CW / 2, CY + CH / 2 + 7, name, 20,
                        WHITE if hot else INK, font="monospace"))
    out += brace(X0, X0 + 4 * CW - 6, CY + CH + 10, MUTED, "数组 w", size=16)
    out += brace(X0 + 4 * CW, X0 + 8 * CW - 6, CY + CH + 10, MUTED, "数组 x", size=16)
    if now is not None:
        cx = X0 + now * CW + CW / 2
        out.append(elbow([(cx, CY), (cx, CY - 14), (80, CY - 14), (80, 74)], ORANGE, 2.8))
        out.append(text((cx + 80) / 2, CY - 20, "4 字节", 17, ORANGE, "bold"))

    # elapsed time
    out.append(text(110, BAR_Y + 4, "累计时间", 17, MUTED, "bold"))
    t = total_ns(k)
    out.append(text(110, BAR_Y + 32, f"{t:g} ns", 22, INK, "bold"))

    # timeline, 1 px per ns
    out.append(rect(BAR_X, BAR_Y, 800, BAR_H, WHITE, MUTED, rx=2, width=1))
    segs = []
    if k >= 2:
        segs.append((0, "w[0]"))
    if k >= 3:
        segs.append((100, "x[0]"))
    if k == 5:
        for i in range(1, 4):
            segs += [(200 * i, f"w[{i}]"), (200 * i + 100, f"x[{i}]")]
    for start, label in segs:
        out.append(rect(BAR_X + start, BAR_Y, READ_NS, BAR_H, FILL_RED, RED, rx=2, width=1.4))
        out.append(text(BAR_X + start + READ_NS / 2, BAR_Y + 21, label, 15, INK,
                        font="monospace"))
    macs = [200] if k == 4 else [200 * i for i in range(1, 5)] if k == 5 else []
    for end in macs:
        out.append(rect(BAR_X + end, BAR_Y - 4, 2.4, BAR_H + 8, GREEN, GREEN, rx=0, width=1))
    if k == 4:
        out.append(text(BAR_X + 200, BAR_Y - 12, "乘加 0.2 ns", 16, GREEN, "bold", "start"))
    if k == 5:
        out.append(text(BAR_X + 800, BAR_Y - 12, "每次迭代的乘加 0.2 ns", 16, GREEN, "bold",
                        "end"))

    # axis
    ay = BAR_Y + BAR_H + 8
    out.append(line(BAR_X, ay, BAR_X + 800, ay, MUTED, 1.2))
    for ns in range(0, 801, 200):
        out.append(line(BAR_X + ns, ay, BAR_X + ns, ay + 6, MUTED, 1.2))
        out.append(text(BAR_X + ns, ay + 24, f"{ns} ns", 15, MUTED))

    # legend
    ly = 322
    out.append(rect(BAR_X, ly - 14, 24, 17, FILL_RED, RED, rx=3, width=1.4))
    out.append(text(BAR_X + 32, ly, "读内存 100 ns", 16, MUTED, anchor="start"))
    out.append(rect(BAR_X + 190, ly - 14, 24, 17, GREEN, GREEN, rx=3, width=1.4))
    out.append(text(BAR_X + 222, ly, "乘加 0.2 ns", 16, MUTED, anchor="start"))
    out.append(text(W / 2, H - 8, CAPTIONS[k - 1], 20, INK, "bold"))
    return out


if __name__ == "__main__":
    for k in range(1, 6):
        save(f"memory-naive-{k}", W, H, frame(k))
