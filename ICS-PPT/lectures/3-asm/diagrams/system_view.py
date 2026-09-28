#!/usr/bin/env python3
"""Disk, DRAM and CPU, with the four numbered flows of one inference run.

Disk and DRAM are drawn as address spaces, high addresses at the top, one
colour per segment: the ollama program is red and the model weights orange in
both, so the loads (1) and (3) join segments of the same colour. The disk is
its logical (LBA) space, file system metadata at the low end. DRAM is shown as
the ollama process's virtual address space on x86-64 Linux: the kernel in the
upper half, then the user stack, the mmap area holding the weights, the heap,
the data and the code at the low end, as in CS:APP figure 9.26. The two panels on the left magnify the first
8 bytes of each file on disk.

The bytes are real. `xxd -l 8 /usr/local/bin/ollama` gives 7f 45 4c 46 02 01
01 00: the ELF magic, then ELFCLASS64, ELFDATA2LSB (little-endian) and
EV_CURRENT. The GGUF bytes 47 47 55 46 03 00 00 00 ("GGUF", then version 3 as
a little-endian uint32) were read from the llama3.2 blob under
/usr/share/ollama/.ollama/models/blobs; every GGUF v3 file, llama3.gguf
included, starts with these 8 bytes.

The photos are embedded from ../assets/photo-*.jpg, scaled down from
Wikimedia Commons originals:
  photo-hdd.jpg   File:Laptop-hard-drive-exposed.jpg, Evan-Amos, CC BY-SA 3.0
  photo-ssd.jpg   File:Samsung 980 PRO PCIe 4.0 NVMe SSD 1TB-top PNr°0915.jpg,
                  D-Kuru, CC BY-SA 4.0
  photo-dram.jpg  File:16 GiB-DDR4-RAM-Riegel RAM019FIX Small Crop 90 PCNT.png,
                  PantheraLeo1359531, CC BY 4.0
  photo-cpu.jpg   File:Intel CPU Core i7 6700K Skylake top.jpg,
                  Eric Gaba (Sting), CC BY-SA 4.0
Run it to refresh ../assets/system-view.svg.
"""

import base64
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (ASSETS, BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, FILL_RED,
                    GREEN, INK, LINE, MUTED, ORANGE, RED, WHITE, arrow, elbow,
                    mono, rect, save, text)

W, H = 1120, 560
TOP, BOT = 124, 492                 # vertical extent of both address spaces
PURPLE, FILL_PURPLE = "#6b4f9a", "#efe9f6"
LENS = "#f4fbf6"

# top (high address) first: (label, height, fill, stroke, monospace label, dashed)
DISK = [("空闲空间", None, WHITE, LINE, False, True),
        ("llama3.gguf", 120, FILL_ORANGE, ORANGE, True, False),
        ("其他文件", 44, FILL_GREY, LINE, False, False),
        ("ollama", 64, FILL_RED, RED, True, False),
        ("文件系统元数据", 36, FILL_PURPLE, PURPLE, False, False)]
DX, DW = 268, 120

# top (high address) first: (label, sub-label, height, fill, stroke, dashed)
DRAM = [("OS Kernel", "常驻，统一管理分配", 66, FILL_BLUE, BLUE, False),
        ("用户栈", "", 28, FILL_GREY, LINE, False),
        ("未映射", "", 20, WHITE, LINE, True),
        ("模型数据缓冲区", "llama3.gguf 的权重", 110, FILL_ORANGE, ORANGE, False),
        ("未映射", "", 20, WHITE, LINE, True),
        ("堆", "", 28, FILL_GREY, LINE, False),
        ("数据段", "", 28, FILL_GREY, LINE, False),
        ("ollama 代码段", "由 OS 映射", 50, FILL_RED, RED, False),
        ("未映射", "", 18, WHITE, LINE, True)]
MX, MW = 492, 160

CX, CW = 756, 176

ELF = [0x7F, 0x45, 0x4C, 0x46, 0x02, 0x01, 0x01, 0x00]
GGUF = [0x47, 0x47, 0x55, 0x46, 0x03, 0x00, 0x00, 0x00]


