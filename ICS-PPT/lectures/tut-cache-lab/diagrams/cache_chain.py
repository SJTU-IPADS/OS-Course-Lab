#!/usr/bin/env python3
"""The calls of one memory access in the framework, from the interpreter to DRAM.

Two grey regions: the hardware on the left (the interpreter of lab 1, the
files of frame/cpu/ and the cache of lab 2) and the software on the right
(frame/os/). Every file is a white band headed by its path; a band of a file
the student writes has a dark outline and a chip, 实验一 for src/lab1/interp.c
and TODO for src/lab2/cache.c, whose band is the thickest.

The rows, top to bottom, are the levels of an access: interp_run calls
x86_begin (fetch), store, and rm_get, which calls load (frame/cpu/x86.c).
These call fetch_bytes, mem_write and mem_read (frame/cpu/mem.c), which all
go to mem_access, the memory access unit of the CPU. mem_access translates
once per page with mmu_translate (frame/cpu/pagemap.c, a band of its own to
the right) and calls cache_write or cache_read once per line. The cache
reaches DRAM through dram_line_read and dram_line_write of frame/cpu/dram.c
(red, as in cache-funcs.svg). Both red arrows leave one point of the band of
cache.c, between cache_write and cache_read, since a read and a write alike
may write a dirty line back and read a line in (cache_fill). On the software side, sys_dispatch serves the
system call write with guest_read of frame/os/copy.c, which cuts the buffer
of the program at line boundaries and calls mem_read once per line.

The labels on the arrows give the split: once per page, once per line.
Read and write keep one column each from the CPU down: write on the left,
read on the right, so that the path of guest_read meets mem_read from the
right without crossing another arrow.
Run this script to refresh ../assets/cache-chain.svg.
"""

from cache_kit import *
from svgkit import label_line

W, H = 1120, 474
REG_TOP, REG_BOT = 14, 468          # the two grey regions; their chips sit on REG_TOP
HW_X0, HW_X1 = 4, 866               # hardware
SW_X0, SW_X1 = 882, 1116            # software
SW_BOT = 216                        # the software region ends above the legend
BAND_X0, BAND_X1 = 18, 852          # the bands of the hardware files
MEM_X1 = 640                        # frame/cpu/mem.c ends here; frame/cpu/pagemap.c follows
LABEL_X = 34                        # the paths at the left of the bands

Y1, Y2, Y3, Y4, Y5, Y6 = 62, 136, 206, 266, 340, 432     # the rows of functions
C1, C2, C4, C5 = 262, 405, 560, 680                       # fetch, write, read, rm_get
DRAM_W, DRAM_R = 380, 560                                 # dram_line_write, dram_line_read
MMU_X = (MEM_X1 + 16 + BAND_X1) / 2
SW_C = (SW_X0 + SW_X1) / 2
NODE_H = 26                         # a function box: size 15 + 11


def region(x0, x1, y1, label):
    return [rect(x0, REG_TOP, x1 - x0, y1 - REG_TOP, FILL_GREY, MUTED, rx=10, width=1.4)
            ] + chip((x0 + x1) / 2, REG_TOP, label, "frame", size=14)


def band(x0, y0, x1, y1, kind="frame"):
    """A file: white, grey outline; a dark outline for lab1, thick for the student."""
    edge, width = {"frame": (MUTED, 1.3), "lab1": (INK, 1.8), "student": (INK, 2.6)}[kind]
    return [rect(x0, y0, x1 - x0, y1 - y0, WHITE, edge, rx=8, width=width)]


def path_label(y, name, tone=MUTED, weight="normal"):
    return mono(LABEL_X, y + 5, name, 14, tone, weight)


def node(kind, x, y, name):
    shapes, x0, x1 = kind(x, y, name, 15, "middle")
    return shapes, x0, x1


def down(x0, x1, y0, y1, color=INK, width=1.6):
    """A call from the box centred at (x0, y0) to the box centred at (x1, y1) below it."""
    return arrow(x0, y0 + NODE_H / 2 + 1, x1, y1 - NODE_H / 2 - 3, color, width)


