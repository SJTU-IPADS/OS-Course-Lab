#!/usr/bin/env python3
"""The stack frame of chat_turn in the demo attack1 (student ID 000000000001):
the 304-byte buffer at %rbp-0x130, the saved %rbp and the return address at
%rbp+8, with the offsets the payload counts (0, 304, 312). The addresses and
numbers come from the demo build in ../examples/attack1. The window starts at
x=280, cropping the empty left margin, because the page shows this figure in a
side column. Run it to refresh ../assets/attack_frame.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, MUTED, ORANGE,
                    WHITE, arrow, mono, rect, save, text)

W, H = 720, 346                   # the visible window
LEFT = 280                        # ... its left edge: the drawing starts at 280
CX = LEFT + W / 2                 # the centre of the window, for the captions
X0, X1 = 400, 730                 # the frame column: left edge, right edge


def row(y, h, label, value, fill, stroke, note):
    out = [rect(X0, y, X1 - X0, h, fill, stroke, rx=2, width=1.6)]
    out.append(text(X0 + 12, y + h / 2 + 5, label, 15, INK, "bold", anchor="start"))
    if value:
        out.append(mono(X0 + 118, y + h / 2 + 5, value, 13, INK))
    if note:
        out.append(text(X1 + 14, y + h / 2 + 5, note, 13, MUTED, anchor="start"))
    return out


def build():
    out = [text(CX, 26, "chat_turn 的栈帧", 17, INK, "bold")]
    # the return address
    out += row(36, 46, "返回地址", "0x00000000004025bc", WHITE, BLUE, "call chat_turn 的下一条指令")
    out += [mono(X0 - 12, 56, "+312", 13, MUTED, anchor="end"),
            mono(X0 - 12, 74, "%rbp+8", 13, ORANGE, "bold", anchor="end")]
    # the saved %rbp
    out += row(82, 46, "保存的 %rbp", "0x00007fff…", FILL_GREY, MUTED, "调用者的帧")
    out += [mono(X0 - 12, 102, "+304", 13, MUTED, anchor="end"),
            mono(X0 - 12, 120, "%rbp", 13, MUTED, anchor="end")]
    # the buffer
    out += row(128, 176, "", "", FILL_ORANGE, ORANGE, None)
    out += [text(X0 + 12, 150, "缓冲区", 15, INK, "bold", anchor="start"),
            text(X0 + 76, 150, "304 字节", 13, MUTED, anchor="start"),
            mono(X0 + 150, 150, "char prompt[304]", 12, MUTED),
            text(X1 + 14, 150, "Gets 从这里写入", 13, ORANGE, "bold", anchor="start"),
            text(X1 + 14, 170, "不限长度", 13, ORANGE, anchor="start")]
    out += [mono(X0 - 12, 292, "+0", 13, MUTED, anchor="end"),
            mono(X0 - 12, 310, "%rbp-0x130", 13, MUTED, anchor="end")]
    # the input grows toward higher addresses
    out.append(arrow(X1 - 26, 286, X1 - 26, 158, ORANGE, 2.2))
    out.append(text(X1 - 40, 226, "输入", 13, ORANGE, "bold", anchor="end"))
    out.append(text(CX, H - 6, "地址从下往上增大；输入从缓冲区的低地址一端写起", 13, MUTED))
    return out


if __name__ == "__main__":
    save("attack_frame", W, H, build(), left=LEFT)
