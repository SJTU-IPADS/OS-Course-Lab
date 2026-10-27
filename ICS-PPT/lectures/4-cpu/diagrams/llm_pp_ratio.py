#!/usr/bin/env python3
"""Fill and drain bubbles for K = 4 GPUs and M = 8 requests.

Eleven cycles do not fit the cells of timegrid.py, so this grid draws its own
smaller cells with the same colours: request m is on GPU g in cycle m + g - 1,
the cell holds the request number in the colour of request ((m - 1) mod 4) + 1
of llm_pp_fill.py, and the two triangles of 1 + 2 + 3 = 6 dashed cells are the
bubbles. The count is on the right.
Run it to refresh ../assets/llm-pp-ratio.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, MUTED, RED, mono, rect, save, text
from timegrid import TONE

K, M = 4, 8
STEPS = M + K - 1
CW, CH, PITCH = 64, 34, 38
LX, TOP = 112, 30
W = 1120
H = TOP + (K - 1) * PITCH + CH + 8


def build():
    out = [text(LX - 12, 20, "周期", 14, MUTED, anchor="end")]
    for t in range(1, STEPS + 1):
        out.append(text(LX + (t - 0.5) * CW, 20, str(t), 14, MUTED))
    for g in range(1, K + 1):
        y = TOP + (g - 1) * PITCH
        out.append(mono(24, y + CH / 2 + 5, f"GPU {g}", 15, INK))
        for t in range(1, STEPS + 1):
            m = t - g + 1
            name = f"请求 {(m - 1) % 4 + 1}" if 1 <= m <= M else "bubble"
            fill, stroke, tone = TONE[name]
            x = LX + (t - 1) * CW
            dash = "4 3" if name == "bubble" else None
            out.append(rect(x + 2, y + 2, CW - 4, CH - 4, fill, stroke, rx=3, width=1.2, dash=dash))
            if name == "bubble":
                out.append(text(x + CW / 2, y + CH / 2 + 4, "bubble", 12, tone, "bold"))
            else:
                out.append(text(x + CW / 2, y + CH / 2 + 5, str(m), 14, tone, "bold"))
    tx = LX + STEPS * CW + 26
    mid = TOP + ((K - 1) * PITCH + CH) / 2
    total, bubble = K * STEPS, K * (K - 1)
    out.append(text(tx, mid - 24, f"共 {K} × {STEPS} = {total} 格", 16, INK, anchor="start"))
    out.append(text(tx, mid + 4, f"bubble {bubble // 2} + {bubble // 2} = {bubble} 格", 16, RED,
                    "bold", anchor="start"))
    out.append(text(tx, mid + 32, f"{bubble} ÷ {total} = {round(100 * bubble / total)}%", 16,
                    RED, "bold", anchor="start"))
    return out


if __name__ == "__main__":
    save("llm-pp-ratio", W, H, build())
