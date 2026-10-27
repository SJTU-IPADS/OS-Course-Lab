#!/usr/bin/env python3
"""Watching a video: download, decode and display as a three-stage pipeline.

Three stages in the station colours of car_wash.py and assembly_line.py,
each busy with a different segment: the network downloads segment 3 while
the decoder works on segment 2 and the screen shows segment 1. Between two
stages a buffer, drawn as a queue of slots: compressed segments (blue, the
download colour) before the decoder, decoded frames (orange, the decode
colour) before the screen. The dashed slots are the empty part of a buffer.
Run it to refresh ../assets/video-pipeline.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, MUTED, WHITE, arrow, rect, save, text
from timegrid import TONE

STAGES = [("下载", "取指", "段 3", "从网络取压缩数据"),
          ("解码", "执行", "段 2", "还原成一帧帧画面"),
          ("显示", "访存", "段 1", "每 16.7 ms 换一帧")]
BUFFERS = [("取指", 2, "缓冲区：已下载、未解码"), ("执行", 3, "缓冲区：已解码、未显示")]
SW, SH = 150, 84                     # a stage box
SLOT, NSLOT = 22, 5                  # a buffer slot and the slots per buffer
BW = NSLOT * SLOT
GAP = 26                             # arrow room on either side of a buffer
W = 1120
END = 60
X0 = (W - (END + 3 * SW + 2 * (BW + 2 * GAP) + END)) / 2
SY = 26
MY = SY + SH / 2
H = SY + SH + 44


def buffer(x, tone, full, label):
    fill, stroke, _ = TONE[tone]
    out = []
    for k in range(NSLOT):
        filled = k >= NSLOT - full   # the queue drains from the right
        out.append(rect(x + k * SLOT, MY - 18, SLOT, 36, fill if filled else WHITE,
                        stroke if filled else MUTED, rx=2, width=1.2,
                        dash=None if filled else "4 3"))
    out.append(text(x + BW / 2, SY + SH + 30, label, 14, MUTED))
    return out


def build():
    x = X0 + END
    out = [text(x - 32, MY + 5, "网络", 15, MUTED, anchor="end")]
    out.append(arrow(x - 26, MY, x - 4, MY, INK, 2))
    for k, (name, tone, seg, what) in enumerate(STAGES):
        fill, stroke, ink = TONE[tone]
        out.append(text(x + SW / 2, SY - 10, f"正在处理{seg}", 14, ink, "bold"))
        out.append(rect(x, SY, SW, SH, fill, stroke, rx=8, width=1.8))
        out.append(text(x + SW / 2, SY + 34, name, 20, ink, "bold"))
        out.append(text(x + SW / 2, SY + 60, what, 13, INK))
        x += SW
        if k < 2:
            out.append(arrow(x + 4, MY, x + GAP - 4, MY, INK, 2))
            out += buffer(x + GAP, *BUFFERS[k])
            out.append(arrow(x + GAP + BW + 4, MY, x + 2 * GAP + BW - 4, MY, INK, 2))
            x += 2 * GAP + BW
    out.append(arrow(x + 4, MY, x + 26, MY, INK, 2))
    out.append(text(x + 32, MY + 5, "屏幕", 15, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("video-pipeline", W, H, build())
