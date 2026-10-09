#!/usr/bin/env python3
"""The shadow stack: a second copy of the return address, used by ret.

The upper row is the ordinary stack after an overflow: the input has filled
the buffer and overwritten the return address. The lower row is the shadow
stack, which holds return addresses only; the call stored A on both stacks,
and the overflow could not reach this one. On return the copy on the shadow
stack is authoritative: the hardware shadow stack (CET) compares the two and
raises an exception when they differ, a software one returns to A directly.
The page's bullets, not the figure, name the two implementations.

Colours as in the other figures of this section: a return address blue, what
the input wrote red.
Run it to refresh ../assets/shadow-stack.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_RED, FONT, INK, MONO, MUTED, RED, WHITE,
                    arrow, label_line, mono, rect, save, text)

W, H = 1120, 258
AX, AW = 652, 210                   # the return address slot, same x in both rows
Y1, Y2, CH = 58, 166, 54            # top of each row, cell height
BUF_X = 196                         # left edge of the overwritten region


def build():
    out = [text(BUF_X - 14, Y1 + 32, "普通的栈", 16, INK, "bold", anchor="end"),
           text(BUF_X - 14, Y2 + 32, "影子栈", 16, INK, "bold", anchor="end")]

    # the ordinary stack: everything the input wrote, up to the return address
    out.append(rect(BUF_X, Y1, AX - BUF_X, CH, WHITE, RED, rx=2, width=1.6, dash="5 4"))
    out.append(text((BUF_X + AX) / 2, Y1 + 32, "输入写入 buf 与其后的字节", 15, MUTED))
    out.append(rect(AX, Y1, AW, CH, FILL_RED, RED, rx=2, width=3))
    out.append(text(AX + AW / 2, Y1 + 32, "被改写的返回地址", 15, RED, "bold"))
    out.append(text(BUF_X, Y1 - 14, "低地址", 14, MUTED, anchor="start"))
    out.append(text(AX + AW, Y1 - 14, "高地址", 14, MUTED, anchor="end"))
    out.append(text(AX + AW + 14, Y1 + 32, "缓冲区溢出能写到这里", 15, RED, "bold",
                    anchor="start"))

    # the shadow stack: return addresses only, A still as call pushed it
    for k in range(2):
        x = AX - (2 - k) * 160
        out.append(rect(x, Y2, 160, CH, WHITE, BLUE, rx=2, width=1.4, dash="5 4"))
        out.append(text(x + 80, Y2 + 32, "更早的返回地址", 14, MUTED))
    out.append(rect(AX, Y2, AW, CH, FILL_BLUE, BLUE, rx=2, width=3))
    out += label_line(AX + 52, Y2 + 32, [("返回地址 ", FONT, BLUE, "bold"),
                                         ("A", MONO, BLUE, "bold")], 15)
    out.append(text(AX + AW + 14, Y2 + 32, "缓冲区溢出写不到这里", 15, BLUE, "bold",
                    anchor="start"))

    # on return the shadow copy is authoritative
    out.append(arrow(AX + AW / 2, Y1 + CH + 4, AX + AW / 2, Y2 - 4, INK, 2.4, both=True))
    out += label_line(AX + AW / 2 + 14, (Y1 + CH + Y2) / 2 + 5,
                      [("ret", MONO, INK, "bold"),
                       (" 以影子栈上的 ", FONT, INK, "bold"),
                       ("A", MONO, INK, "bold"),
                       (" 为准", FONT, INK, "bold")], 15)
    out.append(text(BUF_X, Y2 + CH + 26,
                    "调用时把返回地址多存一份到影子栈：软件实现由编译器插入的指令完成，"
                    "硬件实现由 call 自动完成", 14, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("shadow-stack", W, H, build(), left=12)    # centres the ink on the canvas
