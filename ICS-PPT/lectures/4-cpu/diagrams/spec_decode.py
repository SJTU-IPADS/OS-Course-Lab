#!/usr/bin/env python3
"""Speculative decoding: a small model guesses, the large model verifies.

Three rows: the four tokens the small model guessed in a row, the one
forward pass of the large model that checks all four positions, and what
comes out of that pass -- the guesses accepted up to the first wrong one,
the large model's own token at that position, the rest discarded.  The
column on the right names the processor mechanism each step corresponds
to.  Run it to refresh ../assets/spec-decode.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, FILL_RED, GREEN,
                    INK, LINE, MUTED, ORANGE, RED, arrow, box, save, text)

W, H = 1120, 282
LX, BX = 20, 214
BW, BH, GAP = 104, 38, 14
K = 4
YA, YB, YC = 22, 106, 196

VERDICT = [("接受", GREEN, FILL_GREEN), ("接受", GREEN, FILL_GREEN),
           ("改为大模型的 t3′", ORANGE, FILL_ORANGE), ("丢弃", RED, FILL_RED)]
OUT = ["t1", "t2", "t3′", "×"]


def col_x(i):
    return BX + i * (BW + GAP)


def build():
    out = []
    span = K * BW + (K - 1) * GAP
    # row A: the guesses
    out.append(text(LX, YA + BH / 2 + 6, f"小模型连猜 {K} 个", 16, INK, "bold", anchor="start"))
    for i in range(K):
        out += box(col_x(i), YA, BW, BH, f"猜 t{i + 1}", FILL_GREY, LINE, 15, "normal")
        out.append(arrow(col_x(i) + BW / 2, YA + BH, col_x(i) + BW / 2, YB - 2, INK, 1.4))
    # row B: one forward pass of the large model
    out.append(text(LX, YB + BH / 2 + 6, "大模型一次前向", 16, INK, "bold", anchor="start"))
    out += box(BX, YB, span, BH, f"读一遍权重，同时检查 {K} 个位置", FILL_BLUE, BLUE, 15, "normal")
    for i, (label, color, _) in enumerate(VERDICT):
        cx = col_x(i) + BW / 2
        out.append(text(cx, YB + BH + 20, label, 14, color, "bold"))
        out.append(arrow(cx, YB + BH + 28, cx, YC - 2, color, 1.4))
    # row C: what comes out
    out.append(text(LX, YC + BH / 2 + 6, "这一次的输出", 16, INK, "bold", anchor="start"))
    for i, (label, color, fill) in enumerate(VERDICT):
        out += box(col_x(i), YC, BW, BH, OUT[i], fill, color, 16, "bold",
                   RED if OUT[i] == "×" else INK)
    out.append(text(BX + span / 2, YC + BH + 30,
                    "一次前向产出 3 个 token；逐个 decode 时产出 3 个 token 要读 3 遍权重",
                    15, MUTED))
    # the processor column
    rx = 720
    out.append(text(rx, YA + BH / 2 + 6, "对应处理器的做法", 16, INK, "bold", anchor="start"))
    for i, s in enumerate(["猜：分支预测，判定前按猜测继续取指",
                           "验：条件跳转在执行阶段判定",
                           "丢弃：作废猜错之后取入的指令",
                           "输出与大模型逐个 decode 相同：推测不改变结果"]):
        out.append(text(rx, YA + 50 + i * 34, s, 15, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("spec-decode", W, H, build())
