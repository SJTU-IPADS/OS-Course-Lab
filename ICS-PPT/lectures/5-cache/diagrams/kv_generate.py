#!/usr/bin/env python3
"""Autoregressive generation: the tokens so far go into the model, one new token comes out
and is appended to the input of the next step. Run it to refresh ../assets/kv-generate.svg."""

from kv_kit import *
from svgkit import box

W, H = 1000, 210
X0, CW, PITCH, Y = 40, 70, 80, 70
N = 5


def figure():
    out = [text(X0 + (N * PITCH) / 2 - 5, 40, f"输入：已有的 {N} 个 token", 17, INK, "bold")]
    for i in range(1, N + 2):
        x = X0 + (i - 1) * PITCH
        out += cell(x, Y, CW, 36, f"x{i}", "cache" if i <= N else "empty", size=17)
    out.append(arrow(X0 + (N + 1) * PITCH - 6, Y + 18, 556, Y + 18, INK, 2.2))
    out += box(560, Y - 14, 200, 64, "模型", sub="L 层 Transformer", fill=FILL_GREY, stroke=INK,
               size=19)
    out.append(arrow(760, Y + 18, 806, Y + 18, INK, 2.2))
    out.append(text(845, 40, "输出：下一个 token", 17, INK, "bold"))
    out += cell(810, Y, CW, 36, f"x{N + 1}", "new", size=17)
    cx = 845
    tx = X0 + N * PITCH + CW / 2
    out.append(elbow([(cx, Y + 36 + 4), (cx, 150), (tx, 150), (tx, Y + 36 + 8)], ORANGE, 2.4))
    out.append(text((cx + tx) / 2, 176, f"x{N + 1} 接在输入末尾，进入下一步", 17, ORANGE, "bold"))
    return out


if __name__ == "__main__":
    save("kv-generate", W, H, figure())
