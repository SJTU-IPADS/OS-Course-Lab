#!/usr/bin/env python3
"""All 16 general registers with their 32, 16 and 8-bit names, to scale.

Each row is a 64-bit register drawn with its narrower names nested inside it,
right-aligned on bit 0. The bit numbers along the top mark the slice edges.
The gap after %rsp separates the 8 registers of IA32 from the 8 that x86-64 added.
Run it to refresh ../assets/register-table.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, LINE, MUTED, WHITE, brace, mono, rect, save

W, H = 1120, 574
X0, X1 = 4, 1116                         # bit 63 .. bit 0
PX = (X1 - X0) / 64
TOP, RH, GAP = 50, 32, 10

OLD = [("rax", "eax", "ax", "al", "ah"), ("rbx", "ebx", "bx", "bl", "bh"),
       ("rcx", "ecx", "cx", "cl", "ch"), ("rdx", "edx", "dx", "dl", "dh"),
       ("rsi", "esi", "si", "sil", None), ("rdi", "edi", "di", "dil", None),
       ("rbp", "ebp", "bp", "bpl", None), ("rsp", "esp", "sp", "spl", None)]
NEW = [(f"r{k}", f"r{k}d", f"r{k}w", f"r{k}b", None) for k in range(8, 16)]


def bx(bit):
    """x of the left edge of a slice whose top bit is `bit`."""
    return X1 - (bit + 1) * PX


def row(y, names):
    q, l, w, b, h = names
    out = []
    for k, bit in enumerate([63, 31, 15, 7]):
        inset = k * 1.5
        out.append(rect(bx(bit), y + inset, X1 - bx(bit), RH - 4 - 2 * inset,
                        WHITE, LINE, rx=2, width=1.2))
    labels = [(63, 31, q), (31, 15, l), (15, 7, h or w), (7, -1, b)]
    for hi, lo, name in labels:
        cx = (bx(hi) + bx(lo)) / 2 if lo >= 0 else (bx(7) + X1) / 2
        out.append(mono(cx, y + RH / 2 + 4, "%" + name, 16, INK,
                        "bold" if hi == 63 else "normal", anchor="middle"))
    return out


def build():
    out = []
    for bit in [63, 31, 15, 7]:
        out.append(mono(bx(bit), 16, str(bit), 15, MUTED))
    out.append(mono(X1, 16, "0", 15, MUTED, anchor="end"))
    # %ah names bits 15..8, so %ax is marked once, over the first row
    out += brace(bx(15) + 2, X1 - 2, TOP - 2, MUTED, "", depth=6, below=False)
    out.append(mono((bx(15) + X1) / 2, TOP - 13, "%ax", 15, MUTED, anchor="middle"))
    for k, names in enumerate(OLD):
        out += row(TOP + k * RH, names)
    y2 = TOP + 8 * RH + GAP
    for k, names in enumerate(NEW):
        out += row(y2 + k * RH, names)
    return out


if __name__ == "__main__":
    save("register-table", W, H, build())
