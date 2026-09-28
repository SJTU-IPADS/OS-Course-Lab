#!/usr/bin/env python3
"""From parameter count to seconds per token, as a staircase.

7e9 multiply-adds x 6 instructions (the -O2 scalar loop) = 4.2e10
instructions; at IPC 3.82 that is 1.1e10 cycles; at 4.0 GHz, about 2.75 s,
i.e. fewer than 0.4 tokens per second.
Run it to refresh ../assets/token-time.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_RED, INK, MUTED, RED, box,
                    elbow, save, text)

W, H = 1120, 380
BW, BH = 200, 76

STEPS = [("7B 参数模型", "LLaMA-7B", None, FILL_BLUE, BLUE),
         ("7 × 10⁹ 次乘加", "每个 Token", "每个参数 1 次乘加", FILL_BLUE, BLUE),
         ("4.2 × 10¹⁰ 条指令", "标量机器指令", "× 6 条指令 / 次乘加", FILL_GREY, INK),
         ("1.1 × 10¹⁰ 个周期", "时钟周期", "÷ IPC 3.82", FILL_GREY, INK),
         ("≈ 2.75 秒", "每个 Token", "÷ 4.0 × 10⁹ 周期 / 秒", FILL_RED, RED)]


def build():
    out = [text(20, 30, "生成 1 个 Token：单核标量执行", 18, INK, "bold", anchor="start")]
    for k, (label, sub, how, fill, stroke) in enumerate(STEPS):
        x, y = 20 + 220 * k, 280 - 62 * k
        out += box(x, y, BW, BH, label, fill, stroke, 19, sub=sub)
        if how:
            out.append(text(x + BW / 2, y + BH + 24, how, 15, MUTED, "bold"))
        if k + 1 < len(STEPS):
            ym, yn = y + BH / 2, y - 62 + BH / 2
            out.append(elbow([(x + BW, ym), (x + 210, ym), (x + 210, yn), (x + 216, yn)],
                             INK, 2))
    out.append(text(1100, 270, "每秒不到 0.4 个 Token", 20, RED, "bold", anchor="end"))
    out.append(text(1100, 300, "生成 100 个 Token 的回答需要数分钟", 16, MUTED, anchor="end"))
    return out


if __name__ == "__main__":
    save("token-time", W, H, build())
