#!/usr/bin/env python3
"""The stack while dot_product runs on behalf of dot_bias, high addresses up.

dot_bias (gcc -Og -fcf-protection=none) is pushq %rbx / movl %ecx, %ebx /
call dot_product / addl %ebx, %eax / popq %rbx / ret. When dot_product
executes, three 8-byte slots lie below the data of dot_bias's caller: the
return address into that caller, the saved %rbx, and the return address into
dot_bias. A return address closes the frame of the function that executed the
call, so dot_bias's frame is the lower two slots, 16 bytes. Each slot is
labelled on the left with the instruction that wrote it.

Colours as in the other stack figures: stack data orange, a return address
blue (an address in the code segment), a saved register green. The caller's
data is drawn tall and the three slots equal and small, since each is 8 bytes.
Tall and narrow, for the side column. Run it to refresh ../assets/bias-stack.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, FONT, GREEN, INK, LINE,
                    MONO, MUTED, ORANGE, arrow, label_line, mono, rect, save, text,
                    vbrace)

W, H = 580, 400
CX, CW = 176, 230                   # cell column: left edge, width
TOP, DATA, SLOT = 62, 150, 44       # top edge, the caller's data, one 8-byte slot

# label parts, fill, stroke, the instruction that wrote the slot
SLOTS = [([("返回地址", FONT), ("（回到调用者）", FONT)], FILL_BLUE, BLUE, "调用者的 call"),
         ([("保存的 ", FONT), ("%rbx", MONO)], FILL_GREEN, GREEN, "pushq %rbx"),
         ([("返回地址", FONT), ("（回到 ", FONT), ("dot_bias", MONO), ("）", FONT)],
          FILL_BLUE, BLUE, "call dot_product")]


def centred(y, parts, size=16):
    """A label of CJK and mono pieces, centred in the cell column."""
    from svgkit import _advance
    width = sum(_advance(s, size, f) for s, f in parts)
    return label_line(CX + (CW - width) / 2, y, [(s, f, INK, "bold") for s, f in parts],
                      size)


def build():
    out = [text(W / 2, 30, "dot_product 执行期间的栈（高地址在上）", 18, INK, "bold")]
    out.append(rect(CX, TOP, CW, DATA, FILL_ORANGE, ORANGE, rx=2, width=1.4))
    out.append(text(CX + CW / 2, TOP + DATA / 2 + 6, "调用者的其他数据", 16, MUTED))
    y = TOP + DATA
    edges = [y]
    for parts, fill, stroke, wrote in SLOTS:
        out.append(rect(CX, y, CW, SLOT, fill, stroke, rx=2, width=1.6))
        out += centred(y + SLOT / 2 + 6, parts)
        if wrote.startswith("调用者"):
            out += label_line(CX - 110, y + SLOT / 2 + 5,
                              [("调用者的 ", FONT, MUTED, "normal"),
                               ("call", MONO, MUTED, "normal")], 14)
        else:
            out.append(mono(CX - 12, y + SLOT / 2 + 5, wrote, 14, INK, anchor="end"))
        y += SLOT
        edges.append(y)
    # the two frames
    out += vbrace(CX + CW + 10, TOP + 2, edges[1] - 2, MUTED, "", 16)
    out.append(text(CX + CW + 28, (TOP + edges[1]) / 2 + 6, "调用者的栈帧", 16, MUTED,
                    "bold", anchor="start"))
    out += vbrace(CX + CW + 10, edges[1] + 2, edges[3] - 2, INK, "", 16)
    out += label_line(CX + CW + 28, (edges[1] + edges[3]) / 2 - 4,
                      [("dot_bias", MONO, INK, "bold"), (" 的栈帧", FONT, INK, "bold")], 16)
    out.append(text(CX + CW + 28, (edges[1] + edges[3]) / 2 + 20, "16 字节", 16, INK,
                    "bold", anchor="start"))
    # %rsp while dot_product runs: it pushes nothing and allocates nothing
    out.append(arrow(W - 60, edges[3], CX + CW + 4, edges[3], INK, 2.2))
    out.append(mono(W - 52, edges[3] + 6, "%rsp", 16, INK, "bold"))
    out += label_line(CX, edges[3] + 32, [("dot_product", MONO, INK, "normal"),
                                          (" 没有在栈上存放数据", FONT, INK, "normal")], 15)
    # direction of addresses
    out.append(arrow(16, edges[3], 16, TOP + 6, LINE, 1.6))
    out.append(text(26, TOP + 16, "高地址", 14, MUTED, anchor="start"))
    out.append(text(26, edges[3] + 24, "低地址", 14, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("bias-stack", W, H, build())
