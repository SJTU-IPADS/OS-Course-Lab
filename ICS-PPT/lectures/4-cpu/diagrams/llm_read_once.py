#!/usr/bin/env python3
"""Contrast sequential generation with processing known prompt words together.

Left: decode, one vector in, one word out per pass. Right: four known prompt
words share the weights and produce four sets of next-word scores. Run it to refresh
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


def panel(x, title, inputs, caption, conclusion):
    out = [text(x + PANEL_W / 2, 24, title, 17, INK, "bold")]
    y0 = 50
    wx = x + (PANEL_W - WB_W) / 2
    out += box(wx, y0, WB_W, WB_H, "各层稠密权重", FILL_GREY, INK, 17, "bold",
               sub="126 层，约 810 GB（16 位）", sub_size=12)
    pitch = 36
    n = len(inputs)
    top = y0 + WB_H / 2 - (n - 1) * pitch / 2
    out.append(text(wx - 40 - IN_W / 2, top - IN_H / 2 - 12,
                    "词对应的向量", 12, MUTED))
    if n > 1:
        out.append(text(wx + WB_W + 40 + OUT_W / 2, top - IN_H / 2 - 12,
                        "候选词分数", 12, MUTED))
    for i, word in enumerate(inputs):
        y = top + i * pitch
        out += box(wx - 40 - IN_W, y - IN_H / 2, IN_W, IN_H, f"「{word}」",
                   FILL_BLUE, BLUE, 14, "normal")
        out.append(arrow(wx - 40 + 2, y, wx - 2, y, INK, 1.8))
        out.append(arrow(wx + WB_W + 2, y, wx + WB_W + 40 - 2, y, INK, 1.8))
        out += box(wx + WB_W + 40, y - IN_H / 2, OUT_W, IN_H, f"分数 {i + 1}" if n > 1 else "下一个词",
                   FILL_ORANGE, ORANGE, 14, "bold")
    out.append(text(x + PANEL_W / 2, y0 + WB_H + 30, caption, 15, INK))
    out.append(text(x + PANEL_W / 2, y0 + WB_H + 54, conclusion, 15, GREEN, "bold"))
    return out


def build():
    out = panel(10, "逐词生成：每次生成一个新词", ["是"],
                "主要权重约扫描一遍，生成 1 个词", "下一个词确定后，再进行下一次计算")
    out += panel(570, "提示词已确定：多个词一起计算", ["流水线", "提高", "的", "是"],
                 "一次处理这 4 个词，复用各层权重", "减少每个词摊分的显存读取量")
    return out


if __name__ == "__main__":
    save("llm-read-once", W, H, build())
