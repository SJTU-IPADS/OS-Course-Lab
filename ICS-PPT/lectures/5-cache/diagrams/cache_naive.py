#!/usr/bin/env python3
"""The most direct cache: every entry holds a full address and the 8 bytes stored
at that address. The CPU's address is compared with the address of every entry;
the entry whose address is equal is a hit. Half of each entry is the address, and
a lookup compares all n entries. The addresses are short hex numbers for
legibility; the braces give the real widths (8 bytes each).
Run it to refresh ../assets/cache-naive.svg.
"""

from cachekit import *

W, H = 820, 262
ROW_H = 36
COL_W = 170
X_TABLE, Y_TABLE = 330, 70
ADDRS = ["0x2000", "0x1008", "0x1040", "0x3ff8", "0x0ff0"]
REQ = "0x1040"
HIT = ADDRS.index(REQ)


def figure():
    out = []
    n = len(ADDRS)
    bottom = Y_TABLE + n * ROW_H
    # the address the CPU gives, left of the table, at its vertical middle
    mid = (Y_TABLE + bottom) / 2
    bx, bw, bh = 30, 150, 40
    out.append(text(bx + bw / 2, mid - bh / 2 - 12, "CPU 给出的地址", 16, MUTED))
    out.append(rect(bx, mid - bh / 2, bw, bh, FILL_TAG, TAG, rx=4, width=1.8))
    out.append(text(bx + bw / 2, mid + 6, REQ, 18, TAG, "bold", font=MONO))
    # one comparison per entry
    for k in range(n):
        y = Y_TABLE + (k + 0.5) * ROW_H
        out.append(arrow(bx + bw + 4, mid, X_TABLE - 6, y, TAG, width=1.6))
    out.append(text(bx + bw / 2, mid + bh / 2 + 26, "与每一项的地址比较", 16, TAG, "bold"))
    # the entries: [address | data]
    for k, addr in enumerate(ADDRS):
        y = Y_TABLE + k * ROW_H
        out.append(rect(X_TABLE, y, COL_W, ROW_H, FILL_TAG, INK, rx=0, width=1.2))
        out.append(rect(X_TABLE + COL_W, y, COL_W, ROW_H, FILL_BLUE, INK, rx=0, width=1.2))
        out.append(text(X_TABLE + COL_W / 2, y + ROW_H / 2 + 6, addr, 17, TAG, "bold",
                        font=MONO))
        out.append(text(X_TABLE + COL_W * 1.5, y + ROW_H / 2 + 6, "数据", 16, MUTED))
    out.append(rect(X_TABLE, Y_TABLE, 2 * COL_W, n * ROW_H, "none", INK, rx=0, width=1.8))
    out.append(line(X_TABLE + COL_W, Y_TABLE, X_TABLE + COL_W, bottom, INK, 1.8))
    # the entry whose address is equal
    hy = Y_TABLE + HIT * ROW_H
    out.append(rect(X_TABLE - 4, hy - 4, 2 * COL_W + 8, ROW_H + 8, "none", GREEN, rx=5,
                    width=3.2))
    out.append(text(X_TABLE + 2 * COL_W + 14, hy + ROW_H / 2 + 6, "hit", 17, GREEN, "bold",
                    anchor="start"))
    # widths of the two columns and the number of entries
    out += brace(X_TABLE, X_TABLE + COL_W, Y_TABLE - 10, TAG, "地址：8 字节", 16, below=False)
    out += brace(X_TABLE + COL_W, X_TABLE + 2 * COL_W, Y_TABLE - 10, BLUE, "数据：8 字节", 16,
                 below=False)
    out += vbrace(X_TABLE + 2 * COL_W + 64, Y_TABLE, bottom, INK, "n 项", 16)
    return out


if __name__ == "__main__":
    save("cache-naive", W, H, figure())
