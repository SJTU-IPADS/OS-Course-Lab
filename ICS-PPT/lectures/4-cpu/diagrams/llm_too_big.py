#!/usr/bin/env python3
"""The 810 GB of Llama 3.1 405B cut into 80 GB pieces, one per H100.

One bar, to scale: ten full 80 GB cells and a last piece of 10 GB, whose
cell is drawn dashed for the part of the GPU it leaves empty. The weights are
grey, as the weight matrices of llm_layers.py. Under every cell is the GPU it
goes to; the bracket gives the count.
Run it to refresh ../assets/llm-too-big.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import FILL_GREY, INK, MUTED, WHITE, brace, rect, save, text

TOTAL, PIECE = 810, 80               # GB
S = 90                               # px per 80 GB
N = -(-TOTAL // PIECE)               # 11 pieces
W, H = 1120, 156
X0 = (W - N * S) / 2
BY, BH = 36, 44                      # the bar: top, height


def build():
    out = [text(X0, 22, "Llama 3.1 405B 的权重：4050 亿个 × 2 字节 = 810 GB", 16, INK, "bold",
                anchor="start"),
           text(X0 + N * S, 22, "一格 = 一块 H100 的显存（80 GB）", 14, MUTED, anchor="end")]
    for k in range(N):
        x = X0 + k * S
        left = TOTAL - k * PIECE
        if left >= PIECE:
            out.append(rect(x, BY, S, BH, FILL_GREY, INK, rx=2, width=1.4))
            out.append(text(x + S / 2, BY + BH / 2 + 5, "80 GB", 14, INK))
        else:
            part = S * left / PIECE
            out.append(rect(x, BY, S, BH, WHITE, MUTED, rx=2, width=1.4, dash="5 4"))
            out.append(rect(x, BY, part, BH, FILL_GREY, INK, rx=2, width=1.4))
            out.append(text(x + part + 5, BY + BH / 2 + 5, f"← {left} GB", 13, INK,
                            anchor="start"))
        out.append(text(x + S / 2, BY + BH + 22, f"GPU {k + 1}", 14, INK))
    out += brace(X0 + 2, X0 + N * S - 2, BY + BH + 32, INK,
                 f"{TOTAL} ÷ {PIECE} ≈ 10.1：至少 {N} 块 GPU", 16, depth=8)
    return out


if __name__ == "__main__":
    save("llm-too-big", W, H, build())
