#!/usr/bin/env python3
"""The machine state the framework keeps: R[18], Y[16] and guest memory.

Run it to refresh ../assets/cpu_state.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK,
                    LINE, MONO, MUTED, ORANGE, WHITE, brace, cells, rect, save, text)
from cpu_kit import code, note

W, H = 1120, 340

GPR = ["rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
       "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"]


def build():
    out = []
    # R[0..17]
    out.append(text(20, 30, "R[18]", 17, INK, "bold", anchor="start", font=MONO))
    out.append(note(90, 30, "通用寄存器的下标与机器码中的编号相同；interp.c 直接读写：R[RSP] -= 8"))
    cw = 52
    shapes, x = cells(20, 42, GPR, cw, 32, FILL_BLUE, BLUE, 14)
    out += shapes
    for k in range(16):
        out.append(note(20 + k * cw + cw / 2, 90, str(k), 12, anchor="middle"))
    fx = x + 10
    shapes, x = cells(fx, 42, ["flags"], 96, 32, FILL_ORANGE, ORANGE, 14)
    out += shapes
    out.append(note(fx + 48, 90, "16", 12, anchor="middle"))
    shapes, x2 = cells(x + 10, 42, ["rip"], 80, 32, FILL_GREY, MUTED, 14)
    out += shapes
    out.append(note(x + 50, 90, "17", 12, anchor="middle"))
    # the five flag bits
    out.append(text(x2, 128, "flags 只用五位：CF 0 · PF 2 · ZF 6 · SF 7 · OF 11", 12, ORANGE, "bold",
                    anchor="end"))
    out.append(note(x2, 146, "括号中是位号；其余位保持不变", 12, anchor="end"))

    # Y[0..15]
    out.append(text(20, 176, "Y[16]", 17, INK, "bold", anchor="start", font=MONO))
    out.append(note(90, 176, "ymm0 .. ymm15，每个 256 位；u256 的 d[i] 是第 i 个 32 位通道，d[0..3] 组成 xmm"))
    shapes, x = cells(20, 188, [f"ymm{k}" for k in range(16)], cw, 32, FILL_GREEN, GREEN, 13)
    out += shapes
    # one register expanded into lanes
    lx, ly, lw = 20, 240, 60
    out.append(code(lx, ly + 22, "Y[n]", 15, INK, "bold"))
    lanes = [f"d[{i}]" for i in range(7, -1, -1)]
    fills = [WHITE] * 4 + [FILL_GREEN] * 4
    shapes, lr = cells(lx + 60, ly, lanes, lw, 32, fills, GREEN, 14)
    out += shapes
    out += brace(lx + 60 + 4 * lw + 2, lr - 2, ly + 34, GREEN, "xmm n：低 128 位", 13, 6)
    out += brace(lx + 60 + 2, lx + 60 + 4 * lw - 2, ly + 34, MUTED, "高 128 位；写 xmm 时清零", 13, 6)
    out.append(note(lr + 14, ly + 21, "← 8 个通道，d[0] 在最右", 13))

    # guest memory
    mx, my, mw = 800, 250, 300
    out.append(text(mx, my - 10, "客户机内存", 15, INK, "bold", anchor="start"))
    out.append(note(mx + 90, my - 10, "load、store 按虚拟地址读写", 13))
    segs = [("代码", 0.24, FILL_ORANGE, ORANGE), ("数据", 0.2, FILL_GREY, MUTED),
            ("堆", 0.18, FILL_GREY, MUTED), ("", 0.2, WHITE, LINE), ("栈", 0.18, FILL_BLUE, BLUE)]
    x = mx
    for name, frac, fill, stroke in segs:
        w = mw * frac
        out.append(rect(x, my, w, 32, fill, stroke, rx=2, width=1.2))
        if name:
            out.append(text(x + w / 2, my + 21, name, 14, INK))
        x += w
    out.append(code(mx, my + 52, "0x401000", 12, MUTED))
    out.append(code(mx + mw, my + 52, "0x7fffffffefc0", 12, MUTED, anchor="end"))
    out.append(note(mx, my + 74, "物理内存：当前目录下的文件 dram.img", 13))
    return out


if __name__ == "__main__":
    save("cpu_state", W, H, build())
