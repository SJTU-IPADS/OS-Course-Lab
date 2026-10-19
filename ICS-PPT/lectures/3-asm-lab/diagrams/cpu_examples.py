#!/usr/bin/env python3
"""Worked examples of modrm_decode on lines of objdump -d of the guests: the
bytes coloured by segment, then the fields of modrm_t they give.

  guest/hello  401000  48 8b 3c 24      mov (%rsp),%rdi
  guest/hello  401004  48 8d 74 24 08   lea 0x8(%rsp),%rsi
  guest/hello  401040  49 89 f9         mov %rdi,%r9
  guest/tinytf 4010fb  8b 05 1f 1f 00 00   mov 0x1f1f(%rip),%eax
  guest/matmul 40122e  4c 03 04 d5 20 30 40 00   add 0x403020(,%rdx,8),%r8
  guest/block  401135  41 8d 44 24 ff   lea -0x1(%r12),%eax

Run it to refresh ../assets/cpu_examples-1.svg and cpu_examples-2.svg.
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
     "74 = 01 110 100：mod 1 → 1 字节位移；reg 6；rm 100 → SIB 24，同上",
     "mod 1  sub 6  reg 6  rm -1  base 4  index -1  scale 1  rip 0  dsize 1  disp 8   → 用掉 3 字节"),
    ("mov %rdi,%r9", "hello 401040",
     [(["49"], PREFIX), (["89"], OPCODE), (["f9"], MODRM)],
     "REX 49：B = 1　f9 = 11 111 001：mod 3 → rm 是寄存器，001 + 8 = 9；reg 7",
     "mod 3  sub 7  reg 7  rm 9   base -1 index -1  scale 1  rip 0  dsize 0  disp 0   → 用掉 1 字节"),
]

TWO = [
    ("mov 0x1f1f(%rip),%eax", "tinytf 4010fb",
     [(["8b"], OPCODE), (["05"], MODRM), (["1f", "1f", "00", "00"], DISP)],
     "05 = 00 000 101：mod 0 且 rm 101 → 没有基址，相对于 %rip，4 字节位移；reg 0",
     "mod 0  sub 0  reg 0  rm -1  base -1 index -1  scale 1  rip 1  dsize 4  disp 0x1f1f   → 用掉 5 字节"),
    ("add 0x403020(,%rdx,8),%r8", "matmul 40122e",
     [(["4c"], PREFIX), (["03"], OPCODE), (["04"], MODRM), (["d5"], SIB),
      (["20", "30", "40", "00"], DISP)],
     "REX 4c：R = 1　04：mod 0，reg 0 + 8，rm 100 → SIB　d5 = 11 010 101：scale 8，index 2，base 101 且 mod 0 → 没有基址",
     "mod 0  sub 0  reg 8  rm -1  base -1 index 2   scale 8  rip 0  dsize 4  disp 0x403020 → 用掉 6 字节"),
    ("lea -0x1(%r12),%eax", "block 401135",
     [(["41"], PREFIX), (["8d"], OPCODE), (["44"], MODRM), (["24"], SIB), (["ff"], DISP)],
     "REX 41：B = 1　44 = 01 000 100：mod 1，rm 100 → SIB　24：没有变址，base 100 + 8 = 12；位移 ff → -1",
     "mod 1  sub 0  reg 0  rm -1  base 12 index -1  scale 1  rip 0  dsize 1  disp -1   → 用掉 3 字节"),
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
    save("cpu_examples-1", W, H, build(ONE))
    save("cpu_examples-2", W, H, build(TWO))
