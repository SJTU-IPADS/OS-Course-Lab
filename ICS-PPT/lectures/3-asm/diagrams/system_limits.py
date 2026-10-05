#!/usr/bin/env python3
"""Disk, DRAM and CPU of system_view.py, with the limits of one inference run.

The flows are those of system_view.py. That figure draws step 4 as two green
arrows, operands in and the result out; here the load of the weights is red,
so the output to the terminal has its own number, 5. The CPU is three parts:
a 256-bit register (bit 0 on the right), the arithmetic unit of each of the
eight cores as a bar of the same width, and the execution of instructions.
The page comes before the SIMD part of the lecture, so the figure uses only
what has been taught by then: registers, arithmetic, clock cycles. It names
no lanes, no decoding and no issue width; the page insn-mix counts the cycles.

One colour per limit:
  red     memory bandwidth, 51.2 GB/s, on the path that carries the weights
          from DRAM to the CPU; a token reads 0.968 GB, so 53 tokens a second
  blue    peak compute, 320 GFLOPS: the arithmetic units of all cores at work
  green   what ollama uses on one thread: one core, the whole register; its
          Q4_0 inner product (ggml_vec_dot_q4_0_q8_0, icelake build) runs
          22 instructions for 32 multiply-adds
  orange  what mini-ollama uses: one core, the low 32 bits of the register;
          matvec() runs 20 instructions for 2 multiply-adds

The photos are embedded from ../assets/photo-*.jpg, scaled down from
Wikimedia Commons originals:
  photo-ssd.jpg   File:Samsung 980 PRO PCIe 4.0 NVMe SSD 1TB-top PNr°0915.jpg,
                  D-Kuru, CC BY-SA 4.0
  photo-dram.jpg  File:16 GiB-DDR4-RAM-Riegel RAM019FIX Small Crop 90 PCNT.png,
                  PantheraLeo1359531, CC BY 4.0
  photo-cpu.jpg   File:Intel CPU Core i7 6700K Skylake top.jpg,
                  Eric Gaba (Sting), CC BY-SA 4.0
Run it to refresh ../assets/system-limits.svg.
"""

import base64
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (ASSETS, BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE,
                    FILL_RED, GREEN, INK, LINE, MUTED, ORANGE, RED, WHITE, arrow, mono,
                    rect, save, text)

W, H = 1120, 560
TOP, BOT = 124, 492                 # vertical extent of the three columns

DX, DW = 24, 110                    # disk
MX, MW = 226, 150                   # DRAM
CX, CW = 566, 440                   # CPU
TX, TW = 1024, 84                   # terminal

DATA_Y, CODE_Y = (140, 230), (414, 474)     # the weights and the program, on disk and in DRAM
GX, BAR, LOW = CX + 66, 320, 40     # a 256-bit bar and its low 32 bits
REG_Y = 162                         # the register
GRID_Y, GH = 244, 16                # the cores, one row each
EXEC_Y = 406                        # the execution of instructions

PEAK, BW, GB = 320, 51.2, 0.968


def photo(name, x, y, w, h):
    data = base64.b64encode((ASSETS / name).read_bytes()).decode()
    return [f'<image x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" '
            f'preserveAspectRatio="xMidYMid meet" href="data:image/jpeg;base64,{data}"/>']


def mid(span):
    return (span[0] + span[1]) / 2


def box(x, w, span, label, sub=None, is_mono=False):
    y0, y1 = span
    out = [rect(x, y0, w, y1 - y0, FILL_GREY, LINE, rx=3, width=1.6)]
    cx, cy = x + w / 2, mid(span)
    if is_mono:
        lines = label if isinstance(label, list) else [label]
        for k, s in enumerate(lines):
            out.append(mono(cx, cy + 5 + 20 * (k - (len(lines) - 1) / 2), s, 13, INK, "bold",
                            anchor="middle"))
    elif sub:
        out.append(text(cx, cy - 2, label, 16, INK, "bold"))
        out.append(text(cx, cy + 18, sub, 12, MUTED))
    else:
        out.append(text(cx, cy + 5, label, 14, INK))
    return out


