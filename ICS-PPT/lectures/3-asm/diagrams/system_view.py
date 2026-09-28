#!/usr/bin/env python3
"""Disk, DRAM and CPU, with the four numbered flows of one inference run.

One colour per file: the ollama program is red and the model weights orange,
on disk and in memory, so the loads (1) and (3) join boxes of the same colour.
The disk holds the two files among others. DRAM shows only the two regions
of the ollama process that the flows use: the mapped weights and the code
segment. Address spaces come later in the course, so the figure does not name
one. The CPU is one block.
The two panels on the left magnify a few bytes of each file on disk.

The bytes are real. The GGUF bytes 47 47 55 46 03 00 00 00 ("GGUF", then
version 3 as a little-endian uint32) were read from the llama3.2 blob under
/usr/share/ollama/.ollama/models/blobs; every GGUF v3 file, llama3.gguf
included, starts with these 8 bytes.

The ollama bytes are the 22 at file offset 0xcb4cf0 of /usr/local/bin/ollama
(ollama 0.33.2, x86-64), which is also their virtual address in this PIE:

    xxd -s 0xcb4cf0 -l 22 /usr/local/bin/ollama
    objdump -d --start-address=0xcb4cf0 --stop-address=0xcb4d06 /usr/local/bin/ollama

They are the one-element loop of gonum's f32.DotUnitary(x, y []float32), the
float32 inner product that ollama links in. The binary is stripped; the
function's range comes from .gopclntab (debug/gosym, with runtime.text read
from the R_X86_64_RELATIVE entry of firstmoduledata). %rsi walks the first
vector and %rdi the second, so the lens names them w and x as dot_product does.
Another ollama build puts the loop at another offset.

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
                    GREEN, INK, LINE, MUTED, ORANGE, RED, WHITE, arrow, mono,
                    rect, save, text)

W, H = 1120, 560
TOP, BOT = 124, 492                 # vertical extent of the three columns
LENS = "#f4fbf6"

LX, LW = 16, 360                    # the two magnifier panels
DX, DW = 406, 120                   # disk
MX, MW = 626, 150                   # DRAM
CX, CW = 876, 84                    # CPU
TX = 990                            # terminal

# (top, bottom) of the boxes; a file on disk and its region in memory share a centre
GGUF_Y, OLLAMA_Y = (150, 240), (330, 400)
BUF_Y, CODE_Y = (140, 250), (335, 395)

GGUF = [0x47, 0x47, 0x55, 0x46, 0x03, 0x00, 0x00, 0x00]

# xxd -s 0xcb4cf0 -l 22 /usr/local/bin/ollama, one instruction per row
LOOP = [("f3 0f 10 14 86", "movss (%rsi,%rax,4),%xmm2"),
        ("f3 0f 59 14 87", "mulss (%rdi,%rax,4),%xmm2"),
        ("f3 0f 58 c2", "addss %xmm2,%xmm0"),
        ("48 ff c0", "inc   %rax"),
        ("48 ff cb", "dec   %rbx"),
        ("75 ea", "jne   cb4cf0")]


def photo(name, x, y, w, h):
    data = base64.b64encode((ASSETS / name).read_bytes()).decode()
    return [f'<image x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" '
            f'preserveAspectRatio="xMidYMid meet" href="data:image/jpeg;base64,{data}"/>']


def mid(span):
    return (span[0] + span[1]) / 2


def zoom(seg, panel, color, fill):
    """Dashed trapezoid from a disk box's left edge to its panel."""
    (s0, s1), (p0, p1) = seg, panel
    x0, x1 = DX + 8, LX + LW
    return [f'<path d="M {x0} {s0} L {x1} {p0} L {x1} {p1} L {x0} {s1} Z" fill="{fill}" '
            f'fill-opacity="0.55" stroke="{color}" stroke-width="1.4" '
            f'stroke-dasharray="5 4"/>']


def gguf_lens(y):
    h, cx = 126, LX + LW / 2
    out = [rect(LX, y, LW, h, LENS, GREEN, rx=10, width=2),
           mono(cx, y + 22, "llama3.gguf 的前 8 字节", 14, ORANGE, "bold", anchor="middle")]
    for i in range(2):
        row = " ".join(f"{b:08b}" for b in GGUF[4 * i:4 * i + 4])
        out.append(mono(cx, y + 46 + 22 * i, row, 15, GREEN, "bold", anchor="middle"))
    out.append(mono(cx, y + 94, " ".join(f"{b:02x}" for b in GGUF), 13, INK,
                    anchor="middle"))
    out.append(text(cx, y + 114, "GGUF 魔数，格式版本 3", 13, MUTED))
    return out, (y, y + h)


def ollama_lens(y):
    h, cx, lh = 230, LX + LW / 2, 22
    bx, ax = LX + 14, LX + 134            # byte column, instruction column
    out = [rect(LX, y, LW, h, LENS, GREEN, rx=10, width=2),
           mono(cx, y + 22, "ollama 文件偏移 0xcb4cf0 处的 22 字节", 14, RED, "bold",
                anchor="middle"),
           text(bx, y + 44, "机器码", 12, MUTED, anchor="start"),
           text(ax, y + 44, "汇编指令", 12, MUTED, anchor="start")]
    top = y + 52
    for i, (code, insn) in enumerate(LOOP):
        ry = top + i * lh
        out.append(rect(LX + 8, ry, LW - 16, lh - 2, FILL_ORANGE if i < 3 else FILL_BLUE,
                        "none", rx=3, width=0))
        out.append(mono(bx, ry + 15, code, 13, GREEN, "bold"))
        out.append(mono(ax, ry + 15, insn, 13, INK))
    out.append(text(bx, y + 202, "橙色 3 条：sum += w[i] * x[i]（float32）", 13, ORANGE,
                    anchor="start"))
    out.append(text(bx, y + 221, "蓝色 3 条：i 加 1，剩余次数减 1，非 0 则跳回", 13, BLUE,
                    anchor="start"))
    return out, (y, y + h)


