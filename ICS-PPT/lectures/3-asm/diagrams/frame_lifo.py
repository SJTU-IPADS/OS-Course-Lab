#!/usr/bin/env python3
"""Stack frames are allocated and freed in last-in first-out order.

main calls f; f calls g, g returns, f calls h, h returns; f returns. Each
column is the stack after one of these events, high addresses at the top, so
the stack grows downward on the page. The frame at the top of the stack (the
lowest one drawn) belongs to the call that is running and is orange. Flat,
to sit under a slide's bullets.
Run it to refresh ../assets/frame-lifo.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, LINE, MONO, MUTED, ORANGE,
                    arrow, line, rect, save, text)

W, H = 1120, 206
X0, DX, FW = 176, 146, 104              # first column centre, spacing, frame width
Y0, FH = 12, 38                         # stack bottom, frame height

STEPS = [("main 开始", ["main"]),
         ("main 调用 f", ["main", "f"]),
         ("f 调用 g", ["main", "f", "g"]),
         ("g 返回", ["main", "f"]),
         ("f 调用 h", ["main", "f", "h"]),
         ("h 返回", ["main", "f"]),
         ("f 返回", ["main"])]


def column(i, event, frames):
    cx = X0 + i * DX
    out = [line(cx - FW / 2 - 8, Y0, cx + FW / 2 + 8, Y0, INK, 2.4)]
    for k, name in enumerate(frames):
        top = k == len(frames) - 1
        out.append(rect(cx - FW / 2, Y0 + k * FH, FW, FH,
                        FILL_ORANGE if top else FILL_BLUE, ORANGE if top else BLUE,
                        rx=3, width=1.8 if top else 1.4))
        out.append(text(cx, Y0 + k * FH + 26, name, 18, INK, "bold", font=MONO))
    out.append(text(cx, Y0 + 3 * FH + 30, event, 17, INK, "bold"))
    return out


def build():
    out = [text(20, Y0 + 14, "栈底", 16, INK, "bold", anchor="start"),
           text(20, Y0 + 34, "（高地址）", 15, MUTED, anchor="start"),
           arrow(56, Y0 + 44, 56, Y0 + 3 * FH - 8, ORANGE, 2.6),
           text(70, Y0 + 2 * FH + 8, "栈生长", 15, ORANGE, "bold", anchor="start"),
           text(20, Y0 + 3 * FH + 14, "低地址", 15, MUTED, anchor="start")]
    for i, (event, frames) in enumerate(STEPS):
        out += column(i, event, frames)
    ya = Y0 + 3 * FH + 46
    out.append(arrow(X0 - FW / 2, ya, X0 + 6 * DX + FW / 2, ya, LINE, 1.8))
    out.append(text(X0 + 6 * DX + FW / 2, ya + 26, "时间", 15, MUTED, anchor="end"))
    out.append(rect(X0 - FW / 2, ya + 12, 22, 16, FILL_ORANGE, ORANGE, rx=2, width=1.6))
    out.append(text(X0 - FW / 2 + 32, ya + 25, "正在执行的调用的栈帧，位于栈顶", 15, INK,
                    anchor="start"))
    return out


if __name__ == "__main__":
    save("frame-lifo", W, H, build())
