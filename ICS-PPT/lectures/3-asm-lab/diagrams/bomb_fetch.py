#!/usr/bin/env python3
"""The local PC and the server of the course: make bomb sends the
student number and receives the bomb of that number; the running bomb
reports every defused phase and every explosion; bomb -s reads the
scoreboard. Run it to refresh ../assets/bomb_fetch.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, GREEN, INK, MONO, MUTED,
                    arrow, mono, rect, save, text)

W, H = 1000, 320
LX, LW = 30, 300                    # the local PC
RX, RW = 670, 300                   # the server
TOP, BOT = 40, 290


def column(x, w, title, fill, stroke):
    out = [rect(x, TOP, w, BOT - TOP, fill, stroke, rx=8, width=1.6),
           text(x + w / 2, TOP + 28, title, 18, INK, "bold")]
    return out


def file_row(x, y, name, note, w=260):
    return [rect(x, y, w, 30, "#ffffff", MUTED, rx=3, width=1.2),
            mono(x + 10, y + 21, name, 15, INK),
            text(x + w - 10, y + 21, note, 13, MUTED, anchor="end")]


def build():
    out = []
    out += column(LX, LW, "本地 PC", FILL_GREY, MUTED)
    out += file_row(LX + 20, TOP + 50, "Makefile", "取得 bomb")
    out += file_row(LX + 20, TOP + 88, "README.md", "实验说明")
    out += file_row(LX + 20, TOP + 126, "bomb-light", "课堂演示的程序")
    out.append(rect(LX + 20, TOP + 176, 260, 54, FILL_BLUE, BLUE, rx=3, width=1.6))
    out.append(mono(LX + 30, TOP + 199, "bomb", 17, INK, "bold"))
    out.append(text(LX + 270, TOP + 199, "这个学号的程序", 13, MUTED, anchor="end"))
    out.append(text(LX + 150, TOP + 220, "8 个阶段，运行时保持与服务器的连接", 13, MUTED))

    out += column(RX, RW, "课程服务器", FILL_GREEN, GREEN)
    out.append(text(RX + RW / 2, TOP + 66, "按学号链接出一个 bomb", 15, INK))
    out.append(text(RX + RW / 2, TOP + 90, "同一个学号每次得到相同的 bomb", 13, MUTED))
    out.append(rect(RX + 20, TOP + 126, 260, 104, "#ffffff", GREEN, rx=3, width=1.2))
    out.append(text(RX + RW / 2, TOP + 150, "计分板", 15, INK, "bold"))
    out.append(text(RX + RW / 2, TOP + 174, "每个 bomb 一行：", 13, MUTED))
    out.append(text(RX + RW / 2, TOP + 194, "已拆除的阶段数、爆炸的次数", 13, MUTED))
    out.append(text(RX + RW / 2, TOP + 214, "学号由 8 位十六进制的编号代替", 13, MUTED))

    # the three exchanges
    mid = (LX + LW + RX) / 2
    y1 = TOP + 60
    out.append(arrow(LX + LW + 6, y1, RX - 6, y1, INK, 2))
    out.append(mono(mid, y1 - 10, "make bomb ID=学号", 15, INK, "bold", anchor="middle"))
    out.append(text(mid, y1 + 20, "把学号发给服务器", 13, MUTED))
    y2 = y1 + 56
    out.append(arrow(RX - 6, y2, LX + LW + 6, y2, BLUE, 2))
    out.append(text(mid, y2 - 10, "传回这个学号的 bomb", 15, INK, "bold"))
    out.append(text(mid, y2 + 20, "保存为当前目录下的文件 bomb", 13, MUTED))
    y3 = y2 + 66
    out.append(arrow(LX + LW + 6, y3, RX - 6, y3, GREEN, 2))
    out.append(mono(mid, y3 - 10, "./bomb", 15, INK, "bold", anchor="middle"))
    out.append(text(mid, y3 + 20, "每个被拆除的阶段、每一次爆炸都记在学号下", 13, MUTED))
    y4 = y3 + 56
    out.append(arrow(RX - 6, y4, LX + LW + 6, y4, GREEN, 2))
    out.append(mono(mid, y4 - 10, "./bomb -s", 15, INK, "bold", anchor="middle"))
    out.append(text(mid, y4 + 20, "读取并显示计分板", 13, MUTED))
    return out


if __name__ == "__main__":
    save("bomb_fetch", W, H, build())
