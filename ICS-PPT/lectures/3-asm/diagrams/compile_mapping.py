#!/usr/bin/env python3
"""One C statement, its three instructions, and their twelve machine bytes.

Columns run C, assembly, machine code, the order the compiler produces them.
The byte stream is copied from `objdump -d dot.o` (offsets 0x23, 0x27, 0x2c of
the -Og build); each slice of it is one instruction, 4, 5 and 3 bytes long.
Run it to refresh ../assets/compile-mapping.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, GREEN, INK, MUTED,
                    ORANGE, arrow, cells, listing, mono, rect, save, text, WHITE,
                    LINE, line)

W, H = 1120, 410
CW, CH = 28, 36
AX, AW = 472, 262                   # assembly boxes
BX = 800                            # left edge of the sliced bytes

SLICES = [("0x23", ["42", "8b", "0c", "86"], "mov    (%rsi,%r8,4),%ecx", BLUE, FILL_BLUE),
          ("0x27", ["42", "0f", "af", "0c", "87"], "imul   (%rdi,%r8,4),%ecx",
           ORANGE, FILL_ORANGE),
          ("0x2c", ["41", "01", "c9"], "add    %ecx,%r9d", GREEN, FILL_GREEN)]

C_LOOP = ["for (int i = 0; i < n; i++) {",
          "    sum += w[i] * x[i];",
          "}"]


def build():
    out = [text(185, 36, "高级语言（C）", 19, INK, "bold"),
           text(AX + AW / 2 - 30, 36, "汇编指令（objdump -d）", 19, INK, "bold"),
           text(940, 36, "目标机器码（字节流）", 19, INK, "bold")]
    for x in (372, 760):
        out.append(line(x, 20, x, H - 50, LINE, 1.4, "6 5"))

    # left: the loop, with the statement washed
    out.append(rect(20, 180, 335, 104, WHITE, LINE, rx=6))
    out += listing(34, 190, C_LOOP, 16, 28, marks={1: FILL_ORANGE}, width=300)
    out.append(arrow(357, 234, 452, 234, INK, 2))
    out.append(text(404, 222, "编译", 15, MUTED))

    # right, top: the continuous stream
    x0 = 940 - 12 * CW / 2
    fills = [f for _, b, _, _, f in SLICES for _ in b]
    shapes, _ = cells(x0, 66, [v for _, b, _, _, _ in SLICES for v in b], CW, CH,
                      fills, MUTED, 15)
    out += shapes
    out.append(text(940, 128, "gcc -c 生成的 12 个字节", 15, MUTED))

    # one row per instruction: offset, assembly, its slice of the stream
    for k, (off, bytes_, asm, color, fill) in enumerate(SLICES):
        y = 170 + k * 66
        n = len(bytes_)
        out.append(mono(AX - 10, y + CH / 2 + 6, off, 15, MUTED, anchor="end"))
        out.append(rect(AX, y, AW, CH, fill, color, rx=4))
        out.append(mono(AX + 14, y + CH / 2 + 6, asm, 16, INK))
        out.append(arrow(AX + AW + 6, y + CH / 2, BX - 6, y + CH / 2, color, 2, both=True))
        shapes, right = cells(BX, y, bytes_, CW, CH, fill, color, 15)
        out += shapes
        out.append(text(right + 12, y + CH / 2 + 6, f"{n} 字节", 15, color, "bold",
                        anchor="start"))

    out.append(text(560, H - 20, "汇编指令与变长机器码逐条一一对应，汇编指令的含义将在后续介绍",
                    16, MUTED))
    return out


if __name__ == "__main__":
    save("compile-mapping", W, H, build())