def disk():
    cx = DX + DW / 2
    out = photo("photo-ssd.jpg", cx - 66, 22, 132, 37)
    out.append(text(cx, 102, "外部存储（Disk）", 17, INK, "bold"))
    out.append(rect(DX, TOP, DW, BOT - TOP, WHITE, LINE, rx=6, width=1.6))
    x, w = DX + 8, DW - 16
    out += box(x, w, (155, 215), "q4_0.gguf", is_mono=True)
    out += box(x, w, (290, 336), "其他文件")
    out += box(x, w, CODE_Y, ["ollama", "mini-ollama"], is_mono=True)
    out.append(text(cx, 516, "非易失性介质", 14, MUTED))
    return out


def dram():
    cx = MX + MW / 2
    out = photo("photo-dram.jpg", cx - 106, 14, 212, 50)
    out.append(text(cx, 102, "主存储器（DRAM）", 17, INK, "bold"))
    out.append(rect(MX, TOP, MW, BOT - TOP, WHITE, LINE, rx=6, width=1.6))
    x, w = MX + 10, MW - 20
    out += box(x, w, DATA_Y, "模型数据缓冲区", f"权重 {GB:g} GB")
    out += box(x, w, CODE_Y, "代码段", "ollama 或 mini-ollama")
    out.append(text(cx, 516, "易失性介质", 14, MUTED))
    return out


def bar(y, h, used=True):
    """256 bits: green where ollama works, orange for the 32 bits mini-ollama uses."""
    fill, stroke = (FILL_GREEN, GREEN) if used else (WHITE, LINE)
    out = [rect(GX, y, BAR, h, fill, stroke, rx=2, width=1.2)]
    if used:
        out.append(rect(GX + BAR - LOW, y, LOW, h, FILL_ORANGE, ORANGE, rx=2, width=2.4))
    return out


def registers():
    x0, x1 = CX + 12, CX + CW - 12
    out = [rect(x0, TOP + 8, x1 - x0, 82, WHITE, LINE, rx=5, width=1.4),
           text(x0 + 12, REG_Y - 9, "寄存器：256 位", 15, INK, "bold", anchor="start")]
    out += bar(REG_Y, 28)
    out.append(text(GX + BAR - LOW / 2, REG_Y + 18, "32 位", 12, INK))
    out.append(text(GX, REG_Y + 45, "ollama 使用全部 256 位", 13, GREEN, "bold",
                    anchor="start"))
    out.append(text(GX + BAR, REG_Y + 45, "mini-ollama 使用低 32 位", 13, ORANGE, "bold",
                    anchor="end"))
    return out


def arithmetic():
    x0, x1 = CX + 12, CX + CW - 12
    bottom = GRID_Y + 8 * GH
    out = [rect(x0, GRID_Y - 30, x1 - x0, bottom - GRID_Y + 60, WHITE, LINE, rx=5,
                width=1.4),
           text(x0 + 12, GRID_Y - 10, "运算单元（乘法、加法）：8 个核心各有 1 组", 15,
                INK, "bold", anchor="start")]
    for core in range(8):
        y = GRID_Y + core * GH
        out.append(text(GX - 8, y + GH / 2 + 4, f"核心 {core}", 12, MUTED, anchor="end"))
        out += bar(y + 1, GH - 2, used=core == 0)
    out.append(rect(GX - 4, GRID_Y - 4, BAR + 8, 8 * GH + 8, "none", BLUE, rx=4, width=2.6))
    out.append(text((x0 + x1) / 2, bottom + 22,
                    f"峰值算力 P = {PEAK} GFLOPS：8 个核心的运算单元全部同时运算", 14, BLUE,
                    "bold"))
    return out


