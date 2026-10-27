#!/usr/bin/env python3
"""Reading the weights once: one vector in, or four vectors in.

Two panels with the same weights (grey, as llm_layers.py) read once into
the same compute units. Left, word-by-word generation: one vector (blue)
goes in and one word (orange) comes out. Right, inputs that are all known,
as in a prompt: four vectors go in together and four words come out. Under
each panel the measured rate of examples/llm_stages.py (2026-10-07, RTX 3060
Laptop, Qwen3-VL-2B Q4_0); a new measurement updates RATES.
Run it to refresh ../assets/llm-read-once.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE, MUTED, ORANGE,
                    WHITE, arrow, box, line, rect, save, text)

W, H = 1120, 236
PW, PGAP = 520, 40                  # a panel and the gap between the two
X0 = (W - 2 * PW - PGAP) / 2
RATES = {1: "190.2", 4: "1455.8"}    # words per second
TITLE = {1: "逐词生成：一次 1 个向量", 4: "输入都已知（提示词）：一次 4 个向量"}
MX, MW, MY, MH = 190, 140, 38, 46   # the weights, relative to the panel
CY, CH = 116, 64                    # the compute box


def weights(x, y, w, h):
    out = [rect(x, y, w, h, FILL_GREY, INK, rx=3, width=1.4)]
    for k in range(1, 6):
        out.append(line(x + k * w / 6, y + 3, x + k * w / 6, y + h - 3, LINE, 1))
    out.append(line(x + 3, y + h / 2, x + w - 3, y + h / 2, LINE, 1))
    return out


def vector(x, y, n=5, cw=12, ch=14):
    return [rect(x + k * cw, y, cw, ch, FILL_BLUE, BLUE, rx=1, width=1.1) for k in range(n)]


def panel(x0, n):
    out = [text(x0 + PW / 2, 22, TITLE[n], 16, INK, "bold")]
    out += weights(x0 + MX, MY, MW, MH)
    out.append(text(x0 + MX + MW + 12, MY + MH / 2 + 5, "全部权重 0.968 GB", 14, MUTED,
                    anchor="start"))
    out.append(arrow(x0 + MX + MW / 2, MY + MH + 3, x0 + MX + MW / 2, CY - 3, INK, 2))
    out.append(text(x0 + MX + MW / 2 + 10, (MY + MH + CY) / 2 + 5, "读一遍", 14, INK, "bold",
                    anchor="start"))
    out += box(x0 + MX, CY, MW, CH, "运算单元", WHITE, INK, 16, "bold")
    mid = CY + CH / 2
    ys = [mid + (k - (n - 1) / 2) * 18 for k in range(n)]
    out.append(text(x0 + 80, CY - 8, f"{n} 个向量", 13, MUTED))
    out.append(text(x0 + 420, CY - 8, f"{n} 个词", 13, MUTED))
    for y in ys:
        out += vector(x0 + 50, y - 7)
        out.append(arrow(x0 + 114, y, x0 + MX - 4, y, BLUE, 1.4))
        out.append(arrow(x0 + MX + MW + 4, y, x0 + 392, y, ORANGE, 1.4))
        out.append(rect(x0 + 396, y - 7, 48, 14, FILL_ORANGE, ORANGE, rx=2, width=1.1))
    tone = GREEN if n > 1 else INK
    out.append(text(x0 + PW / 2, CY + CH + 34,
                    f"读一遍权重得到 {n} 个词：实测 {RATES[n]} 词/秒", 16, tone, "bold"))
    return out


def build():
    out = panel(X0, 1) + panel(X0 + PW + PGAP, 4)
    out.append(line(X0 + PW + PGAP / 2, 10, X0 + PW + PGAP / 2, H - 10, LINE, 1.2, dash="5 4"))
    return out


if __name__ == "__main__":
    save("llm-read-once", W, H, build())
