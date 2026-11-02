#!/usr/bin/env python3
"""The accesses of a stride-k traversal of an int array that fall into one 64-byte
block (16 ints), one row per stride k = 1, 2, 4, 8, 16. In a row the accessed elements
are numbered in the order of access; the first access to the block misses (red), the
others in the block hit (green), elements that are skipped are white. The number of
accesses in the block and the miss rate (one miss per block) are computed from the
stride, as min(1, 4k / 64).
Run it to refresh ../assets/friendly-miss-rate.svg.
"""

from fractions import Fraction

from cdraw import *

W, H = 1100, 346
X_K, X_GAP, X_LINE, X_CNT, X_RATE = 30, 90, 250, 840, 960
CW, CH = 34, 38
ROW = 50
TOP = 56
BLOCK, ELEM = 64, 4
STRIDES = [1, 2, 4, 8, 16]


def accessed(k):
    """Offsets in elements of the accesses that fall in the block, in order."""
    return list(range(0, BLOCK // ELEM, k))


def rate(k):
    """The miss rate: the first of the accesses to a block misses."""
    return Fraction(1, len(accessed(k)))


def row(r, k):
    y = TOP + r * ROW
    out = []
    out.append(text(X_K, y + 27, f"{k}", 22, INK, "bold", "start"))
    out.append(text(X_GAP, y + 27, f"{ELEM * k} 字节", 19, INK, "normal", "start"))
    order = accessed(k)
    for j in range(BLOCK // ELEM):
        fill, stroke, label = WHITE, LINE, None
        if j in order:
            first = j == order[0]
            fill, stroke = (FILL_RED, RED) if first else (FILL_GREEN, GREEN)
            label = str(order.index(j) + 1)
        out.append(rect(X_LINE + j * CW, y, CW, CH, fill, stroke, rx=0, width=1.4))
        if label:
            out.append(text(X_LINE + j * CW + CW / 2, y + 26, label, 16, stroke, "bold",
                            font=MONO))
    out.append(rect(X_LINE, y, 16 * CW, CH, "none", INK, rx=0, width=2))
    out.append(text(X_CNT, y + 28, f"{len(order)} 次", 20, INK, "bold", "start"))
    out.append(text(X_RATE, y + 29, f"{rate(k)}", 24, RED, "bold", "start"))
    return out


def figure():
    out = [text(X_K, 34, "步长 k", 17, MUTED, "bold", "start"),
           text(X_GAP, 34, "相邻访问相差", 17, MUTED, "bold", "start"),
           text(X_LINE, 34, "一个块：64 字节 = 16 个 int，数字是访问的次序", 17, MUTED, "bold",
                "start"),
           text(X_CNT, 34, "块内访问", 17, MUTED, "bold", "start"),
           text(X_RATE, 34, "miss 率", 17, MUTED, "bold", "start")]
    for r, k in enumerate(STRIDES):
        out += row(r, k)
    ly = TOP + 5 * ROW + 12
    for x, fill, stroke, label in ((X_LINE, FILL_RED, RED, "块的第一次访问：miss"),
                                   (X_LINE + 250, FILL_GREEN, GREEN, "块内的其他访问：hit"),
                                   (X_LINE + 470, WHITE, LINE, "未被访问")):
        out.append(rect(x, ly, 22, 22, fill, stroke, rx=0, width=1.4))
        out.append(text(x + 30, ly + 17, label, 16, INK, "normal", "start"))
    return out


if __name__ == "__main__":
    save("friendly-miss-rate", W, H, figure())