def execution():
    x0, x1 = CX + 12, CX + CW - 12
    return [rect(x0, EXEC_Y, x1 - x0, BOT - 8 - EXEC_Y, WHITE, LINE, rx=5, width=1.4),
            text(x0 + 12, EXEC_Y + 22, "指令执行：每个时钟周期能开始执行的指令条数有上限", 15, INK,
                 "bold", anchor="start"),
            text(x0 + 12, EXEC_Y + 44, "ollama：22 条指令完成 32 次乘加，每次乘加 0.69 条", 14,
                 GREEN, "bold", anchor="start"),
            text(x0 + 12, EXEC_Y + 65, "mini-ollama：20 条指令完成 2 次乘加，每次乘加 10 条", 14,
                 ORANGE, "bold", anchor="start")]


def cpu():
    cx = CX + CW / 2
    out = photo("photo-cpu.jpg", cx - 36, 2, 72, 72)
    out.append(text(cx, 102, "中央处理器（CPU）", 17, INK, "bold"))
    out.append(rect(CX, TOP, CW, BOT - TOP, FILL_BLUE, BLUE, rx=10, width=1.8))
    return out + registers() + arithmetic() + execution()


def terminal():
    y = GRID_Y + 4 * GH - 34
    cx = TX + TW / 2
    return [rect(TX, y, TW, 60, INK, INK, rx=6),
            mono(cx, y + 36, "Token", 15, "#7ee2a0", "bold", anchor="middle"),
            rect(cx - 9, y + 60, 18, 12, MUTED, MUTED, rx=1),
            rect(cx - 26, y + 72, 52, 6, MUTED, MUTED, rx=2),
            text(cx, y - 12, "⑤ 计算并输出", 14, INK, "bold"),
            text(cx, y + 100, "终端输出", 14, MUTED),
            arrow(CX + CW, y + 30, TX - 2, y + 30, INK, 2.4)]


def flows():
    left, right = (DX + DW + MX) / 2, (MX + MW + CX) / 2
    data, code = mid(DATA_Y), mid(CODE_Y)
    out = [
        arrow(DX + DW - 8, code, MX + 8, code, INK, 2.4),
        text(left, code - 12, "① 装载程序", 15, INK, "bold"),
        arrow(DX + DW - 8, data, MX + 8, data, INK, 2.4),
        text(left, data - 12, "③ 读入权重", 15, INK, "bold"),
        arrow(MX + MW - 10, code, CX + 10, code, INK, 2.4),
        text(right, code - 12, "② 取指运行", 15, INK, "bold"),
        # the path the memory bandwidth limits
        arrow(MX + MW - 10, data, CX + 10, data, RED, 3.6),
        text(right, data - 14, "④ 加载数据", 15, RED, "bold"),
    ]
    x, w = MX + MW + 6, CX - MX - MW - 12
    out.append(rect(x, data + 16, w, 84, FILL_RED, RED, rx=5, width=1.6))
    for k, (s, size, weight) in enumerate([
            (f"内存带宽 B = {BW:g} GB/s", 14, "bold"),
            ("限制读取权重的通路", 13, "normal"),
            (f"每个 Token 读取 {GB:g} GB", 13, "normal"),
            (f"至多 {BW / GB:.0f} Token/s", 13, "bold")]):
        out.append(text(right, data + 36 + 19 * k, s, size, RED if k in (0, 3) else INK,
                        weight))
    return out


def legend():
    out = []
    for k, (fill, stroke, s) in enumerate([
            (FILL_GREEN, GREEN, "ollama（1 个线程）使用 1 个核心，一条指令处理 32 个权重"),
            (FILL_ORANGE, ORANGE, "mini-ollama 使用 1 个核心，一条指令处理 1 个权重")]):
        y = 508 + 25 * k
        out.append(rect(CX + 12, y, 18, 14, fill, stroke, rx=2, width=1.6))
        out.append(text(CX + 38, y + 12, s, 14, INK, anchor="start"))
    return out


def build():
    return disk() + dram() + cpu() + terminal() + flows() + legend()


if __name__ == "__main__":
    save("system-limits", W, H, build())
