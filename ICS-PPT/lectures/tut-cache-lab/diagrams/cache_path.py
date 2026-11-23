#!/usr/bin/env python3
"""The path of a memory access in mini-cpu from lab 2 on, file by file.

The canvas is shared with cache_path_lab1.py, the machine of lab 1: the
two slides follow each other, and every part drawn by base() stands in the
same place on both, so that turning the slide changes only the middle panel
and the paths below it.

base(): the mini-cpu process, a grey band. In its top
left corner the interpreter of lab 1 calls frame/cpu/mem.c below it (fetch,
load, store), the memory access unit of the CPU. mem.c (fetch_bytes,
mem_read, mem_write) hands a physical address to the middle panel, one
line per call. On the right is the physical memory of the
guest, the file dram.img mapped with mmap into the process. The two arrows
from the middle panel to the mapping are dram_ptr, which lab 1 uses
(frame/cpu/nocache.c). The band leaves room below the middle panel for the
functions of frame/cpu/dram.c and the paths of build().

The row is symmetric: mem.c and the mapping have one width, the two gaps
around the middle panel another, and the margins of the band are equal.

build(): src/lab2/cache.c in the middle panel: cache_read, cache_write and
the 64 x 4 lines, an array in the process. A dashed box around the sets
says that they are in the caches of the host CPU. The arrows of dram_ptr
are crossed out, since tests/srccheck.sh rejects dram_ptr in cache.c. The
cache reaches memory through dram_line_read and dram_line_write of
frame/cpu/dram.c, which take the line out of the caches of the host with
clflush and so access physical memory (red, labelled between the two
paths). The paths do not cross: the write path leaves dram_line_write on
the right and rises at WRITE_UP; dram_line_read stands one row lower, and
the read path comes down at READ_DOWN and enters it from the right, under
dram_line_write. The black arrows between the cache and the two functions
point the way the line moves: up from dram_line_read, down to
dram_line_write. frame/main.c calls
cache_init and cache_flush. The legend, in the top right corner of the
band, belongs to base() and shows on both slides.
Run this script to refresh ../assets/cache-path.svg.
"""

from cache_kit import *

MARGIN = 20                   # inside the band, on both sides
SIDE_W = 210                  # frame/cpu/mem.c and the mapping
GAP = 100                     # mem.c to the middle panel, the middle panel to the mapping
C_W = 344                     # src/lab2/cache.c or frame/cpu/nocache.c
PB_X0 = 4                     # the band of the mini-cpu process
B_X = PB_X0 + MARGIN          # frame/cpu/mem.c
C_X = B_X + SIDE_W + GAP
M_X = C_X + C_W + GAP         # the mapping of dram.img
PB_X1 = M_X + SIDE_W + MARGIN
W = PB_X1 + PB_X0

PB_Y0 = 2
I_Y, I_H = 26, 66             # src/lab1/interp.c, above mem.c; its chip clears the band
TOP = I_Y + I_H + 38          # panels
BOT = TOP + 250
ROWS = (TOP + 96, TOP + 138, TOP + 180)   # fetch_bytes, mem_read, mem_write
READ_Y, WRITE_Y = (ROWS[0] + ROWS[1]) / 2, ROWS[2]   # the two entries of the middle panel
CB_Y0, CB_Y1 = TOP + 78, TOP + 238        # the box of the host CPU caches, around the sets
FN_Y = BOT + 36               # dram_line_write
READ_FN_Y = FN_Y + 34         # dram_line_read, one row lower: its path passes under dram_line_write
PB_Y1 = READ_FN_Y + 30
H = PB_Y1 + PB_Y0             # the canvas, cache-path-lab1.svg included
MEM_FNS = ("fetch_bytes", "mem_read", "mem_write")

# The contents of the middle panel, centred as one group: the two entries
# on one centre line, an arrow, and the box of the host CPU caches with the
# set numbers and the four ways inside.
ENTRY_W = pill_width("cache_write")
ARROW_W = 22
LABEL_W = width_of("组 63", 14) + 10
CELL_W = 26
GRID_W = 4 * CELL_W
CB_PAD = 8                    # inside the box, left of the set numbers and right of the ways
CB_W = CB_PAD + LABEL_W + GRID_W + CB_PAD
GROUP_W = ENTRY_W + 4 + ARROW_W + CB_W
PCX = C_X + (C_W - GROUP_W) / 2 + ENTRY_W / 2      # centre line of the two entries
CB_X = PCX + ENTRY_W / 2 + 4 + ARROW_W             # the box, where the two arrows end
GRID_X = CB_X + CB_PAD + LABEL_W

LEGEND_D = 20                 # the legend: this far from the top and the right of the band
LEGEND = [("fn", "你实现的函数"), ("given", "已写出的函数"), ("frame", "框架的函数")]


