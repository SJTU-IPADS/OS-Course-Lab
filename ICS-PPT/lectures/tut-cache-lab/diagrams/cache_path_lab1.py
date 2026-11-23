#!/usr/bin/env python3
"""The path of a memory access in the machine of lab 1 (make LAB=1).

The canvas and the parts of cache_path.base(): the band of the process with
the interpreter, frame/cpu/mem.c and the mapping of dram.img stand where
cache-path.svg has them. The places of frame/main.c and
dram_line_read/dram_line_write stay empty; the legend shows on both slides.
In the middle is frame/cpu/nocache.c: cache_read and cache_write copy the bytes
with memcpy at dram_ptr(pa), straight in the mapping. nocache.c keeps no
cache of its own, and the caches of the host CPU keep the lines it reads
and writes all the same, done by the hardware of the CPU with neither the
program nor the operating system taking part: the dashed box of cache-path.svg stands at the
same place, the memcpy paths run through it, and the green cells inside are
lines of the mapping. On the next slide the box holds the sets of cache.c
instead, and the paths below the middle panel appear.
Run this script to refresh ../assets/cache-path-lab1.svg.
"""

from cache_kit import *
import cache_path as cp


def nocache_c():
    out = panel(cp.C_X, cp.TOP, cp.C_W, cp.BOT - cp.TOP, "frame/cpu/nocache.c", "frame",
                "直接读写映射", chip_label="框架")
    shapes, read_x0, read_x1 = frame_fn(cp.PCX, cp.READ_Y, "cache_read", 15, "middle")
    out += shapes
    shapes, write_x0, write_x1 = frame_fn(cp.PCX, cp.WRITE_Y, "cache_write", 15, "middle")
    out += shapes
    out += cp.entries(read_x0, write_x0)
    out += cp.cpu_cache_box()
    mx = cp.CB_X + cp.CB_W / 2
    for y, x1 in ((cp.READ_Y, read_x1), (cp.WRITE_Y, write_x1)):
        out.append(arrow(x1 + 4, y, mx - 44, y, INK, 1.6))
        out.append(mono(mx, y + 6, "memcpy", 17, INK, "bold", anchor="middle"))
        # on to the panel edge, where the arrow of dram_ptr (cp.dram_ptr) starts
        out.append(line(mx + 40, y, cp.C_X + cp.C_W, y, INK, 1.8))
    # lines of the mapping that the caches of the host CPU keep
    count, cell_w, cell_h = 6, 20, 14
    x0, y0 = mx - count * cell_w / 2, (cp.READ_Y + cp.WRITE_Y) / 2 - cell_h / 2
    for k in range(count):
        out.append(rect(x0 + k * cell_w, y0, cell_w, cell_h, MEM_FILL, MEM_EDGE, rx=0,
                        width=1.0))
    out.append(text(mx, cp.CB_Y1 - 16, "CPU 硬件自动缓存", 15, LINE_EDGE, "bold"))
    return out


def build():
    return cp.base(cross=False) + nocache_c()


if __name__ == "__main__":
    save("cache-path-lab1", cp.W, cp.H, build())
