#!/usr/bin/env python3
"""One pass over the weights gives one word, or several if their inputs are known.

Left: decode, one vector in, one word out per pass. Right: four vectors go
through the same weights in one pass. Run it to refresh
../assets/llm-read-once.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, GREEN, INK, MUTED, ORANGE,
                    arrow, box, save, text)

W, H = 1120, 262
PANEL_W = 540
WB_W, WB_H = 210, 150          # the weights box
IN_W, IN_H = 92, 30            # one input vector
OUT_W = 72


def panel(x, title, n, caption, rate):
    out = [text(x + PANEL_W / 2, 24, title, 17, INK, "bold")]
    y0 = 50
    wx = x + (PANEL_W - WB_W) / 2
    out += box(wx, y0, WB_W, WB_H, "全部权重", FILL_GREY, INK, 17, "bold",
               sub="28 层，0.968 GB，读一遍", sub_size=13)
    pitch = 36
    top = y0 + WB_H / 2 - (n - 1) * pitch / 2
    for i in range(n):
        y = top + i * pitch
        out += box(wx - 40 - IN_W, y - IN_H / 2, IN_W, IN_H, f"向量 {i + 1}" if n > 1 else "向量",
                   FILL_BLUE, BLUE, 14, "normal")
        out.append(arrow(wx - 40 + 2, y, wx - 2, y, INK, 1.8))
        out.append(arrow(wx + WB_W + 2, y, wx + WB_W + 40 - 2, y, INK, 1.8))
        out += box(wx + WB_W + 40, y - IN_H / 2, OUT_W, IN_H, f"词 {i + 1}" if n > 1 else "词",
                   FILL_ORANGE, ORANGE, 14, "bold")
    out.append(text(x + PANEL_W / 2, y0 + WB_H + 30, caption, 15, INK))
    out.append(text(x + PANEL_W / 2, y0 + WB_H + 54, rate, 15, GREEN, "bold"))
    return out


def build():
    out = panel(10, "逐词生成：一个词的输入要等上一个词", 1,
                "读一遍权重，得到 1 个词", "实测 190.2 词/秒")
    out += panel(570, "几个词的输入都已知（如提示词）", 4,
                 "读一遍权重，同时得到 4 个词", "实测 1455.8 词/秒")
    return out


if __name__ == "__main__":
    save("llm-read-once", W, H, build())