def box(x, w, span, label, sub, fill, stroke, is_mono=False):
    y0, y1 = span
    out = [rect(x, y0, w, y1 - y0, fill, stroke, rx=3, width=1.6)]
    cy = mid(span)
    if is_mono:
        out.append(mono(x + w / 2, cy + 5, label, 15, INK, "bold", anchor="middle"))
    elif sub:
        out.append(text(x + w / 2, cy - 2, label, 16, INK, "bold"))
        out.append(text(x + w / 2, cy + 18, sub, 12, MUTED))
    else:
        out.append(text(x + w / 2, cy + 5, label, 14, INK))
    return out


def disk():
    gx = (LX + DX + DW) / 2               # the panels and the disk share a heading
    out = photo("photo-hdd.jpg", gx - 144, 0, 88, 66)
    out += photo("photo-ssd.jpg", gx - 40, 8, 184, 51)
    out.append(text(gx - 100, 80, "机械硬盘", 12, MUTED))
    out.append(text(gx + 52, 80, "固态硬盘（NVMe）", 12, MUTED))
    out.append(text(gx, 102, "外部存储（Disk）", 19, INK, "bold"))
    out.append(rect(DX, TOP, DW, BOT - TOP, WHITE, LINE, rx=6, width=1.6))
    body, gl = gguf_lens(TOP)
    out += zoom(GGUF_Y, gl, ORANGE, FILL_ORANGE) + body
    body, ol = ollama_lens(BOT - 230)
    out += zoom(OLLAMA_Y, ol, RED, FILL_RED) + body
    x, w = DX + 8, DW - 16
    out += box(x, w, GGUF_Y, "llama3.gguf", None, FILL_ORANGE, ORANGE, True)
    out += box(x, w, (262, 308), "其他文件", None, FILL_GREY, LINE)
    out += box(x, w, OLLAMA_Y, "ollama", None, FILL_RED, RED, True)
    out.append(text(LX + LW / 2, 518, "程序与数据都是比特序列", 16, INK, "bold"))
    out.append(text(DX + DW / 2, 516, "非易失性介质", 14, MUTED))
    return out


def dram():
    out = photo("photo-dram.jpg", MX + MW / 2 - 127, 8, 254, 60)
    out.append(text(MX + MW / 2, 102, "主存储器（DRAM）", 19, INK, "bold"))
    out.append(rect(MX, TOP, MW, BOT - TOP, WHITE, LINE, rx=6, width=1.6))
    x, w = MX + 10, MW - 20
    out += box(x, w, BUF_Y, "模型数据缓冲区", "llama3.gguf 的权重", FILL_ORANGE, ORANGE)
    out += box(x, w, CODE_Y, "ollama 代码段", "由 OS 载入", FILL_RED, RED)
    out.append(text(MX + MW / 2, 516, "易失性介质", 14, MUTED))
    return out


def cpu():
    out = photo("photo-cpu.jpg", CX + CW / 2 - 36, 2, 72, 72)
    out.append(text(CX + CW / 2, 102, "中央处理器（CPU）", 19, INK, "bold"))
    out.append(rect(CX, TOP, CW, BOT - TOP, FILL_BLUE, BLUE, rx=10, width=1.8))
    out.append(text(CX + CW / 2, mid((TOP, BOT)) + 7, "CPU", 20, INK, "bold"))
    return out


def terminal():
    y = mid((TOP, BOT)) - 38
    return [rect(TX, y, 116, 76, INK, INK, rx=6),
            mono(TX + 58, y + 45, "Token", 18, "#7ee2a0", "bold", anchor="middle"),
            rect(TX + 48, y + 76, 20, 14, MUTED, MUTED, rx=1),
            rect(TX + 28, y + 90, 60, 7, MUTED, MUTED, rx=2),
            text(TX + 58, y + 124, "终端输出", 15, MUTED)]


def flows():
    left, right = (DX + DW + MX) / 2, (MX + MW + CX) / 2
    code, buf, out_y = mid(CODE_Y), mid(BUF_Y), mid((TOP, BOT))
    return [
        # 1 red: load the program into the code segment
        arrow(DX + DW - 8, code, MX + 8, code, RED, 3),
        text(left, code - 12, "① 装载程序", 16, RED, "bold"),
        # 3 orange: read the weights into the buffer
        arrow(DX + DW - 8, buf, MX + 8, buf, ORANGE, 3),
        text(left, buf - 12, "③ 读入权重", 16, ORANGE, "bold"),
        # 2 blue: fetch instructions
        arrow(MX + MW - 10, code, CX - 2, code, BLUE, 3),
        text(right, code - 12, "② 取指运行", 16, BLUE, "bold"),
        # 4 green: operands into the CPU, the result out to the terminal
        arrow(MX + MW - 10, buf, CX - 2, buf, GREEN, 3),
        text(right, buf - 12, "④ 加载数据", 16, GREEN, "bold"),
        arrow(CX + CW, out_y, TX - 2, out_y, GREEN, 3),
        text(TX + 58, out_y - 58, "④ 计算并输出", 16, GREEN, "bold"),
    ]


def build():
    return disk() + dram() + cpu() + terminal() + flows()


if __name__ == "__main__":
    save("system-view", W, H, build())
