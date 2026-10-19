#!/usr/bin/env python3
"""guest/hello after loading: where its memory and its registers are.

Left: the ELF file. Inside the mini-cpu process: the three parts of guest
memory at the addresses guest/hello uses, each with the dram.img offset its
bytes are at; below, the registers as the array R[] of the variable cpu in
main, with the values the loader sets; interp_run reads and writes both.
The offsets and values were read on 2026-10-19 from dram.img with od and
from ./mini-cpu -v, after running guest/hello. Segment and register colours
are those of cpu_state; interp_run is the student's (orange).
Run it to refresh ../assets/cpu_hello_mem.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, FONT, INK, LINE, MONO, MUTED,
                    ORANGE, WHITE, arrow, cells, elbow, rect, save, text)
from cpu_kit import code, note

W, H = 1120, 406

MX, MW, SH = 300, 340, 46              # guest memory: x, width, one part's height
TOP = 94                               # the first part of guest memory
DX, DW = 744, 336                      # dram.img
SEGS = [("0x401000", "代码 .text", "48 8b 3c 24 48 8d 74 24 08 ...", True, FILL_ORANGE, ORANGE, "0x101000"),
        ("0x402000", "只读数据 .rodata", '"hello from the guest\\n"  "argc = "', True, FILL_GREY, MUTED, "0x102000"),
        ("0x7fffffffefc0", "栈", "argc = 1，argv[0] 指向 \"guest/hello\"", False, FILL_BLUE, BLUE, "0x142fc0")]
GPR = ["rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
       "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"]


def build():
    out = []
    # the file
    out.append(code(16, 40, "./mini-cpu guest/hello", 14, INK, "bold"))
    out.append(rect(16, TOP, 200, 162, FILL_GREY, MUTED, rx=6, width=1.6))
    out.append(code(116, TOP + 26, "guest/hello", 15, INK, "bold", anchor="middle"))
    out.append(note(116, TOP + 44, "x86-64 的 ELF 文件", 12, anchor="middle"))
    for k, (name, addr, what) in enumerate([(".text", "0x401000", "指令，0x16a 字节"),
                                            (".rodata", "0x402000", "字符串，0x1e 字节"),
                                            ("入口", "0x401000", "第一条指令的地址")]):
        y = TOP + 68 + k * 34
        out.append(text(28, y, name, 12.5, INK, anchor="start", font=MONO if name.isascii() else FONT))
        out.append(code(100, y, addr, 12.5))
        out.append(note(28, y + 15, what, 11))
    out.append(arrow(216, TOP + 78, MX - 6, TOP + 78, INK, 2))
    out.append(text(240, TOP + 70, "装入", 12, INK, "bold"))

    # the process
    out.append(rect(262, 14, 842, H - 24, WHITE, INK, rx=10, width=1.6))
    out.append(text(282, 40, "mini-cpu 进程：本机上的一个普通程序", 15, INK, "bold", anchor="start"))

    # guest memory, and where its bytes are in dram.img
    out.append(text(MX, 64, "客户机内存：guest/hello 使用的地址", 13.5, INK, "bold", anchor="start"))
    out.append(note(MX, 82, "guest/hello 的指令中写的是这些地址", 11.5))
    out.append(text(DX, 64, "dram.img：当前目录下 64 MiB 的文件", 13.5, INK, "bold", anchor="start"))
    out.append(note(DX, 82, "mini-cpu 用 mmap 把它映射到自己的内存中", 11.5))
    out.append(rect(DX, TOP, DW, 160, WHITE, INK, rx=4, width=1.4))
    for k, (addr, name, body, mono_body, fill, stroke, off) in enumerate(SEGS):
        y = TOP + k * (SH + 11)
        out.append(rect(MX, y, MW, SH, fill, stroke, rx=4, width=1.4))
        out.append(code(MX + 10, y + 19, addr, 13, INK, "bold"))
        out.append(text(MX + MW - 10, y + 19, name, 12.5, INK, anchor="end"))
        out.append(text(MX + 10, y + 37, body, 12, INK, anchor="start", font=MONO if mono_body else FONT))
        # the same bytes in the file
        out.append(arrow(MX + MW + 4, y + SH / 2, DX + 16, y + SH / 2, INK, 1.6))
        out.append(rect(DX + 20, y + 10, 70, SH - 20, fill, stroke, rx=2, width=1.2))
        out.append(note(DX + 104, y + SH / 2 + 5, "偏移", 12.5))
        out.append(code(DX + 136, y + SH / 2 + 5, off, 13, INK, "bold"))
    out.append(note((MX + MW + DX) / 2 + 6, TOP + SH + 11 + SH / 2 - 8, "按页对应", 11.5, anchor="middle"))

    # interp_run, which uses both
    ix, iy, iw, ih = 920, 298, 164, 80
    out.append(rect(ix, iy, iw, ih, FILL_ORANGE, ORANGE, rx=6, width=1.6))
    out.append(code(ix + iw / 2, iy + 26, "interp_run", 15, INK, "bold", anchor="middle"))
    out.append(text(ix + iw / 2, iy + 48, "按 R[RIP] 取指令", 12, INK))
    out.append(text(ix + iw / 2, iy + 66, "写 R[] 与客户机内存", 12, INK))
    out.append(elbow([(ix + 30, iy - 2), (ix + 30, 280), (MX + MW - 30, 280), (MX + MW - 30, TOP + 3 * SH + 22 + 4)],
                     ORANGE, 1.6))
    out.append(note(MX + MW - 20, 274, "按客户机地址取指令、读写内存", 11.5))

    # the registers
    ry, cw = 316, 30
    out.append(text(MX, 306, "寄存器：main 中的变量 mcpu cpu 的数组 R[18]", 13.5, INK, "bold", anchor="start"))
    shapes, x = cells(MX, ry, GPR, cw, 30, FILL_BLUE, BLUE, 11)
    out += shapes
    shapes, x = cells(x + 6, ry, ["flags"], 44, 30, FILL_ORANGE, ORANGE, 11)
    out += shapes
    fx = x - 22
    shapes, x = cells(x + 6, ry, ["rip"], 44, 30, FILL_GREY, MUTED, 11)
    out += shapes
    out.append(code(MX + 4 * cw + cw / 2, ry + 48, "0x7fffffffefc0", 11.5, BLUE, "bold", anchor="middle"))
    out.append(code(fx, ry + 48, "0x2", 11.5, ORANGE, "bold", anchor="middle"))
    out.append(code(x - 22, ry + 48, "0x401000", 11.5, INK, "bold", anchor="middle"))
    out.append(note(MX, ry + 70, "装入之后：R[RSP] 指向栈顶的 argc，R[RIP] 是入口，其余为 0", 12))
    out.append(arrow(ix - 4, iy + ih / 2 - 13, x + 6, ry + 15, ORANGE, 1.6, both=True))
    return out


if __name__ == "__main__":
    save("cpu_hello_mem", W, H, build())