def process():
    """The band of the mini-cpu process, behind the panels."""
    return [rect(PB_X0, PB_Y0, PB_X1 - PB_X0, PB_Y1 - PB_Y0, FILL_GREY, MUTED, rx=10,
                 width=1.4),
            text(B_X, PB_Y1 - 14, "mini-cpu 进程", 15, MUTED, "bold", "start")]


def interpreter():
    """The interpreter of lab 1, calling mem.c below it."""
    out = panel(B_X, I_Y, SIDE_W, I_H, "src/lab1/interp.c", "lab1", "解释器",
                chip_label="实验一")
    x = B_X + 44
    out.append(arrow(x, I_Y + I_H, x, TOP - 3, INK, 1.8))
    out.append(text(x + 12, I_Y + I_H + 17, "取指、读数据、写数据", 14, MUTED, anchor="start"))
    return out


def mem_c():
    out = panel(B_X, TOP, SIDE_W, BOT - TOP, "frame/cpu/mem.c", "frame", "CPU 的访存单元",
                chip_label="框架")
    for y, name in zip(ROWS, MEM_FNS):
        shapes, _, _ = frame_fn(B_X + SIDE_W / 2, y, name, 15, "middle")
        out += shapes
    out.append(text(B_X + SIDE_W / 2, BOT - 18, "给出物理地址 pa", 14, MUTED))
    return out


def mem_fn_end(name):
    """The right end of a function box of mem.c, as frame_fn draws it."""
    return B_X + SIDE_W / 2 + (width_of(name, 15, MONO) + 16) / 2


def entries(read_x, write_x):
    """mem.c -> the two entries, whose left ends are read_x and write_x:
    fetch_bytes and mem_read both read."""
    jx = B_X + SIDE_W + GAP / 2
    out = []
    for y, name in zip(ROWS[:2], MEM_FNS[:2]):
        out.append(path(f"M {mem_fn_end(name) + 3:.1f} {y:.1f} L {jx:.1f} {y:.1f} "
                        f"L {jx:.1f} {READ_Y:.1f}", INK, 1.8, head=False))
    out.append(arrow(jx, READ_Y, read_x - 3, READ_Y, INK, 1.8))
    out.append(arrow(mem_fn_end(MEM_FNS[2]) + 3, WRITE_Y, write_x - 3, WRITE_Y, INK, 1.8))
    return out


def mapping():
    """The physical memory of the guest: dram.img mapped into the mini-cpu process."""
    cx = M_X + SIDE_W / 2
    out = [rect(M_X, TOP, SIDE_W, BOT - TOP, WHITE, MEM_EDGE, rx=8, width=2.0),
           text(cx, TOP + 30, "dram.img 的映射", 14, MEM_EDGE),
           text(cx, TOP + 56, "物理内存", 18, MEM_EDGE, "bold")]
    x0 = M_X + (SIDE_W - 12 * 14) / 2
    for r in range(8):                    # bytes, not lines: a grid of small cells
        for k in range(12):
            out.append(rect(x0 + k * 14, TOP + 78 + r * 17, 14, 13, MEM_FILL, MEM_EDGE,
                            rx=0, width=0.8))
    out.append(text(cx, BOT - 16, "64 MiB", 15, MEM_EDGE, "bold"))
    return out


def dram_ptr(cross=False):
    """The two arrows of dram_ptr, from the middle panel into the mapping."""
    out = [arrow(C_X + C_W, y, M_X - 3, y, INK, 1.8) for y in (READ_Y, WRITE_Y)]
    gx = C_X + C_W + GAP / 2
    out.append(mono(gx, (READ_Y + WRITE_Y) / 2 + 6, "dram_ptr", 16, INK, "bold",
                    anchor="middle"))
    if cross:
        for y in (READ_Y, WRITE_Y):
            out.append(line(gx - 11, y - 11, gx + 11, y + 11, RED, 3.4))
            out.append(line(gx - 11, y + 11, gx + 11, y - 11, RED, 3.4))
    return out


def legend_column(items, gap=24, size=14):
    """The legend as a column, LEGEND_D from the top and the right of the band."""
    width = 34 + max(width_of(label, size) for _, label in items)
    x, y = PB_X1 - LEGEND_D - width, PB_Y0 + LEGEND_D + 12
    out = []
    for k, item in enumerate(items):
        out += legend(x, y + k * gap, [item], size)
    return out


def base(cross=False):
    return (process() + interpreter() + mem_c() + mapping() + dram_ptr(cross)
            + legend_column(LEGEND))


