#!/usr/bin/env python3
"""The 100 points of lab 1 as one bar: four parts, the items of each, and the
cumulative score after each part (tests/lab1/run.sh).

Run it to refresh ../assets/cpu_score.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, INK, MUTED, ORANGE, rect, save, text
from cpu_kit import code, note

W, H = 1120, 150
X0, BW, BH = 20, 1080, 46

PARTS = [("第 1 部分 modrm.c", 30, "3 项 × 10 分", FILL_ORANGE),
         ("第 2 部分 alu.c", 30, "3 项 × 10 分", FILL_BLUE),
         ("第 3 部分 interp.c 整数指令", 30, "10 项 × 3 分", FILL_GREEN),
         ("第 4 部分", 10, "5 项 × 2 分", FILL_GREY)]


def build():
    out = []
    x, total = X0, 0
    for name, pts, items, fill in PARTS:
        w = BW * pts / 100
        total += pts
        out.append(rect(x, 30, w, BH, fill, INK, rx=0, width=1.2))
        out.append(text(x + w / 2, 50, name, 14, INK, "bold"))
        out.append(text(x + w / 2, 68, items, 12, MUTED))
        out.append(text(x + w, 96, f"{total}", 14, ORANGE, "bold"))
        x += w
    out.append(text(X0, 96, "0", 14, MUTED, "bold"))
    out.append(note(X0, 120, "第 4 部分是向量指令。依次完成四个部分时的得分；一项全部通过才得分；一个部分的项目只依赖这一部分与它之前的部分", 13, INK))
    out.append(note(X0, 140, "随机参数由 make check 第一行的种子决定，SEED=种子 make check 重复同一次评测", 13))
    return out


if __name__ == "__main__":
    save("cpu_score", W, H, build())