def photo(name, x, y, w, h):
    data = base64.b64encode((ASSETS / name).read_bytes()).decode()
    return [f'<image x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" '
            f'preserveAspectRatio="xMidYMid meet" href="data:image/jpeg;base64,{data}"/>']


def spans(segs, hi):
    """Top and bottom of each segment; a height of None takes what is left."""
    rest = BOT - TOP - sum(seg[hi] for seg in segs if seg[hi] is not None)
    out, y = [], TOP
    for seg in segs:
        h = seg[hi] if seg[hi] is not None else rest
        out.append((y, y + h))
        y += h
    return out


def ends(x, w, space, medium):
    return [text(x + w, TOP - 5, "高地址", 13, MUTED, anchor="end"),
            text(x + w, BOT + 18, "低地址", 13, MUTED, anchor="end"),
            text(x + w / 2, 530, space, 13, MUTED),
            text(x + w / 2, 550, medium, 14, MUTED)]


def zoom(seg, panel, color, fill):
    """Dashed trapezoid from a disk segment's left edge to its panel."""
    (s0, s1), (p0, p1) = seg, panel
    px = 16 + 206
    return [f'<path d="M {DX} {s0} L {px} {p0} L {px} {p1} L {DX} {s1} Z" fill="{fill}" '
            f'fill-opacity="0.55" stroke="{color}" stroke-width="1.4" '
            f'stroke-dasharray="5 4"/>']


def lens(y, title, color, data, meaning):
    x, w, h = 16, 206, 172
    out = [rect(x, y, w, h, LENS, GREEN, rx=10, width=2),
           mono(x + w / 2, y + 22, title, 14, color, "bold", anchor="middle")]
    for i in range(4):
        pair = " ".join(f"{b:08b}" for b in data[2 * i:2 * i + 2])
        out.append(mono(x + w / 2, y + 48 + 22 * i, pair, 15, GREEN, "bold", anchor="middle"))
    out.append(mono(x + w / 2, y + 140, " ".join(f"{b:02x}" for b in data), 13, INK,
                    anchor="middle"))
    out.append(text(x + w / 2, y + 160, meaning, 13, MUTED))
    return out


def disk():
    out = photo("photo-hdd.jpg", 58, 0, 88, 66)
    out += photo("photo-ssd.jpg", 164, 8, 184, 51)
    out.append(text(102, 80, "机械硬盘", 12, MUTED))
    out.append(text(256, 80, "固态硬盘（NVMe）", 12, MUTED))
    out.append(text(202, 102, "外部存储（Disk）", 19, INK, "bold"))
    ys = spans(DISK, 1)
    out += zoom(ys[1], (124, 296), ORANGE, FILL_ORANGE)
    out += zoom(ys[3], (312, 484), RED, FILL_RED)
    for (label, _, fill, stroke, is_mono, dash), (y0, y1) in zip(DISK, ys):
        out.append(rect(DX, y0, DW, y1 - y0, fill, stroke, rx=2, width=1.6,
                        dash="5 4" if dash else None))
        cy = (y0 + y1) / 2 + 5
        if is_mono:
            out.append(mono(DX + DW / 2, cy + 1, label, 16, INK, "bold", anchor="middle"))
        else:
            out.append(text(DX + DW / 2, cy, label, 13 if y1 - y0 < 40 else 14,
                            MUTED if dash else INK))
    out += ends(DX, DW, "磁盘的逻辑地址空间", "非易失性介质")
    out += lens(124, "llama3.gguf 的前 8 字节", ORANGE, GGUF, "GGUF 魔数，格式版本 3")
    out += lens(312, "ollama 的前 8 字节", RED, ELF, "ELF 魔数，64 位，小端")
    out.append(text(119, 512, "程序与数据都是比特序列", 16, INK, "bold"))
    return out, ys


