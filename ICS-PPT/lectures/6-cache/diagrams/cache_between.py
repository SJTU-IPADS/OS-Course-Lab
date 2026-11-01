#!/usr/bin/env python3
"""A cache between the CPU and main memory, in five frames. The CPU asks the cache
for a word at an address (frame 1). Case one: the block holding the word is in the
cache, and the cache returns the word (frame 2). Case two: it is not; the cache
asks main memory for the whole block (frame 3), main memory returns it and the
cache stores it in a line (frame 4), and the cache returns the word (frame 5).
Every frame has the same boxes; only the arrows and the marked parts change.
Run it to refresh ../assets/cache-between-1.svg ... cache-between-5.svg.
"""

from cachekit import *

W, H = 1000, 356
TOP, BH = 36, 170
CPU_X, CPU_W = 30, 230
L1_X, L1_W = 390, 220
MEM_X, MEM_W = 740, 230
AX0, AX1 = CPU_X + CPU_W, L1_X
BX0, BX1 = L1_X + L1_W, MEM_X
Y_REQ, Y_REP = TOP + 66, TOP + 122          # the two lanes of both gaps
LINES = 4
LINE_Y = [TOP + 48 + k * 28 for k in range(LINES)]
MEM_COLS, MEM_ROWS = 4, 3
TARGET = 6                                   # the memory block that holds the word of A
HIT_LINE, NEW_LINE = 1, 3                    # the line that holds it (case one); the filled line (case two)

CAPTIONS = {1: ("CPU 向缓存发出地址 A", ORANGE),
            2: ("情形一：A 所在的块在缓存中，缓存直接返回 A 处的字", GREEN),
            3: ("情形二：A 所在的块不在缓存中，缓存向内存发出块的地址", RED),
            4: ("内存返回整个块，缓存把它存入一行", GREEN),
            5: ("缓存返回 A 处的字", ORANGE)}


def line_cells(y, state):
    """One cache line as [有效位 | 标记 | 块]; state is 'valid', 'empty', 'hit' or 'new'."""
    x = L1_X + 20
    w = L1_W - 40
    out = []
    if state == "empty":
        out.append(rect(x, y, w, 22, WHITE, LINE, rx=2, width=1.4, dash="4 3"))
        out.append(text(x + w / 2, y + 16, "空", 14, MUTED))
        return out
    out.append(rect(x, y, w, 22, FILL_BLUE, INK, rx=2, width=1.4))
    out.append(line(x + 34, y, x + 34, y + 22, INK, 1.2))
    out.append(line(x + 68, y, x + 68, y + 22, INK, 1.2))
    out.append(text(x + 17, y + 16, "有效位", 11, MUTED))
    out.append(text(x + 51, y + 16, "标记", 12, PURPLE, "bold"))
    out.append(text(x + 68 + (w - 68) / 2 - 12, y + 16, "块", 14, MUTED))
    if state in ("hit", "new"):
        out.append(rect(x + w - 36, y + 3, 14, 16, ORANGE, ORANGE, rx=2, width=1))
        out.append(rect(x - 4, y - 4, w + 8, 30, "none", GREEN if state == "hit" else ORANGE,
                        rx=5, width=3.2))
    return out


def frame(n):
    out = []
    # CPU
    out.append(rect(CPU_X, TOP, CPU_W, BH, WHITE, INK, rx=8, width=2))
    out.append(text(CPU_X + CPU_W / 2, TOP + 28, "CPU", 20, INK, "bold"))
    out += box(CPU_X + 30, TOP + 56, CPU_W - 60, 80, "寄存器", FILL_GREY, INK, 18,
               sub="%rax %rbx ...", sub_size=14)
    # cache
    out.append(rect(L1_X, TOP, L1_W, BH, WHITE, INK, rx=8, width=2))
    out.append(text(L1_X + L1_W / 2, TOP + 28, "缓存", 20, INK, "bold"))
    for k in range(LINES):
        if n == 2:
            state = "hit" if k == HIT_LINE else "valid"
        elif n == 3:
            state = "empty" if k == NEW_LINE else "valid"
        elif n >= 4:
            state = "new" if k == NEW_LINE else "valid"
        else:
            state = "valid"
        out += line_cells(LINE_Y[k], state)
    # main memory
    out.append(rect(MEM_X, TOP, MEM_W, BH, WHITE, GREEN, rx=8, width=2))
    out.append(text(MEM_X + MEM_W / 2, TOP + 28, "内存", 20, GREEN, "bold"))
    for k in range(MEM_COLS * MEM_ROWS):
        col, row = k % MEM_COLS, k // MEM_COLS
        hot = k == TARGET and n >= 3
        x, y = MEM_X + 24 + col * 46, TOP + 48 + row * 40
        out.append(rect(x, y, 42, 34, FILL_ORANGE if hot else FILL_GREEN,
                        ORANGE if hot else GREEN, rx=2, width=3 if hot else 1.4))
        if hot:
            out.append(rect(x + 25, y + 9, 14, 16, ORANGE, ORANGE, rx=2, width=1))
    # arrows: the ones of earlier steps stay, the current one is drawn thicker
    def lane(x0, x1, y, color, label, now, left_to_right=True):
        wd = 3.6 if now else 2.2
        a, b = (x0 + 4, x1 - 4) if left_to_right else (x1 - 4, x0 + 4)
        o = [arrow(a, y, b, y, color, wd)]
        o.append(text((x0 + x1) / 2, y + (-16 if left_to_right else 24), label, 17, color,
                      "bold"))
        return o
    if n >= 1:
        out += lane(AX0, AX1, Y_REQ, ORANGE, "地址 A", n == 1)
    if n in (2, 5):
        out += lane(AX0, AX1, Y_REP, ORANGE, "A 处的字", True, False)
    if n >= 3:
        out += lane(BX0, BX1, Y_REQ, GREEN, "块的地址", n == 3)
    if n >= 4:
        out += lane(BX0, BX1, Y_REP, GREEN, "整个块", n == 4, False)
    if n == 3:
        out += badge(L1_X + L1_W / 2, TOP + BH + 20, "不命中", "miss", size=15)
    if n == 2:
        out += badge(L1_X + L1_W / 2, TOP + BH + 20, "命中", "hit", size=15)
    # transfer units
    y = TOP + BH + 52
    out.append(text((AX0 + AX1) / 2, y, "传输单位：字（1 至 8 字节）", 16, ORANGE, "bold"))
    out.append(text((BX0 + BX1) / 2, y, "传输单位：块（B 字节）", 16, GREEN, "bold"))
    out.append(text((AX0 + AX1) / 2, y + 22, "由指令的操作数宽度决定", 14, MUTED))
    out.append(text((BX0 + BX1) / 2, y + 22, "块大小由缓存模型决定", 14, MUTED))
    cap, col = CAPTIONS[n]
    out.append(text(W / 2, H - 14, cap, 20, col, "bold"))
    return out


if __name__ == "__main__":
    for n in range(1, 6):
        save(f"cache-between-{n}", W, H, frame(n))