def grid(x, y, cell_w=CELL_W, cell_h=18, gap=5, rows=("0", "1", "2", "…", "63")):
    """The 64 x 4 lines of the cache, drawn as a few of the sets."""
    out = []
    for r, name in enumerate(rows):
        ry = y + r * (cell_h + gap)
        if name == "…":
            out.append(text(x + 2 * cell_w, ry + cell_h - 3, "⋮", 16, MUTED))
            continue
        out.append(text(x - 10, ry + cell_h - 4, f"组 {name}", 14, IDX, "bold", "end"))
        for way in range(4):
            out.append(rect(x + way * cell_w, ry, cell_w, cell_h, LINE_FILL, LINE_EDGE,
                            rx=0, width=1.2))
    return out, y + len(rows) * (cell_h + gap) - gap


def cpu_cache_box():
    """A dashed box around the sets: the array lives in the caches of the host CPU."""
    label = "宿主 CPU 的缓存"
    cx, lw = CB_X + CB_W / 2, width_of(label, 14) + 12
    return [rect(CB_X, CB_Y0, CB_W, CB_Y1 - CB_Y0, "none", LINE_EDGE, rx=6, width=1.4,
                 dash="6 4"),
            rect(cx - lw / 2, CB_Y0 - 9, lw, 18, WHITE, "none", rx=0, width=0),
            text(cx, CB_Y0 + 5, label, 14, LINE_EDGE, "bold")]


def cache_c():
    out = panel(C_X, TOP, C_W, BOT - TOP, "src/lab2/cache.c", "student", "缓存",
                chip_label="TODO")
    shapes, read_x0, read_x1 = fn(PCX, READ_Y, "cache_read", 15, "middle")
    out += shapes
    shapes, write_x0, write_x1 = fn(PCX, WRITE_Y, "cache_write", 15, "middle")
    out += shapes
    out += entries(read_x0, write_x0)
    out += cpu_cache_box()
    shapes, gy1 = grid(GRID_X, CB_Y0 + 18)
    out += shapes
    out.append(arrow(read_x1 + 4, READ_Y, CB_X - 2, READ_Y, INK, 1.6))
    out.append(arrow(write_x1 + 4, WRITE_Y, CB_X - 2, WRITE_Y, INK, 1.6))
    out.append(text(GRID_X + GRID_W / 2, gy1 + 22, "64 组 × 4 路", 15, LINE_EDGE, "bold"))
    return out


WRITE_UP = M_X + 40           # where the write path rises into the mapping
READ_DOWN = M_X + SIDE_W - 40  # where the read path leaves it


def transfers():
    """cache.c -> dram_line_read and dram_line_write -> physical memory,
    past the caches of the host."""
    read_w = width_of("dram_line_read", 15, MONO) + 16
    write_w = width_of("dram_line_write", 15, MONO) + 16
    x0 = C_X + (C_W - (read_w + 24 + write_w)) / 2
    rx, wx = x0 + read_w / 2, x0 + read_w + 24 + write_w / 2
    out = [mono(x0 - 12, (FN_Y + READ_FN_Y) / 2 + 5, "frame/cpu/dram.c", 14, MUTED, anchor="end")]
    for x, y, name in ((rx, READ_FN_Y, "dram_line_read"), (wx, FN_Y, "dram_line_write")):
        shapes, _, _ = frame_fn(x, y, name, 15, "middle")
        out += shapes
    # the black arrows follow the line: up from dram_line_read into the cache,
    # down from the cache into dram_line_write
    out.append(arrow(rx, READ_FN_Y - 18, rx, BOT + 3, INK, 1.6))
    out.append(arrow(wx, BOT + 2, wx, FN_Y - 16, INK, 1.6))
    write_x1, read_x1 = wx + write_w / 2, rx + read_w / 2
    out.append(elbow([(write_x1 + 2, FN_Y), (WRITE_UP, FN_Y), (WRITE_UP, BOT + 3)], RED, 2.4))
    out.append(elbow([(READ_DOWN, BOT + 2), (READ_DOWN, READ_FN_Y), (read_x1 + 3, READ_FN_Y)],
                     RED, 2.4))
    out.append(text((WRITE_UP + READ_DOWN) / 2, BOT + 26, "访问物理内存", 15, RED, "bold"))
    return out


def main_c():
    """frame/main.c above the cache: cache_init and cache_flush."""
    out = []
    bx, bw = C_X + 40, C_W - 80
    out.append(rect(bx, I_Y, bw, 30, WHITE, MUTED, rx=6, width=1.4))
    out.append(mono(bx + bw / 2, I_Y + 20, "frame/main.c", 15, MUTED, anchor="middle"))
    for x, name, kind in ((C_X + C_W / 2 - 95, "cache_init", given_fn),
                          (C_X + C_W / 2 + 95, "cache_flush", fn)):
        out.append(arrow(x, I_Y + 30, x, TOP - 12, INK, 1.6))
        shapes, _, _ = kind(x, I_Y + 60, name, 15, "middle")
        out += shapes
    return out


def build():
    out = base(cross=True)
    out += cache_c()
    out += transfers()
    out += main_c()
    return out


if __name__ == "__main__":
    save("cache-path", W, H, build())
