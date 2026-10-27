#!/usr/bin/env python3
"""Decode on a pipeline: word 2 cannot enter GPU 1 until GPU 4 takes word 1.

Same four GPU rows. Word 1 walks GPU 1..4 in cycles 1..4; its result is the
input of word 2, which enters GPU 1 in cycle 5. Everything else is 停: with a
single request the pipeline is as empty as llm-pp-single. Run it to refresh
../assets/llm-pp-decode.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, RED, elbow, rect, save, text
from timegrid import CH, CW, LX, TOP, cx, cycle_numbers, row

W, H = 1120, 356
PITCH = CH + 8
GPUS = ["GPU 1（1–7 层）", "GPU 2（8–14 层）", "GPU 3（15–21 层）", "GPU 4（22–28 层）"]
NCOL = 8


def build():
    out = [text(LX + 4 * CW, 22, "第 4 拍 GPU 4 取出词 1；词 1 是词 2 的输入，词 2 第 5 拍才能进入 GPU 1",
                16, INK, "bold")]
    out += cycle_numbers(NCOL)
    for r, label in enumerate(GPUS):
        cells = ["停"] * NCOL
        cells[r] = "词 1"
        cells[r + 4] = "词 2"
        out += row(label, TOP + r * PITCH, 0, cells)
    y4 = TOP + 3 * PITCH + CH / 2            # centre of the GPU 4 row
    y1 = TOP + CH / 2                        # centre of the GPU 1 row
    xg = LX + 4 * CW                         # the gap between cycle 4 and cycle 5
    out.append(elbow([(LX + 4 * CW - 4, y4), (xg, y4), (xg, y1), (LX + 4 * CW + 4, y1)],
                     INK, 2.4))
    out.append(rect(LX + CW + 1, TOP - 4, 3 * CW - 2, CH + 8, "none", RED, rx=6, width=2, dash="6 4"))
    bottom = TOP + 3 * PITCH + CH
    out.append(text(cx(2), bottom + 28, "GPU 1 在第 2、3、4 拍没有这个请求的词可做", 16, RED, "bold"))
    return out


if __name__ == "__main__":
    save("llm-pp-decode", W, H, build(), left=-64)