def hardware():
    out = region(HW_X0, HW_X1, REG_BOT, "硬件")

    # src/lab1/interp.c: interp_run
    out += band(BAND_X0, Y1 - 24, BAND_X1, Y1 + 24, "lab1")
    out += chip(110, Y1 - 24, "实验一", "lab1")
    out.append(path_label(Y1, "src/lab1/interp.c", INK))
    run_x = (C1 + C5) / 2
    shapes, _, _ = node(fn, run_x, Y1, "interp_run")
    out += shapes

    # frame/cpu/x86.c: x86_begin, store, rm_get -> load
    out += band(BAND_X0, Y2 - 24, BAND_X1, Y2 + 24)
    out.append(path_label(Y2, "frame/cpu/x86.c"))
    for x, name in ((C1, "x86_begin"), (C2, "store"), (C4, "load"), (C5, "rm_get")):
        out += node(frame_fn, x, Y2, name)[0]
    for x in (C1, C2, C5):
        out.append(down(run_x + (x - run_x) * 0.25, x, Y1, Y2))
    load_x1 = C4 + (width_of("load", 15, MONO) + 16) / 2
    rm_x0 = C5 - (width_of("rm_get", 15, MONO) + 16) / 2
    out.append(arrow(rm_x0 - 1, Y2, load_x1 + 3, Y2, INK, 1.6))

    # frame/cpu/mem.c: fetch_bytes, mem_write, mem_read -> mem_access
    out += band(BAND_X0, Y3 - 24, MEM_X1, Y4 + 24)
    out.append(path_label(Y3, "frame/cpu/mem.c"))
    out.append(text(LABEL_X, Y4 + 5, "CPU 的访存单元", 14, MUTED, "normal", "start"))
    for x, name in ((C1, "fetch_bytes"), (C2, "mem_write"), (C4, "mem_read")):
        out += node(frame_fn, x, Y3, name)[0]
        out.append(down(x, x, Y2, Y3))
    shapes, acc_x0, acc_x1 = node(frame_fn, C2, Y4, "mem_access")
    out += shapes
    for x in (C1, C2, C4):
        out.append(down(x, C2 + (x - C2) * 0.22, Y3, Y4))

    # frame/cpu/pagemap.c: mmu_translate, once per page
    out += band(MEM_X1 + 16, Y4 - 40, BAND_X1, Y4 + 24)
    out.append(mono(MMU_X, Y4 - 20, "frame/cpu/pagemap.c", 14, MUTED, anchor="middle"))
    shapes, mmu_x0, _ = node(frame_fn, MMU_X, Y4 + 6, "mmu_translate")
    out += shapes
    out.append(arrow(acc_x1 + 1, Y4 + 6, mmu_x0 - 3, Y4 + 6, INK, 1.6))
    out.append(text((acc_x1 + MEM_X1) / 2, Y4 - 2, "每页一次", 14, INK, "bold"))

    # src/lab2/cache.c: cache_write, cache_read, once per line
    out += band(BAND_X0, Y5 - 26, BAND_X1, Y5 + 26, "student")
    out += chip(110, Y5 - 26, "TODO", "student")
    out.append(path_label(Y5, "src/lab2/cache.c", INK, "bold"))
    for x, name in ((C2, "cache_write"), (C4, "cache_read")):
        out += node(fn, x, Y5, name)[0]
        out.append(down(C2 + (x - C2) * 0.22, x, Y4, Y5, INK, 2.0))
    out.append(text(C2 - 12, (Y4 + Y5) / 2 + 9, "每行一次", 14, INK, "bold", "end"))
    out += label_line(626, Y5 + 5, [("实验一中为 ", FONT, MUTED, "normal"),
                                    ("frame/cpu/nocache.c", MONO, MUTED, "normal")], 13)

    # frame/cpu/dram.c: one whole line from or to the DRAM of the host
    out += band(BAND_X0, Y6 - 24, BAND_X1, Y6 + 24)
    out.append(path_label(Y6, "frame/cpu/dram.c"))
    # one fork out of the band of cache.c, between its two functions: a read
    # or a write may call either function of dram.c
    fork_x, fork_y = (C2 + C4) / 2, (Y5 + 26 + Y6 - 24) / 2 - 6
    for x, name in ((DRAM_W, "dram_line_write"), (DRAM_R, "dram_line_read")):
        out += node(frame_fn, x, Y6, name)[0]
        out.append(elbow([(fork_x, Y5 + 27), (fork_x, fork_y), (x, fork_y),
                          (x, Y6 - NODE_H / 2 - 3)], RED, 1.8))
    out.append(text(DRAM_R + 85, Y6 + 5, "一次读写一行，访问宿主的内存", 14, RED, "bold", "start"))
    return out


def software():
    out = region(SW_X0, SW_X1, SW_BOT, "软件")
    bx0, bx1 = SW_X0 + 14, SW_X1 - 14
    # frame/os/sysbase.c: sys_dispatch, the system call write
    out += band(bx0, 42, bx1, 104)
    out.append(mono(SW_C, 62, "frame/os/sysbase.c", 14, MUTED, anchor="middle"))
    out += node(frame_fn, SW_C, 86, "sys_dispatch")[0]
    # frame/os/copy.c: guest_read, the buffer cut at line boundaries; its path
    # sits below the function, so the call from sys_dispatch crosses no label
    out += band(bx0, 126, bx1, 188)
    shapes, guest_x0, _ = node(frame_fn, SW_C, 148, "guest_read")
    out += shapes
    out.append(mono(SW_C, 177, "frame/os/copy.c", 14, MUTED, anchor="middle"))
    out.append(down(SW_C, SW_C, 86, 148))
    out.append(text(SW_C + 10, 120, "write", 14, MUTED, "normal", "start"))
    # guest_read -> mem_read, once per line: out of its left end, down the
    # gap between the regions, then along the row of mem_read
    read_x1 = C4 + (width_of("mem_read", 15, MONO) + 16) / 2
    gap_x = (HW_X1 + SW_X0) / 2
    out.append(elbow([(guest_x0 - 1, 148), (gap_x, 148), (gap_x, Y3), (read_x1 + 3, Y3)],
                     INK, 2.0))
    out.append(text((MEM_X1 + HW_X1) / 2, Y3 - 8, "每行一次", 14, INK, "bold"))
    return out


def legend_rows():
    items = [("fn", "你实现的函数"), ("frame", "框架的函数")]
    out = []
    for k, item in enumerate(items):
        out += legend(SW_X0 + 18, 262 + k * 28, [item], 14)
    return out


def build():
    return hardware() + software() + legend_rows()


if __name__ == "__main__":
    save("cache-chain", W, H, build())
