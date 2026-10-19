#!/usr/bin/env python3
"""The AVX2 matvec_q4_0 at the level of one row: a block at a time into 8
partial sums, and the horizontal sum at the end.

Top: the row's 64 blocks. Middle: one block becomes 32 codes, meets the 32
floats of x as 4 groups of 8 lanes, and its 8-lane sum bsum is scaled by d
and added to the row's 8 accumulators acc; the loop goes back for the next
block. Bottom: hsum8 adds the 8 lanes of acc into y[r], one row per step;
each row is a whole %ymm, and the lanes whose data is no longer needed are
drawn dashed. Lane 0 is at the right, as in pd-class-mac and lecture 3.
Colours: codes blue, x grey, d gold, products orange, sums green.
Run it to refresh ../assets/pd-class-plan.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, FONT, GREEN, INK,
                    LINE, MONO, MUTED, ORANGE, WHITE, arrow, box, circle, elbow, line,
                    rect, save, text)

W, H = 1120, 374
GOLD, FILL_GOLD = "#b8860b", "#fbf2dc"

RX, RY, RC, RH = 80, 30, 15, 22        # the row: 64 blocks
BLOCK = 5                              # the block followed
MY = 96                                # the middle band
DX = 18                                # the middle band right of the opened block
LOOP_X = 308                           # the loop back, between the opened block and the arrow
SEP = 240                              # the line above the hsum8 band
SX, SC, SH, SGAP = 514, 52, 22, 10     # the hsum8 rows: x, lane width, height, gap


def lanes(x, y, label, live=8, cw=24, ch=26, size=10.5):
    """The 8 lanes of a %ymm, lane 0 at the right; lanes from `live` up are
    no longer needed and are drawn dashed, without a label."""
    out = []
    for i in range(8):
        cx = x + (7 - i) * cw
        if i < live:
            out.append(rect(cx, y, cw, ch, FILL_GREEN, GREEN, rx=2, width=1.2))
            s = label(i)
            out.append(text(cx + cw / 2, y + ch / 2 + size * 0.36, s, size, INK,
                            font=MONO if s.isascii() else FONT))
        else:
            out.append(rect(cx + 1, y + 1, cw - 2, ch - 2, WHITE, MUTED, rx=2, width=1, dash="3 3"))
    return out, x + 8 * cw


def build():
    out = []
    # the row and its 64 blocks
    out.append(text(RX, RY - 10, "第 r 行：64 个块（cols = 2048）", 14, INK, "bold", anchor="start"))
    for b in range(64):
        out.append(rect(RX + b * RC, RY, RC, RH, FILL_BLUE, BLUE, rx=1, width=0.8))
    bx = RX + BLOCK * RC
    out.append(rect(bx, RY, RC, RH, WHITE, INK, rx=1, width=2))
    out.append(text(bx + RC - 3, RY + RH + 15, "块 b", 11.5, INK, "bold", anchor="end"))   # clear of the arrow into b + 1
    out.append(text(RX + 63 * RC + RC / 2, RY + RH + 15, "块 63", 11.5, MUTED))
    out.append(text(RX + 40 * RC, RY + RH + 15, "按块处理，对 64 个块重复", 12, MUTED))

    # the block opened: d and 16 code bytes
    out.append(line(bx, RY + RH, 80, MY, LINE, 1.1, dash="4 4"))
    out.append(line(bx + RC, RY + RH, 80 + 18 * 12, MY, LINE, 1.1, dash="4 4"))
    for i in range(18):
        scale = i < 2
        out.append(rect(80 + i * 12, MY, 12, 22, FILL_GOLD if scale else FILL_BLUE,
                        GOLD if scale else BLUE, rx=1, width=1))
    out.append(text(80 + 12, MY + 38, "d", 12, GOLD, "bold", font=MONO))
    out.append(text(80 + 10 * 12, MY + 38, "16 个字节", 12, BLUE, "bold"))
    # decode
    out.append(arrow(LOOP_X + 12, MY + 11, 330 + DX - 4, MY + 11, INK, 1.6))
    out += box(330 + DX, MY - 12, 200, 46, "拆出 32 个编码", FILL_BLUE, BLUE, 14, sub="减 8，转为 float")
    out.append(arrow(530 + DX + 4, MY + 11, 560 + DX - 4, MY + 11, INK, 1.6))
    out += box(560 + DX, MY - 12, 230, 46, "与 x[32b .. 32b+31] 相乘", FILL_ORANGE, ORANGE, 14,
               sub="4 组 × 8 路，积累加到 bsum")
    out.append(arrow(790 + DX + 4, MY + 11, 830 + DX - 4, MY + 11, INK, 1.6))
    bs = 830 + DX                        # bsum
    shapes, end = lanes(bs, MY - 2, lambda i: f"b{i}")
    out += shapes
    out.append(text(bs + 96, MY + 40, "bsum：块的 8 路部分和", 12, GREEN, "bold"))

    # scale by d and accumulate
    y2 = MY + 88
    out.append(arrow(bs + 96, MY + 48, bs + 96, y2 - 20, INK, 1.6))
    out.append(circle(bs + 96, y2 - 2, 15, FILL_GOLD, GOLD, 1.8))
    out.append(text(bs + 96, y2 + 4, "×d", 14, GOLD, "bold"))
    out.append(arrow(bs + 96 - 15, y2 - 2, 700 + 4, y2 - 2, INK, 1.6))
    out.append(text((bs + 81 + 704) / 2, y2 - 10, "每个块乘一次 d", 11.5, MUTED))
    out.append(circle(685, y2 - 2, 15, FILL_GREEN, GREEN, 1.8))
    out.append(text(685, y2 + 4, "+", 18, GREEN, "bold"))
    out.append(text(685, y2 + 26, "累加", 11.5, MUTED))
    out.append(arrow(670, y2 - 2, 460 + 4 + 192, y2 - 2, INK, 1.6))
    shapes, end = lanes(460, y2 - 15, lambda i: f"s{i}")
    out += shapes
    out.append(text(460 + 96, y2 + 30, "acc：行的 8 路累加器，一个 %ymm", 12, GREEN, "bold"))
    out.append(text(460 + 96, y2 + 46, "每一路累加每第 8 个乘积", 11.5, MUTED))
    # the loop back to the next block
    nx = bx + RC + RC / 2                # the middle of block b + 1
    out.append(elbow([(460 - 6, y2 - 2), (LOOP_X, y2 - 2), (LOOP_X, RY + RH + 26), (nx, RY + RH + 26),
                      (nx, RY + RH + 4)], ORANGE, 1.8, dash="6 4"))
    # on the lower run, where nothing else is drawn
    out.append(text((LOOP_X + 454) / 2, y2 - 8, "下一个块 b + 1", 11.5, ORANGE, "bold"))

    # hsum8: one row per step, each a whole %ymm
    out.append(line(60, SEP, 1060, SEP, LINE, 1, "4 4"))
    rows = [("hsum8：把 acc 的 8 个 float 相加为 1 个，写入 y[r]", 8, lambda i: f"s{i}"),
            ("高 128 位的 4 个加到低 128 位的 4 个上", 4, lambda i: f"s{i}+s{i + 4}"),
            ("相邻两个相加", 2, lambda i: "4 个之和"),
            ("相邻两个相加", 1, lambda i: "8 个之和")]
    for k, (what, live, label) in enumerate(rows):
        y = SEP + 10 + k * (SH + SGAP)
        shapes, end = lanes(SX, y, label, live, cw=SC, ch=SH, size=10)
        out += shapes
        if k == 0:
            out.append(text(SX - 16, y + SH / 2 + 5, what, 14, INK, "bold", anchor="end"))
        else:
            out.append(text(SX - 16, y + SH / 2 + 4, what, 12, INK, anchor="end"))
    out.append(arrow(end + 6, y + SH / 2, end + 44, y + SH / 2, GREEN, 1.6))
    out += box(end + 50, y - 3, 80, SH + 6, "y[r]", FILL_GREEN, GREEN, 15, font=MONO)
    # the key to the dashed lanes
    out.append(rect(80, y + 3, 26, SH - 6, WHITE, MUTED, rx=2, width=1, dash="3 3"))
    out.append(text(114, y + SH / 2 + 4, "虚线：这一路的数据不再需要", 11.5, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("pd-class-plan", W, H, build(), left=23)
