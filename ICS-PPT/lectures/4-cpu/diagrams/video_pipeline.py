#!/usr/bin/env python3
"""Watching a video: download, decode, display, with buffers between.

Three stages working on three different segments at once. The display
stage has a fixed beat (one frame every 16.7 ms); an empty buffer in front
of it is the spinner everyone has seen. Run it to refresh
../assets/video-pipeline.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK, MUTED,
                    ORANGE, RED, arrow, box, save, text)

W = 1120
SW, SH = 210, 74              # a stage
BUF_W, BUF_H = 150, 46        # a buffer
GAP = 34
X0 = (W - (3 * SW + 2 * BUF_W + 4 * GAP)) / 2
Y = 20
STAGES = [("下载", "第 3 段：从服务器取压缩数据", FILL_BLUE, BLUE),
          ("解码", "第 2 段：还原成一帧帧画面", FILL_ORANGE, ORANGE),
          ("显示", "第 1 段：每 16.7 ms 换一帧", FILL_GREEN, GREEN)]
BUFS = [("缓冲区", "已下载、未解码"), ("缓冲区", "已解码、未显示")]
STALLS = ["网络慢，这里读空：画面停住转圈", "显示的节拍固定：解码停下等它"]


def build():
    out = []
    x = X0
    for i, (name, sub, fill, stroke) in enumerate(STAGES):
        out += box(x, Y, SW, SH, name, fill, stroke, 18, "bold", sub=sub, sub_size=12)
        x += SW
        if i < 2:
            out.append(arrow(x + 2, Y + SH / 2, x + GAP - 2, Y + SH / 2, INK, 2))
            x += GAP
            name_b, sub_b = BUFS[i]
            out += box(x, Y + (SH - BUF_H) / 2, BUF_W, BUF_H, name_b, FILL_GREY, MUTED, 14,
                       "normal", color=INK, sub=sub_b, sub_size=11)
            out.append(text(x + BUF_W / 2, Y + SH + 28, STALLS[i], 13, RED))
            x += BUF_W
            out.append(arrow(x + 2, Y + SH / 2, x + GAP - 2, Y + SH / 2, INK, 2))
            x += GAP
    return out


if __name__ == "__main__":
    save("video-pipeline", W, Y + SH + 44, build())
