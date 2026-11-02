#!/usr/bin/env python3
"""The bus structure between the CPU and main memory, and the two memory transactions.

CPU chip (dashed): register file, ALU, bus interface. The system bus joins the
bus interface to the I/O bridge, the memory bus joins the bridge to main
memory (green). What is on the bus in the current step is orange: the address
or the data word, with an orange arrow for its direction.

read_transaction.py draws `movq A, %rax` in three frames:
  1. the CPU puts address A on the bus;
  2. main memory reads A, fetches the word x and puts it on the bus;
  3. the CPU reads x from the bus and copies it into %rax.
write_transaction.py (which imports this module) draws `movq %rax, A`:
  1. the CPU puts address A on the bus, main memory reads it and waits;
  2. the CPU puts the data word y on the bus;
  3. main memory reads y from the bus and stores it at address A.
Drawn at the width it is shown on the slide (1100 px).
Run it to refresh ../assets/read-transaction-1.svg .. read-transaction-3.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE, MUTED, ORANGE,
                    WHITE, arrow, box, line, rect, save, text, mono)

W, H = 1100, 350                         # drawn at the width it is shown on the slide
CPU = (16, 8, 520, 276)
REG = (50, 52, 120, 120)                 # register file: x, y, w, h (5 rows)
ALU = (250, 52, 90, 120)
BUSIF = (50, 208, 260, 58)
BRIDGE = (610, 208, 120, 58)
MEM = (860, 52, 224, 224)
BUS_Y = 237
SYS = (BUSIF[0] + BUSIF[2], BRIDGE[0])   # x range of the system bus
MBUS = (BRIDGE[0] + BRIDGE[2], MEM[0])   # x range of the memory bus
CELL_Y = 160                             # the memory cell at address A


def static(rax, cell):
    """Everything that does not move. rax / cell are (text, highlighted) or None."""
    out = []
    cx, cy, cw, ch = CPU
    out.append(rect(cx, cy, cw, ch, "none", MUTED, rx=6, width=1.4, dash="6 4"))
    out.append(text(cx + cw - 14, cy + 24, "CPU 芯片", 18, MUTED, "bold", "end"))
    # register file
    x, y, w, h = REG
    out.append(text(x + w / 2, y - 8, "寄存器堆", 17, INK, "bold"))
    for r in range(5):
        hot = r == 0 and rax is not None and rax[1]
        out.append(rect(x, y + r * h / 5, w, h / 5, FILL_ORANGE if hot else WHITE,
                        ORANGE if hot else MUTED, rx=0, width=1.6 if hot else 1.2))
    out.append(mono(x + 8, y + 18, "%rax", 17, INK, "bold"))
    if rax is not None:
        out.append(text(x + w - 12, y + 18, rax[0], 19, ORANGE if rax[1] else INK, "bold", "end"))
    # ALU
    ax, ay, aw, ah = ALU
    out += box(ax, ay, aw, ah, "ALU", FILL_GREY, MUTED, size=19)
    out.append(arrow(x + w + 2, y + 36, ax - 2, y + 36, MUTED, 1.6))
    out.append(arrow(ax - 2, y + 84, x + w + 2, y + 84, MUTED, 1.6))
    # bus interface
    bx, by, bw, bh = BUSIF
    out += box(bx, by, bw, bh, "总线接口", FILL_GREY, MUTED, size=19)
    out.append(arrow(x + w / 2, by - 2, x + w / 2, y + h + 4, MUTED, 2, both=True))
    # I/O bridge and main memory
    px, py, pw, ph = BRIDGE
    out += box(px, py, pw, ph, "I/O 桥", FILL_GREY, MUTED, size=19)
    mx, my, mw, mh = MEM
    out.append(rect(mx, my, mw, mh, FILL_GREEN, GREEN, rx=6, width=1.6))
    out.append(text(mx + mw / 2, my + 30, "内存", 19, INK, "bold"))
    for k, ty in enumerate((CELL_Y - 46, CELL_Y, CELL_Y + 46)):
        if k == 1:
            hot = cell is not None and cell[1]
            out.append(rect(mx + 80, ty, 124, 32, FILL_ORANGE if hot else WHITE,
                            ORANGE if hot else GREEN, rx=2, width=1.8 if hot else 1.2))
            out.append(mono(mx + 28, ty + 22, "A", 18, INK, "bold"))
            if cell is not None:
                out.append(text(mx + 142, ty + 23, cell[0], 19, ORANGE if hot else INK, "bold"))
        else:
            out.append(rect(mx + 80, ty, 124, 32, WHITE, GREEN, rx=2, width=1.2))
            out.append(text(mx + 142, ty + 23, "…", 19, MUTED))
    # the two buses
    sx0, sx1 = SYS
    mx0, mx1 = MBUS
    out.append(line(sx0, BUS_Y, sx1, BUS_Y, LINE, 8))
    out.append(line(mx0, BUS_Y, mx1, BUS_Y, LINE, 8))
    out.append(text((sx0 + sx1) / 2, BUS_Y + 40, "系统总线", 17, MUTED, "bold"))
    out.append(text((mx0 + mx1) / 2, BUS_Y + 40, "内存总线", 17, MUTED, "bold"))
    return out


def traffic(label, to_memory, to_register=False):
    """The word on the buses this step: a chip on each bus and an orange arrow."""
    out = []
    sx0, sx1 = SYS
    mx0, mx1 = MBUS
    for a, b in ((sx0, sx1), (mx0, mx1)):
        if to_memory:
            out.append(arrow(a + 2, BUS_Y, b - 2, BUS_Y, ORANGE, 5))
        else:
            out.append(arrow(b - 2, BUS_Y, a + 2, BUS_Y, ORANGE, 5))
        mid = (a + b) / 2
        out.append(rect(mid - 20, BUS_Y - 40, 40, 30, FILL_ORANGE, ORANGE, rx=4, width=1.6))
        out.append(text(mid, BUS_Y - 18, label, 20, INK, "bold"))
    if to_register:
        rx_, ry, rw, rh = REG
        out.append(arrow(rx_ + rw / 2 + 14, BUSIF[1] - 2, rx_ + rw / 2 + 14, ry + rh + 4,
                         ORANGE, 3))
    return out


def caption(s):
    return [text(W / 2, H - 16, s, 20, INK, "bold")]


FRAMES = [
    (None, ("x", False), "A", True, False, "① CPU 将地址 A 放到总线上"),
    (None, ("x", True), "x", False, False, "② 内存从总线读出 A，取出字 x，放到总线上"),
    (("x", True), ("x", True), "x", False, True, "③ CPU 从总线读出字 x，复制到寄存器 %rax"),
]


def frame(k):
    rax, cell, label, to_memory, to_reg, cap = FRAMES[k - 1]
    return static(rax, cell) + traffic(label, to_memory, to_reg) + caption(cap)


if __name__ == "__main__":
    for k in (1, 2, 3):
        save(f"read-transaction-{k}", W, H, frame(k))
