#!/usr/bin/env python3
"""Autoregressive generation: two consecutive decode steps.

Each row is one step. The tokens so far (prompt blue, generated orange) go
through the model, which gives a probability distribution over the next
token; the token taken from it is appended to the input of the next row.
Bar lengths illustrate a distribution, they are not measured. Colours follow
llm_kv_cache.py: blue for tokens already there, orange for the new one.
Run it to refresh ../assets/llm-autoregressive.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE,
                    arrow, box, elbow, rect, save, text)

W, H = 1120, 178
TOKENS = ["流水线", "提高", "的", "是"]
STEPS = [                       # (step, generated so far, distribution, taken)
    (5, [], [("吞吐", 1.0), ("速度", 0.22), ("效率", 0.15)], "吞吐"),
    (6, ["吞吐"], [("率", 1.0), ("量", 0.3), ("能力", 0.1)], "率"),
]
TX, TW, TP, TH = 84, 74, 80, 40      # token boxes: left, width, pitch, height
ROW_Y = [30, 126]                    # top of each row's boxes
MX, MW = 504, 156                    # the model box
DX, BAR = 722, 132                   # the distribution: left edge, longest bar
OX, OW = 960, 80                     # the token taken


def step(n, generated, dist, taken, y):
    out = [text(16, y + TH / 2 + 6, f"第 {n} 步", 16, INK, "bold", anchor="start")]
    seq = TOKENS + generated
    for i, tok in enumerate(seq):
        x = TX + i * TP
        new = i >= len(TOKENS)
        fill, stroke = (FILL_ORANGE, ORANGE) if new else (FILL_BLUE, BLUE)
        out.append(text(x + TW / 2, y - 8, f"x{i + 1}", 13, MUTED))
        out += box(x, y, TW, TH, tok, fill, stroke, 15, "normal")
    out.append(arrow(TX + 5 * TP - 4, y + TH / 2, MX - 2, y + TH / 2, INK, 2))
    out += box(MX, y - 6, MW, TH + 12, "模型", FILL_GREY, INK, 16, "bold")
    out.append(arrow(MX + MW, y + TH / 2, DX - 6, y + TH / 2, INK, 2))
    out.append(text(DX + 80, y - 8, f"P(x{n} | x1 … x{n - 1})", 14, INK))
    for k, (word, p) in enumerate(dist):
        by = y - 1 + k * 14
        tone = ORANGE if word == taken else MUTED
        out.append(text(DX + 34, by + 11, word, 13, tone, anchor="end"))
        out.append(rect(DX + 40, by + 1, BAR * p, 11, FILL_ORANGE if word == taken else LINE,
                        tone, rx=2, width=1))
    out.append(arrow(DX + 40 + BAR + 10, y + TH / 2, OX - 2, y + TH / 2, INK, 2))
    out.append(text((DX + 40 + BAR + 10 + OX) / 2, y + TH / 2 - 8, "取出", 13, MUTED))
    out.append(text(OX + OW / 2, y - 8, f"x{n}", 13, ORANGE, "bold"))
    out += box(OX, y, OW, TH, taken, FILL_ORANGE, ORANGE, 15, "bold")
    return out


def build():
    out = []
    for (n, generated, dist, taken), y in zip(STEPS, ROW_Y):
        out += step(n, generated, dist, taken, y)
    # x5 taken in step 5 becomes the fifth input of step 6
    y0, y1 = ROW_Y[0] + TH, ROW_Y[1] - 20
    mid = (y0 + y1) / 2 + 6
    x_in = TX + 4 * TP + TW / 2
    out.append(elbow([(OX + OW / 2, y0 + 1), (OX + OW / 2, mid), (x_in, mid), (x_in, y1)],
                     ORANGE, 2))
    out.append(text((x_in + OX) / 2 + 60, mid - 7, "追加到输入末尾", 14, ORANGE, "bold"))
    return out


if __name__ == "__main__":
    save("llm-autoregressive", W, H, build())