def dram():
    out = photo("photo-dram.jpg", MX + MW / 2 - 127, 8, 254, 60)
    out.append(text(MX + MW / 2, 102, "主存储器（DRAM）", 19, INK, "bold"))
    ys = spans(DRAM, 2)
    for (label, sub, _, fill, stroke, dash), (y0, y1) in zip(DRAM, ys):
        out.append(rect(MX, y0, MW, y1 - y0, fill, stroke, rx=2, width=1.6,
                        dash="5 4" if dash else None))
        cy = (y0 + y1) / 2
        if sub:
            out.append(text(MX + MW / 2, cy - 2, label, 16, INK, "bold"))
            out.append(text(MX + MW / 2, cy + 18, sub, 12, MUTED))
        else:
            out.append(text(MX + MW / 2, cy + 5, label, 12 if dash else 14,
                            MUTED if dash else INK))
    out += ends(MX, MW, "ollama 进程的虚拟地址空间", "易失性介质")
    return out, ys


def unit(y, h, label, fill, stroke, size=17):
    x, w = CX + 18, CW - 36
    return [rect(x, y, w, h, fill, stroke, rx=6, width=1.6),
            text(x + w / 2, y + h / 2 + 6, label, size, INK, "bold")]


def cpu():
    out = photo("photo-cpu.jpg", CX + CW / 2 - 36, 2, 72, 72)
    out.append(text(CX + CW / 2, 102, "中央处理器（CPU）", 19, INK, "bold"))
    out.append(rect(CX, TOP, CW, BOT - TOP, WHITE, INK, rx=10, width=1.8))
    out += unit(144, 56, "寄存器堆", FILL_BLUE, BLUE)
    out += unit(236, 66, "ALU", FILL_ORANGE, ORANGE, 19)
    out += unit(342, 50, "控制单元", FILL_GREY, MUTED)
    out += unit(422, 54, "PC（%rip）", FILL_BLUE, BLUE)
    return out


def terminal():
    x, y = 978, 231
    return [rect(x, y, 116, 76, INK, INK, rx=6),
            mono(x + 58, y + 45, "Token", 18, "#7ee2a0", "bold", anchor="middle"),
            rect(x + 48, y + 76, 20, 14, MUTED, MUTED, rx=1),
            rect(x + 28, y + 90, 60, 7, MUTED, MUTED, rx=2),
            text(x + 58, y + 124, "终端输出", 15, MUTED)]


def flows(dk, dm):
    mid = lambda s: (s[0] + s[1]) / 2
    prog, weights = mid(dk[3]), mid(dk[1])
    code, buf = mid(dm[7]), mid(dm[3])
    gap = (DX + DW + MX) / 2
    fx = (MX + MW + CX) / 2
    regs, alu, pc = 172, 269, 449          # centres of the CPU units
    out = []
    # 1 red: load the program into the code segment
    out.append(elbow([(DX + DW, prog), (gap, prog), (gap, code), (MX - 2, code)], RED, 3))
    out.append(text(gap, prog - 10, "① 装载程序", 16, RED, "bold"))
    # 3 orange: read the weights into the buffer
    out.append(elbow([(DX + DW, weights), (gap - 20, weights), (gap - 20, buf),
                      (MX - 2, buf)], ORANGE, 3))
    out.append(text(gap, weights - 10, "③ 读入权重", 16, ORANGE, "bold"))
    # 2 blue: fetch instructions
    out.append(elbow([(MX + MW, code), (fx, code), (fx, pc), (CX + 16, pc)], BLUE, 3))
    out.append(text(fx, pc - 10, "② 取指运行", 16, BLUE, "bold"))
    # 4 green: operands into registers, ALU, out to the terminal
    out.append(elbow([(MX + MW, buf), (fx, buf), (fx, regs), (CX + 16, regs)], GREEN, 3))
    out.append(text(fx, buf + 26, "④ 加载数据", 16, GREEN, "bold"))
    out.append(arrow(CX + CW / 2, regs + 30, CX + CW / 2, alu - 35, GREEN, 3))
    out.append(arrow(CX + CW - 18, alu, 976, alu, GREEN, 3))
    out.append(text(1036, 213, "④ 计算并输出", 16, GREEN, "bold"))
    return out


def build():
    dk_body, dk = disk()
    dm_body, dm = dram()
    return dk_body + dm_body + cpu() + terminal() + flows(dk, dm)


if __name__ == "__main__":
    save("system-view", W, H, build())
