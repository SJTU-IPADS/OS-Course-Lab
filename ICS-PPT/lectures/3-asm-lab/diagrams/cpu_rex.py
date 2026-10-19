#!/usr/bin/env python3
"""The REX prefix: W, R, X and B, and the field each bit extends. Two
instructions: 4c 03 04 d5 .. (REX.R extends reg to %r8) and
41 8d 44 24 ff  lea -0x1(%r12),%eax (guest/block, 401135: REX.B extends the
base of the SIB byte to %r12; rm 100 still means a SIB byte).

Run it to refresh ../assets/cpu_rex.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import GREEN, INK, LINE, MUTED, WHITE, elbow, line, rect, save, text
from cpu_kit import (BASE, DISP, INDEX, MOD, MODRM, OPCODE, PREFIX, PURPLE, REG, RM, SCALE,
                     SIB, bit_fields, byte_row, code, note)

W, H = 1120, 330
BW = 26


def build():
    out = [text(20, 28, "REX 前缀", 16, MUTED, "bold", anchor="start"),
           note(110, 28, "0100 W R X B；x86_begin 把它记在 x.rex 中，modrm_decode 的参数 rex 是它（没有时为 0）")]
    # the REX byte 4c, bit by bit
    shapes, rx = bit_fields(20, 44, [("0100", PREFIX, ""), ("1", PREFIX, "W"), ("1", PREFIX, "R"),
                                     ("0", PREFIX, "X"), ("0", PREFIX, "B")], labels=True)
    out += shapes
    out.append(code(rx + 14, 65, "= 0x4c", 15, MUTED))
    # ModRM and SIB of the same instruction below, with arrows from R, X, B
    y = 150
    out.append(text(20, y + 20, "ModRM 04", 14, GREEN, "bold", anchor="start"))
    shapes, mx = bit_fields(110, y, [("00", MOD, "mod"), ("000", REG, "reg"), ("100", RM, "rm")])
    out += shapes
    out.append(text(mx + 30, y + 20, "SIB d5", 14, PURPLE, "bold", anchor="start"))
    shapes, sx = bit_fields(mx + 100, y, [("11", SCALE, "scale"), ("010", INDEX, "index"),
                                          ("101", BASE, "base")])
    out += shapes
    # R -> reg, X -> index, B -> rm and base
    xr = 20 + 5 * BW + BW / 2
    xx = 20 + 6 * BW + BW / 2
    xb = 20 + 7 * BW + BW / 2
    out.append(elbow([(xr, 96), (xr, 120), (110 + 2 * BW + 1.5 * BW, 120), (110 + 3.5 * BW, y - 4)],
                     GREEN, 1.8))
    out.append(elbow([(xx, 96), (xx, 128), (mx + 100 + 2 * BW + 1.5 * BW, 128),
                      (mx + 100 + 3.5 * BW, y - 4)], PURPLE, 1.8))
    out.append(elbow([(xb, 96), (xb, 136), (110 + 5 * BW + 1.5 * BW, 136), (110 + 6.5 * BW, y - 4)],
                     GREEN, 1.8))
    out.append(elbow([(xb, 136), (mx + 100 + 5 * BW + 1.5 * BW, 136), (mx + 100 + 6.5 * BW, y - 4)],
                     PURPLE, 1.8, head=True))
    X = 640
    out.append(text(X, 70, "W：操作数 64 位，x.size = 8", 14, INK, anchor="start"))
    out.append(text(X, 96, "R：reg 的第 3 位　　000 + 8 = 8，即 %r8", 14, INK, anchor="start"))
    out.append(text(X, 122, "X：index 的第 3 位", 14, INK, anchor="start"))
    out.append(text(X, 148, "B：rm 的第 3 位（mod 11），或 base 的第 3 位", 14, INK, anchor="start"))
    out.append(text(X, 184, "两个例外只看 3 位的字段，先于 REX.B：", 14, INK, "bold", anchor="start"))
    out.append(text(X, 208, "rm 100 仍然带 SIB 字节；rm 101 且 mod 00 仍然是 %rip 相对", 14, INK,
                    anchor="start"))
    # the second example
    out.append(rect(20, 232, 1080, 84, WHITE, LINE, rx=8, width=1.2, dash="5 4"))
    out.append(code(36, 258, "41 8d 44 24 ff   lea -0x1(%r12),%eax", 15, INK))
    out.append(note(400, 258, "guest/block，地址 401135"))
    out.append(text(36, 284, "REX 41：B = 1　　ModRM 44 = 01 000 100：rm 100 → 读 SIB　　SIB 24 = 00 100 100：index 100 → 没有变址，"
                    "base 100 + 8 = 12，即 %r12", 14, INK, anchor="start"))
    out.append(text(36, 306, "mod 01 → 1 字节位移 ff，符号扩展为 -1；%r12 作基址需要 SIB 字节，因为 rm 字段 100 已经另有含义",
                    14, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("cpu_rex", W, H, build())
