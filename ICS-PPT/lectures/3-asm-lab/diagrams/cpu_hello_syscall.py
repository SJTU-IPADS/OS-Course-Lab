#!/usr/bin/env python3
"""What happens at the syscall instructions of guest/hello.

Top row: the first write, at 40112a. The registers the guest has set, the
path through interp_run (default: x86_other, syscall_enter) into
sys_dispatch of frame/sysbase.c, which copies the string out of guest
memory and calls write on the host. Bottom row: exit, at 401019, which
sets halted and ends the loop. Addresses from objdump -d guest/hello; the
student's code orange, the framework's grey, what comes out green.
Run it to refresh ../assets/cpu_hello_syscall.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, GREEN, INK, LINE, MUTED, WHITE, arrow, rect, save, text)
from cpu_kit import GIVEN, OPCODE, TODO, byte_row, code, note

W, H = 1120, 284

COLS = [(16, 236), (292, 236), (568, 316), (924, 180)]      # x, width


def column(k, y, h, colour, title, lines, mono_title=True):
    x, w = COLS[k]
    fill, stroke = colour
    out = [rect(x, y, w, h, fill, stroke, rx=6, width=1.6)]
    if mono_title:
        out.append(code(x + 12, y + 22, title, 14, INK, "bold"))
    else:
        out.append(text(x + 12, y + 22, title, 14, INK, "bold", anchor="start"))
    for i, s in enumerate(lines):
        if isinstance(s, tuple):            # (mono part, note)
            out.append(code(x + 12, y + 44 + i * 20, s[0], 12.5))
            out.append(note(x + w - 12, y + 44 + i * 20, s[1], 12, anchor="end"))
        else:
            out.append(text(x + 12, y + 44 + i * 20, s, 12.5, INK, anchor="start"))
    return out


def arrows(y):
    out = []
    for (x0, w0), (x1, _) in zip(COLS, COLS[1:]):
        out.append(arrow(x0 + w0 + 4, y, x1 - 4, y, INK, 1.8))
    return out


def build():
    out = []
    # the first write
    out.append(text(16, 20, '第一次 write：puts("hello from the guest\\n")', 14, INK, "bold", anchor="start"))
    y, h = 32, 146
    out.append(rect(COLS[0][0], y, COLS[0][1], h, WHITE, LINE, rx=6, width=1.6))
    out.append(code(28, y + 23, "40112a", 14, INK, "bold"))
    shapes, x = byte_row(92, y + 6, [(["0f", "05"], OPCODE)], cw=30, ch=24, size=13)
    out += shapes
    out.append(code(x + 10, y + 23, "syscall", 14, INK, "bold"))
    for i, (reg, what) in enumerate([("R[RAX] = 1", "write"), ("R[RDI] = 1", "stdout"),
                                     ("R[RSI] = 0x402000", "字符串"), ("R[RDX] = 21", "字节数")]):
        out.append(code(28, y + 54 + i * 22, reg, 12.5))
        out.append(note(COLS[0][0] + COLS[0][1] - 12, y + 54 + i * 22, what, 12, anchor="end"))
    out += column(1, y, h, TODO, "interp_run",
                  ["x86_begin：op = 0x105", "switch 中没有 case 0x105，", "进入 default：x86_other(&x)",
                   "x86_other 调用 syscall_enter，", "再进入 sys_dispatch"])
    out += column(2, y, h, GIVEN, "sys_dispatch（frame/sysbase.c）",
                  ["R[RAX] 是 1：write", "mem_read 从客户机地址 0x402000",
                   "复制 21 个字节到 mini-cpu 的 buf", "调用本机的 write(1, buf, 21)",
                   "R[RAX] = 21：写出的字节数"], mono_title=False)
    out.append(rect(COLS[3][0], y, COLS[3][1], h, FILL_GREEN, GREEN, rx=6, width=1.6))
    out.append(text(COLS[3][0] + 12, y + 22, "终端", 14, INK, "bold", anchor="start"))
    out.append(code(COLS[3][0] + 12, y + 56, "hello from the guest", 12.5, INK, "bold"))
    out += arrows(y + h / 2)

    # exit
    out.append(text(16, 206, "exit：guest_main 返回之后，_start 的最后一个 syscall", 14, INK, "bold", anchor="start"))
    y, h = 218, 62
    out.append(rect(COLS[0][0], y, COLS[0][1], h, WHITE, LINE, rx=6, width=1.6))
    out.append(code(28, y + 23, "401019  syscall", 14, INK, "bold"))
    out.append(code(28, y + 44, "R[RAX] = 60", 12.5))
    out.append(note(COLS[0][0] + COLS[0][1] - 12, y + 44, "exit，R[RDI] = 0", 12, anchor="end"))
    out += column(1, y, h, TODO, "interp_run", ["同样经 x86_other 进入 sys_dispatch"])
    out += column(2, y, h, GIVEN, "sys_dispatch", ["R[RAX] 是 60：exit，置 c->halted = 1"], mono_title=False)
    out += column(3, y, h, (FILL_GREEN, GREEN), "循环结束", ["mini-cpu 的退出码 0"], mono_title=False)
    out += arrows(y + h / 2)
    return out


if __name__ == "__main__":
    save("cpu_hello_syscall", W, H, build())
