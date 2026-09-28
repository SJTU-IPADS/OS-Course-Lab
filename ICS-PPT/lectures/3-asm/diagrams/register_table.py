#!/usr/bin/env python3
"""All 16 general registers with their 32, 16 and 8-bit names, to scale.

Each row is a 64-bit register drawn with its narrower names nested inside it,
right-aligned on bit 0. The fill marks the System V role named on page 13:
stack pointer, return value, the six argument registers.
Run it to refresh ../assets/register-table.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, brace, mono, rect, save, text,
                    vbrace)

W, H = 1120, 568
X0, X1 = 190, 910                        # bit 63 .. bit 0
PX = (X1 - X0) / 64
TOP, RH, GAP = 72, 30, 12

OLD = [("rax", "eax", "ax", "al", "ah"), ("rbx", "ebx", "bx", "bl", "bh"),
       ("rcx", "ecx", "cx", "cl", "ch"), ("rdx", "edx", "dx", "dl", "dh"),
       ("rsi", "esi", "si", "sil", None), ("rdi", "edi", "di", "dil", None),
       ("rbp", "ebp", "bp", "bpl", None), ("rsp", "esp", "sp", "spl", None)]
NEW = [(f"r{k}", f"r{k}d", f"r{k}w", f"r{k}b", None) for k in range(8, 16)]

ROLE = {"rsp": ("栈指针", FILL_ORANGE, ORANGE), "rax": ("返回值", FILL_GREEN, GREEN),
        "rdi": ("参数 1", FILL_BLUE, BLUE), "rsi": ("参数 2", FILL_BLUE, BLUE),
        "rdx": ("参数 3", FILL_BLUE, BLUE), "rcx": ("参数 4", FILL_BLUE, BLUE),
        "r8": ("参数 5", FILL_BLUE, BLUE), "r9": ("参数 6", FILL_BLUE, BLUE)}


def bx(bit):
    """x of the left edge of a slice whose top bit is `bit`."""
    return X1 - (bit + 1) * PX


def row(y, names):
    q, l, w, b, h = names
    role, fill, stroke = ROLE.get(q, ("", WHITE, LINE))
    out = []
    for k, (bit, name) in enumerate([(63, q), (31, l), (15, w), (7, b)]):
        inset = k * 1.5
        out.append(rect(bx(bit), y + inset, X1 - bx(bit), RH - 4 - 2 * inset,
                        fill if k == 0 else WHITE if k % 2 else fill, stroke,
                        rx=2, width=1.2))
    labels = [(63, 31, q), (31, 15, l), (15, 7, h or w), (7, -1, b)]
    for hi, lo, name in labels:
        cx = (bx(hi) + bx(lo)) / 2 if lo >= 0 else (bx(7) + X1) / 2
        out.append(mono(cx, y + RH / 2 + 3, "%" + name, 14, INK,
                        "bold" if hi == 63 else "normal", anchor="middle"))
    if role:
        out.append(text(X1 + 24, y + RH / 2 + 4, role, 15, stroke, "bold",
                        anchor="start"))
    return out


def build():
    out = []
    for bit, label in [(63, "63"), (31, "31"), (15, "15"), (7, "7")]:
        out.append(mono(bx(bit), 44, label, 13, MUTED))
    out.append(mono(X1, 44, "0", 13, MUTED, anchor="end"))
    # %ah names bits 15..8, so %ax is marked once, over the first row
    out += brace(bx(15) + 2, X1 - 2, TOP - 2, MUTED, "", depth=6, below=False)
    out.append(mono((bx(15) + X1) / 2, TOP - 12, "%ax", 13, MUTED, anchor="middle"))
    for hi, lo, s in [(63, 31, "64 位"), (31, 15, "32 位"), (15, 7, "16 位"),
                      (7, -1, "8 位")]:
        out.append(text(bx(hi) + 6, 22, s, 14, INK, "bold", anchor="start"))
    for k, names in enumerate(OLD):
        out += row(TOP + k * RH, names)
    y2 = TOP + 8 * RH + GAP
    for k, names in enumerate(NEW):
        out += row(y2 + k * RH, names)
    out += vbrace(X0 - 12, TOP, TOP + 8 * RH - 4, MUTED, "", depth=-8)
    out.append(text(20, TOP + 4 * RH - 6, "前 8 个", 16, INK, "bold", anchor="start"))
    out.append(text(20, TOP + 4 * RH + 16, "8086/IA32 演进", 14, MUTED, anchor="start"))
    out += vbrace(X0 - 12, y2, y2 + 8 * RH - 4, MUTED, "", depth=-8)
    out.append(text(20, y2 + 4 * RH - 6, "后 8 个", 16, INK, "bold", anchor="start"))
    out.append(text(20, y2 + 4 * RH + 16, "x86-64 新增", 14, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("register-table", W, H, build())
