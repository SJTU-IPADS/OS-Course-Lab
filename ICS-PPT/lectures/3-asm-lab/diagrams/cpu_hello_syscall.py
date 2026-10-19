#!/usr/bin/env python3
"""The syscall of guest/hello is carried out by a syscall in mini-cpu.

Two listings of machine code. Left: guest/hello, which mini-cpu reads;
its syscall at 40112a is two bytes 0f 05. Middle: mini-cpu, which the PC's
CPU executes: its own code calls libc's write, and write executes a syscall
with the same two bytes, which enters Linux on the right. The two syscall
rows are on one line. Guest bytes from objdump -d guest/hello; call write
from objdump -d mini-cpu (write@plt); the syscall of write from objdump -d
of the host libc.so.6 (glibc 2.43), where write reaches it through an
internal function, hence the rows "...". Syscall rows orange, as the
opcode; Linux blue, as on the page before.
Run it to refresh ../assets/cpu_hello_syscall.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE, arrow,
                    elbow, rect, save, text)
from cpu_kit import code, note

W, H = 1120, 318

TOP, BOT = 32, 310                     # the three boxes
LX, LW = 16, 404                       # guest/hello
MX, MW = 478, 400                      # mini-cpu
KX, KW = 944, 160                      # Linux
ROW = 32
SYS = 254                              # the line of both syscall rows

GUEST = [("", "...", ""),
         ("40111d", "b8 01 00 00 00", "mov    $0x1,%eax"),
         ("401122", "be 00 20 40 00", "mov    $0x402000,%esi"),
         ("401127", "48 89 c7", "mov    %rax,%rdi"),
         ("40112a", "0f 05", "syscall"),
         ("", "...", "")]


def highlight(x, y, w):
    return rect(x, y - 19, w, 28, FILL_ORANGE, ORANGE, rx=4, width=1.6)


def build():
    out = []

    # guest/hello: machine code that mini-cpu reads
    out.append(rect(LX, TOP, LW, BOT - TOP, WHITE, LINE, rx=8, width=1.6))
    out.append(text(LX + 16, TOP + 26, "guest/hello 的机器码", 15, INK, "bold", anchor="start"))
    out.append(note(LX + 16, TOP + 46, "由 mini-cpu 读取", 12.5))
    top = SYS - 4 * ROW
    for k, (addr, raw, insn) in enumerate(GUEST):
        y = top + k * ROW
        bold = "bold" if insn == "syscall" else "normal"
        if insn == "syscall":
            out.append(highlight(LX + 8, y, LW - 16))
        out.append(code(LX + 16, y, addr, 14, MUTED))
        out.append(code(LX + 76, y, raw, 14, INK, bold))
        out.append(code(LX + 218, y, insn, 14, INK, bold))

    # mini-cpu: machine code that the PC's CPU executes
    out.append(rect(MX, TOP, MW, BOT - TOP, WHITE, INK, rx=8, width=1.6))
    out.append(text(MX + 16, TOP + 26, "mini-cpu 的机器码", 15, INK, "bold", anchor="start"))
    out.append(note(MX + 16, TOP + 46, "由 PC 的 CPU 执行", 12.5))
    sx, sw = MX + 12, MW - 24
    bx, ax = MX + 104, MX + 240            # bytes, instruction
    ys = [SYS - 4 * ROW - 10, SYS - 3 * ROW - 10, SYS - 2 * ROW - 10]   # mini-cpu's own code
    yl = [SYS - ROW, SYS, SYS + ROW]                                   # libc's write
    for (rows, label) in ((ys, ["mini-cpu", "自己的代码"]), (yl, ["libc 的", "write"])):
        y0 = rows[0] - 20
        out.append(rect(sx, y0, sw, rows[-1] + 10 - y0, WHITE, LINE, rx=5, width=1.2))
        mid = (rows[0] + rows[-1]) / 2
        out.append(text(sx + 12, mid - 4, label[0], 13, INK, "bold", anchor="start"))
        out.append(text(sx + 12, mid + 14, label[1], 13, INK, "bold", anchor="start"))
    out.append(code(bx, ys[0], "...", 14))
    out.append(text(ax, ys[0], "读取客户机的指令", 13, MUTED, anchor="start"))
    out.append(code(bx, ys[1], "e8 d2 85 ff ff", 14, INK, "bold"))
    out.append(code(ax, ys[1], "call   write", 14, INK, "bold"))
    out.append(code(bx, ys[2], "...", 14))
    out.append(code(bx, yl[0], "...", 14))
    out.append(highlight(bx - 8, SYS, sx + sw - bx))
    out.append(code(bx, SYS, "0f 05", 14, INK, "bold"))
    out.append(code(ax, SYS, "syscall", 14, INK, "bold"))
    out.append(text(sx + sw - 10, SYS, "代为执行", 14, ORANGE, "bold", anchor="end"))
    out.append(code(bx, yl[2], "...", 14))

    # read: the guest's syscall is data for mini-cpu
    vx = (LX + LW + MX) / 2
    out.append(elbow([(LX + LW - 8, SYS - 5), (vx, SYS - 5), (vx, ys[0] - 5), (sx - 3, ys[0] - 5)],
                     MUTED, 1.6, dash="5 4"))
    out.append(rect(vx - 18, (SYS + ys[0]) / 2 - 22, 36, 22, WHITE, WHITE, rx=0, width=0))
    out.append(text(vx, (SYS + ys[0]) / 2 - 6, "读取", 13, MUTED, "bold"))

    # call: from mini-cpu's code into libc's write
    cx = sx + sw - 24
    out.append(elbow([(ax + 108, ys[1] - 5), (cx, ys[1] - 5), (cx, yl[0] - 22)], INK, 1.6))
    out.append(text(cx - 8, ys[2] + 2, "call", 13, INK, "bold", anchor="end"))

    # the syscall of write enters Linux
    out.append(arrow(MX + MW + 4, SYS - 5, KX - 4, SYS - 5, ORANGE, 2.6))
    out.append(code((MX + MW + KX) / 2, SYS - 14, "syscall", 12.5, ORANGE, "bold", anchor="middle"))
    out.append(rect(KX, TOP, KW, BOT - TOP, FILL_BLUE, BLUE, rx=8, width=1.6))
    mid = (TOP + BOT) / 2                  # three lines centred in the box
    out.append(text(KX + KW / 2, mid - 18, "操作系统 Linux", 15, INK, "bold"))
    out.append(text(KX + KW / 2, mid + 9, "把 21 个字节", 13.5, INK))
    out.append(text(KX + KW / 2, mid + 29, "输出到终端", 13.5, INK))
    return out


if __name__ == "__main__":
    save("cpu_hello_syscall", W, H, build())
