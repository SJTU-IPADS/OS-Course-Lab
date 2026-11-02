#!/usr/bin/env python3
"""Static storage structure: registers and cache inside the CPU, memory outside.

Memory holds more blocks than the cache. Both latency labels measure the
complete time until the CPU receives its data, including the cache lookup
on a memory access. The memory number is not a bus-link delay alone.
The rounded teaching estimates span cache access times of roughly 1 to
40 ns in Intel's memory hierarchy overview and Arm's 100 ns memory example.
cache_hit_miss.py reuses this layout for its three access frames.
Run this script to refresh ../assets/cache-between.svg.
"""

from cachekit import *


W, H = 1000, 276
ANIMATION_H = 302
CPU_X, CPU_Y, CPU_W, CPU_H = 20, 26, 620, 230
REG_X, REG_Y, REG_W, REG_H = 100, 110, 160, 80
CACHE_X, CACHE_Y, CACHE_W, CACHE_H = 380, 58, 220, 172
MEM_X, MEM_Y, MEM_W, MEM_H = 720, 38, 260, 216
AX0, AX1 = REG_X + REG_W, CACHE_X
BX0, BX1 = CACHE_X + CACHE_W, MEM_X
Y_REQ, Y_REP = 128, 162
MEM_LABEL_X = (CPU_X + CPU_W + MEM_X) / 2
LINES = 4
LINE_Y = [CACHE_Y + 44 + index * 26 for index in range(LINES)]
MEM_COLS, MEM_ROWS = 4, 4
TARGET = 6
HIT_LINE, NEW_LINE = 1, 3


def access_times(y):
    """Compact CPU-to-data times, placed above the two access connections."""
    return [
        text((AX0 + AX1) / 2, y, "≈1–50 ns", 17, BLUE, "bold"),
        text(MEM_LABEL_X, y, "≈100 ns", 17, GREEN, "bold"),
    ]


def line_cells(y, state):
    """A cached data block; metadata is introduced in separate figures."""
    x, width = CACHE_X + 20, CACHE_W - 40
    out = [rect(x, y, width, 22, FILL_BLUE, INK, rx=2, width=1.4)]
    label = "旧块" if state == "old" else "数据块"
    out.append(text(x + width / 2 - (12 if state in ("hit", "new") else 0),
                    y + 17, label, 16, MUTED))
    if state in ("hit", "new"):
        out.append(rect(x + width - 36, y + 3, 14, 16, ORANGE, ORANGE,
                        rx=2, width=1))
        out.append(rect(x - 4, y - 4, width + 8, 30, "none",
                        GREEN if state == "hit" else ORANGE,
                        rx=5, width=3.2))
    elif state == "old":
        out.append(rect(x - 4, y - 4, width + 8, 30, "none", RED,
                        rx=5, width=2.4))
    return out


def lane(x0, x1, y, color, label, current, left_to_right=True):
    start, end = ((x0 + 4, x1 - 4) if left_to_right
                  else (x1 - 4, x0 + 4))
    label_x = MEM_LABEL_X if x0 == BX0 else (x0 + x1) / 2
    return [arrow(start, y, end, y, color, 3.6 if current else 2.2),
            text(label_x, y + (-16 if left_to_right else 24),
                 label, 17, color, "bold")]


def structure(*, cache_states=None, memory_selected=False):
    states = cache_states or ["valid"] * LINES
    out = []
    out.append(rect(CPU_X, CPU_Y, CPU_W, CPU_H, WHITE, INK, rx=8, width=2))
    out.append(text(CPU_X + CPU_W / 2, CPU_Y + 28, "CPU", 20, INK, "bold"))
    out += box(REG_X, REG_Y, REG_W, REG_H, "寄存器", FILL_GREY, INK, 18,
               sub="%rax %rbx ...", sub_size=14)

    out.append(rect(CACHE_X, CACHE_Y, CACHE_W, CACHE_H, WHITE, INK, rx=8, width=2))
    out.append(text(CACHE_X + CACHE_W / 2, CACHE_Y + 28, "缓存", 20, INK, "bold"))
    for y, state in zip(LINE_Y, states):
        out += line_cells(y, state)

    out.append(rect(MEM_X, MEM_Y, MEM_W, MEM_H, WHITE, GREEN, rx=8, width=2))
    out.append(text(MEM_X + MEM_W / 2, MEM_Y + 28, "内存", 20, GREEN, "bold"))
    for index in range(MEM_COLS * MEM_ROWS):
        col, row = index % MEM_COLS, index // MEM_COLS
        selected = index == TARGET and memory_selected
        x, y = MEM_X + 24 + col * 54, MEM_Y + 44 + row * 40
        out.append(rect(x, y, 50, 32, FILL_ORANGE if selected else FILL_GREEN,
                        ORANGE if selected else GREEN, rx=2,
                        width=3 if selected else 1.4))
        if selected:
            out.append(rect(x + 29, y + 8, 14, 16, ORANGE, ORANGE,
                            rx=2, width=1))

    return out


def build():
    out = structure()
    out.append(arrow(AX0 + 4, 148, AX1 - 4, 148, ORANGE, 2.4, both=True))
    out.append(arrow(BX0 + 4, 148, BX1 - 4, 148, GREEN, 2.4, both=True))
    out += access_times(128)
    return ['<g transform="translate(0,-18)">', *out, '</g>']


if __name__ == "__main__":
    save("cache-between", W, H, build())
