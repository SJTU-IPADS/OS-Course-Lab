#!/usr/bin/env python3
"""Worked examples of modrm_decode on lines of objdump -d of the guests: the
bytes coloured by segment, then the fields of modrm_t they give.

  guest/hello  401000  48 8b 3c 24      mov (%rsp),%rdi
  guest/hello  401004  48 8d 74 24 08   lea 0x8(%rsp),%rsi
  guest/hello  401040  49 89 f9         mov %rdi,%r9

Run it to refresh ../assets/cpu_examples.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import GREEN, INK, LINE, MUTED, ORANGE, line, save, text
from cpu_kit import CH, DISP, MODRM, OPCODE, PREFIX, PURPLE, SIB, byte_row, code, note

W, H = 1120, 320

ONE = [
    ("mov (%rsp),%rdi", "hello 401000",
     [(["48"], PREFIX), (["8b"], OPCODE), (["3c"], MODRM), (["24"], SIB)],
     "3c = 00 111 100：mod 0，reg 7；rm 100 → SIB　24 = 00 100 100：index 100 → 没有变址，base 4",
     "mod 0  sub 7  reg 7  rm -1  base 4  index -1  scale 1  rip 0  dsize 0  disp 0   → 用掉 2 字节"),
    ("lea 0x8(%rsp),%rsi", "hello 401004",
     [(["48"], PREFIX), (["8d"], OPCODE), (["74"], MODRM), (["24"], SIB), (["08"], DISP)],
     "74 = 01 110 100：mod 1 → 1 字节偏移；reg 6；rm 100 → SIB 24，同上",
     "mod 1  sub 6  reg 6  rm -1  base 4  index -1  scale 1  rip 0  dsize 1  disp 8   → 用掉 3 字节"),
    ("mov %rdi,%r9", "hello 401040",
     [(["49"], PREFIX), (["89"], OPCODE), (["f9"], MODRM)],
     "REX 49：B = 1　f9 = 11 111 001：mod 3 → rm 是寄存器，001 + 8 = 9；reg 7",
     "mod 3  sub 7  reg 7  rm 9   base -1 index -1  scale 1  rip 0  dsize 0  disp 0   → 用掉 1 字节"),
]


def build(rows):
    out = []
    for k, (asm, where, groups, how, fields) in enumerate(rows):
        y = 16 + k * 100
        out.append(code(20, y + CH / 2 + 5, asm, 16, INK, "bold"))
        out.append(note(20, y + CH / 2 + 25, where, 12))
        shapes, right = byte_row(300, y, groups)
        out += shapes
        out.append(note(300, y + 56, how, 13, INK))
        out.append(code(300, y + 78, fields, 13, MUTED))
        if k < len(rows) - 1:
            out.append(line(20, y + 92, W - 20, y + 92, LINE, 1, "4 4"))
    return out


if __name__ == "__main__":
    save("cpu_examples", W, H, build(ONE))
