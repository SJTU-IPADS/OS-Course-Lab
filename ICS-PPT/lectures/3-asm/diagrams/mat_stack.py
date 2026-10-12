#!/usr/bin/env python3
"""main's frame with a matrix in it, drawn twice.

mat-stack.svg is the frame of examples/mat_main.c, tensor-stack.svg the frame
of examples/tensor_main.c (gcc -Og -fcf-protection=none -fno-stack-protector):

  mat_main.c      subq $88, %rsp     W at 32..79, x at 16..31, y at 4..15
  tensor_main.c   subq $120, %rsp    W at 64..111, x at 48..63, y at 36..47,
                                     t at 0..23 (rows, cols, data)

Every row is 8 bytes and all rows have one height, high addresses at the top
and on the left, as in main-stack.svg: an int is 4 bytes, so a row holds two
array elements and addresses fall from left to right and from top to bottom.
Each cell carries its address as the listing writes it. The second figure adds
the struct at the bottom of the frame and the pointer in t.data, which holds
the address of W[0][0].

Local data is orange as in the other stack figures of the lecture; bytes the
function does not use are white with a dashed edge.
Run it to refresh ../assets/mat-stack.svg and ../assets/tensor-stack.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE, arrow, elbow, line,
                    mono, rect, save, text, vbrace)

W = 490                             # canvas of the frame with the pointer lane
W_PLAIN = 470                       # canvas of the frame without it
CX, CW = 126, 220                   # cell column: left edge, width
TOP, RH = 14, 40                    # top edge of the frame, one 8-byte row
LANE = W - 16                       # the pointer of t.data runs up here


def address(off):
    return f"{off}(%rsp)" if off else "(%rsp)"


def cell(x, y, w, off, label):
    """One variable or field: its address above its name. label None: unused."""
    used = label is not None
    out = [rect(x, y, w, RH, FILL_ORANGE if used else WHITE, ORANGE if used else LINE,
                rx=2, width=1.4 if used else 1.6, dash=None if used else "5 4")]
    if used:
        out.append(mono(x + w / 2, y + 15, address(off), 13, MUTED, anchor="middle"))
        out.append(mono(x + w / 2, y + 33, label, 16, INK, "bold", anchor="middle"))
    else:
        out.append(text(x + w / 2, y + RH / 2 + 5, "未使用", 14, MUTED))
    return out


def ints(base, name, count):
    """{offset: label} for an int array of `count` elements at `base`."""
    return {base + 4 * k: f"{name}[{k}]" for k in range(count)}


def matrix(base):
    return {base + 16 * r + 4 * j: f"W[{r}][{j}]" for r in range(3) for j in range(4)}


def build(size, ints4, quads, groups, pointer=None):
    """A frame of `size` bytes.

    ints4 maps the offset of a 4-byte cell to its label, quads the offset of an
    8-byte cell; whatever neither names is unused. groups are (low, high, label)
    spans bracketed on the right. pointer is (from offset, to offset): an arrow
    from the 8-byte cell at the first to the 4-byte cell at the second.
    """
    def row_y(off):
        return TOP + (size - 8 - off // 8 * 8) // 8 * RH

    out = []
    for base in range(size - 8, -8, -8):
        y = row_y(base)
        if base in quads:
            out += cell(CX, y, CW, base, quads[base])
        elif base in ints4 or base + 4 in ints4:
            # two 4-byte cells, the higher address on the left
            out += cell(CX, y, CW / 2, base + 4, ints4.get(base + 4))
            out += cell(CX + CW / 2, y, CW / 2, base, ints4.get(base))
        else:
            out += cell(CX, y, CW, base, None)
    bottom = TOP + size // 8 * RH
    # one bracket per variable
    for low, high, label in groups:
        y0, y1 = row_y(high - 1), row_y(low) + RH
        out += vbrace(CX + CW + 8, y0 + 3, y1 - 3, INK, label, 15)
    # the frame
    out += vbrace(CX - 10, TOP + 2, bottom - 2, MUTED, "", 15, right=False)
    mid = (TOP + bottom) / 2
    out.append(text(CX - 26, mid - 4, "main 的栈帧", 15, MUTED, "bold", anchor="end"))
    out.append(text(CX - 26, mid + 18, f"{size} 字节", 15, MUTED, "bold", anchor="end"))
    # direction of addresses
    out.append(arrow(14, bottom, 14, TOP + 4, LINE, 1.6))
    out.append(text(22, TOP + 14, "高地址", 13, MUTED, anchor="start"))
    out.append(text(22, bottom - 2, "低地址", 13, MUTED, anchor="start"))
    # %rsp is the address of the bottom right cell
    out.append(arrow(CX + CW + 62, bottom, CX + CW + 6, bottom, INK, 2.2))
    out.append(mono(CX + CW + 68, bottom + 5, "%rsp", 15, INK, "bold"))
    if pointer:
        src, dst = pointer
        y0, y1 = row_y(src) + RH / 2, row_y(dst) + RH / 2
        out.append(elbow([(CX + CW, y0), (LANE, y0), (LANE, y1), (CX + CW + 6, y1)],
                         INK, 2))
    return out, bottom + 22


if __name__ == "__main__":
    body, h = build(88, {**matrix(32), **ints(16, "x", 4), **ints(4, "y", 3)}, {},
                    [(32, 80, "W，48 字节"), (16, 32, "x，16 字节"), (4, 16, "y，12 字节")])
    save("mat-stack", W_PLAIN, h, body)

    body, h = build(120, {**matrix(64), **ints(48, "x", 4), **ints(36, "y", 3)},
                    {0: "t.rows = 3", 8: "t.cols = 4", 16: "t.data"},
                    [(64, 112, "W，48 字节"), (48, 64, "x，16 字节"),
                     (36, 48, "y，12 字节"), (0, 24, "t，24 字节")],
                    pointer=(16, 64))
    save("tensor-stack", W, h, body)
