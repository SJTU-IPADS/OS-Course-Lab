#!/usr/bin/env python3
"""y = a + b from the executable file, through memory, into the CPU.

Three panels recap the first part of the lecture: gcc puts add's instructions
and the three globals into the executable; once loaded, both are bytes at
addresses in memory; the CPU fetches at PC, loads a and b into %edx / %eax,
adds them in the ALU and stores y. The snapshot is taken while addl executes:
a and b are already in the registers, y not yet written.
Addresses, encodings and registers are those of examples/add.c built as the
page's demo builds it (gcc -O0 -fomit-frame-pointer -fcf-protection=none
-no-pie); y lives in .bss, after a one-byte libc flag at 0x404030, which is
why it is not at 0x404030. Run it to refresh ../assets/recap-part1.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK,
                    LINE, MUTED, ORANGE, WHITE, arrow, cells, elbow, mono, rect,
                    save, text)

W, H = 1120, 512
TOP, BOT = 44, 406                       # panel top and bottom
ROW, CH, BL = 28, 24, 18                 # row pitch, cell height, text baseline
CODE_Y, DATA_Y = 104, 306                # first code row top, first data row top

ASM = ["movl  a(%rip), %edx", "movl  b(%rip), %eax",
       "addl  %edx, %eax", "movl  %eax, y(%rip)", "nop", "ret"]
CODE = [("0x401106", "8b 15 1c 2f 00 00"), ("0x40110c", "8b 05 1a 2f 00 00"),
        ("0x401112", "01 d0"), ("0x401114", "89 05 1a 2f 00 00"),
        ("0x40111a", "90"), ("0x40111b", "c3")]
DATA = [("0x404028", "01 00 00 00", "a = 1"), ("0x40402c", "06 00 00 00", "b = 6"),
        ("0x404034", "00 00 00 00", "y = 0")]
PC_ROW = 2                               # addl is executing
QUIET = 4                                # rows from here on (nop, ret) are muted


def panel(x, w, title, fill, stroke):
    return [rect(x, TOP, w, BOT - TOP, fill, stroke, rx=8, width=1.6),
            text(x + w / 2, 69, title, 17, INK, "bold")]


def executable():
    x, w = 16, 300
    out = panel(x, w, "① 编译：可执行文件（ELF）", WHITE, BLUE)
    out.append(rect(x + 12, 86, w - 24, 194, FILL_BLUE, BLUE, rx=4, width=1))
    out.append(text(x + 24, 100, "代码段：add 函数的指令", 13, MUTED, anchor="start"))
    for i, s in enumerate(ASM):
        out.append(mono(x + 24, CODE_Y + i * ROW + BL, s, 16,
                        MUTED if i >= QUIET else INK))
    out.append(rect(x + 12, 288, w - 24, 108, FILL_GREEN, GREEN, rx=4, width=1))
    out.append(text(x + 24, 302, "数据段：全局变量 a、b、y", 13, MUTED, anchor="start"))
    for i, s in enumerate(["int a = 1", "int b = 6", "int y"]):
        out.append(mono(x + 24, DATA_Y + i * ROW + BL, s, 16))
    return out


def memory():
    x, w = 376, 360
    out = panel(x, w, "② 内存：每个字节一个地址", FILL_GREY, MUTED)
    out.append(text(x + 16, 96, "代码区：指令编码", 13, MUTED, anchor="start"))
    for i, (addr, byts) in enumerate(CODE):
        top = CODE_Y + i * ROW
        out.append(mono(x + 16, top + BL, addr, 15, MUTED))
        fill = FILL_ORANGE if i == PC_ROW else WHITE
        tone = MUTED if i >= QUIET else INK
        shapes, right = cells(x + 92, top, byts.split(), 26, CH, fill, LINE, 14,
                              tone=tone)
        out += shapes
        out.append(mono(right + 10, top + BL, ASM[i].split()[0], 14, MUTED))
    out.append(text(x + 16, 298, "数据区：整数编码（小端，低字节在前）", 13, MUTED,
                    anchor="start"))
    for i, (addr, byts, name) in enumerate(DATA):
        top = DATA_Y + i * ROW
        out.append(mono(x + 16, top + BL, addr, 15, MUTED))
        shapes, right = cells(x + 92, top, byts.split(), 26, CH, WHITE, LINE, 14)
        out += shapes
        out.append(mono(right + 10, top + BL, name, 15))
    out.append(text(x + 92 + 4 * 26 + 10 + 5 * 15 * 0.6 + 8, DATA_Y + 2 * ROW + BL,
                    "← ④ 写入 7", 14, ORANGE, "bold", anchor="start"))
    return out


def cpu():
    x, w = 836, 268
    out = panel(x, w, "③ CPU：寄存器与指令执行", FILL_BLUE, BLUE)
    rows = [("PC（%rip）", "0x401112"), ("%rax", "%eax = 6"), ("%rdx", "%edx = 1")]
    for i, (name, val) in enumerate(rows):
        top = 88 + i * 62
        out.append(rect(x + 12, top, w - 24, 48, WHITE, BLUE, rx=4, width=1.2))
        out.append(text(x + 24, top + 30, name, 15, INK, "bold", anchor="start"))
        out.append(mono(x + w - 24, top + 30, val, 15, anchor="end"))
    out.append(rect(x + 12, 292, w - 24, 88, FILL_ORANGE, ORANGE, rx=4, width=1.2))
    out.append(text(x + w / 2, 324, "ALU 执行 addl", 15, INK, "bold"))
    out.append(mono(x + w / 2, 354, "6 + 1 = 7 → %eax", 15, anchor="middle"))
    return out


def links():
    mid = (TOP + BOT) / 2
    out = [arrow(318, mid, 374, mid, MUTED, 2),
           text(346, mid - 10, "装入", 13, MUTED, "bold")]
    pc_y = CODE_Y + PC_ROW * ROW + CH / 2
    out.append(elbow([(836, 112), (770, 112), (770, pc_y), (738, pc_y)], BLUE, 2))
    out.append(text(776, 150, "取指", 13, BLUE, "bold", anchor="start"))
    ab_y = DATA_Y + ROW - 2               # between the a and b rows
    out.append(elbow([(736, ab_y), (800, ab_y), (800, 205), (834, 205)], BLUE, 2))
    out.append(text(746, ab_y - 8, "读 a、b", 13, BLUE, "bold", anchor="start"))
    y_y = DATA_Y + 2 * ROW + CH / 2
    out.append(arrow(836, y_y, 738, y_y, ORANGE, 2))
    out.append(text(786, y_y + 17, "写 y", 13, ORANGE, "bold"))
    return out


def footer():
    lines = ["执行过程：① PC = 0x401106，取指 movl，%edx ← M[0x404028] = 1，PC ← PC + 6；"
             "② movl，%eax ← M[0x40402c] = 6，PC ← PC + 6；",
             "③ addl，%eax ← 6 + 1 = 7，PC ← PC + 2；④ movl，M[0x404034] ← %eax = 7，"
             "PC ← PC + 6。"]
    out = [text(16, 436 + i * 26, s, 16, INK, anchor="start") for i, s in enumerate(lines)]
    out.append(text(16, 490, "指令与数据都是内存中的字节，区别只在 CPU 如何解释。", 16, BLUE,
                    "bold", anchor="start"))
    return out


def build():
    return executable() + memory() + cpu() + links() + footer()


if __name__ == "__main__":
    save("recap-part1", W, H, build())
