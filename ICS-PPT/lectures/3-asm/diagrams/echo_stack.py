#!/usr/bin/env python3
"""The stack of echo.c while gets writes into buf, drawn in four frames.

Offsets are echo.s's (gcc -Og -fcf-protection=none -fno-stack-protector):
after `pushq %rbx` and `subq $16, %rsp`, 8 unused bytes at 0..7, buf[0..7] at
8..15, the saved %rbx at 16..23. The return address pushed by main's call is
at 24..31 and closes main's frame.

Every row is 8 bytes, high addresses at the top and on the left, as in
main-stack.svg. gets writes one byte per character and one for the
terminator, upward from buf[0]:

  frame 1   before gets writes: buf is 8 empty cells
  frame 2   7 characters, 8 bytes: buf is full
  frame 3   15 characters, 16 bytes: buf[8..15] lie on the saved %rbx
  frame 4   23 characters, 24 bytes: buf[16..23] lie on the return address

A row gets has written past the end of buf is drawn as 8 byte cells labelled
with their index in buf, the way slides 5 to 8 of refs/ppts/12-pointer.pptx
draw it. The arrow on the right runs from buf[0] to the last byte written and
grows from frame to frame; the line beside its tip names the input and what
it overwrote. The four frames share one canvas, so the stack stays in place
from page to page.

Colours as in the other stack figures: a local array orange, the saved
register green, the return address blue. A write past the end of buf is red.
The figure has no title line: the page title names it.
Run it to refresh ../assets/echo-stack-{1,2,3,4}.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, FILL_RED, FONT, GREEN,
                    INK, LINE, MONO, MUTED, ORANGE, RED, WHITE, _advance, arrow,
                    label_line, line, mono, rect, save, text, vbrace)

W, H = 1120, 262
CX, CELL = 300, 48                  # cell column: left edge, width of one byte
CW = 8 * CELL
TOP, DATA, RH = 10, 48, 48          # top edge, main's other data, one 8-byte row
PX = CX + CW                        # right edge of the column
WX = PX + 150                       # the arrow of the writes
BOTTOM = TOP + DATA + 4 * RH        # bottom edge of the column
LEFT = -43                          # canvas shift, see svgkit.svg

# the 8-byte rows below main's other data, top to bottom:
# offset from %rsp, label parts, fill, stroke
ROWS = [(24, [("返回地址", FONT)], FILL_BLUE, BLUE),
        (16, [("保存的 ", FONT), ("%rbx", MONO)], FILL_GREEN, GREEN),
        (8, None, FILL_ORANGE, ORANGE),                 # buf
        (0, [("未使用", FONT)], WHITE, LINE)]

# what each frame after the first adds: bytes written, the two caption lines
STEPS = [(8, "7 个字符，写入 8 字节", [("填满 ", FONT), ("buf", MONO)]),
         (16, "15 个字符，写入 16 字节", [("覆盖保存的 ", FONT), ("%rbx", MONO)]),
         (24, "23 个字符，写入 24 字节", [("覆盖返回地址", FONT)])]


def row_top(off):
    return TOP + DATA + (24 - off) // 8 * RH


def centred(y, parts, fill, weight="bold", size=16):
    """A label of CJK and mono pieces, centred in the cell column."""
    width = sum(_advance(s, size, f) for s, f in parts)
    return label_line(CX + (CW - width) / 2, y, [(s, f, fill, weight) for s, f in parts],
                      size)


def byte_cells(y, first, fill, stroke, colour, weight):
    """One row as 8 bytes of buf: buf[first + 7] on the left, buf[first] on the right."""
    out = []
    for j in range(8):
        x = CX + j * CELL
        out.append(rect(x, y, CELL, RH, fill, stroke, rx=2, width=1.6))
        out.append(mono(x + CELL / 2, y + RH / 2 + 5, f"[{first + 7 - j}]", 15, colour,
                        weight, anchor="middle"))
    return out


def stack(rows, buf, written):
    """The column, its addresses, the two frames and the direction of addresses.

    rows are the 8-byte rows under main's other data, buf is the offset of buf
    from %rsp, written the number of bytes gets has written from buf[0] on.
    """
    out = [rect(CX, TOP, CW, DATA, FILL_ORANGE, ORANGE, rx=2, width=1.4)]
    out.append(text(CX + CW / 2, TOP + DATA / 2 + 6, "main 的其他数据", 16, MUTED))
    for off, parts, fill, stroke in rows:
        y = row_top(off)
        index = off - buf                       # index in buf of the row's lowest byte
        if parts is None:                       # buf: empty before gets writes it
            out += (byte_cells(y, 0, fill, stroke, INK, "bold") if written
                    else byte_cells(y, 0, WHITE, stroke, MUTED, "normal"))
        elif 0 < index < written:               # written past the end of buf
            out += byte_cells(y, index, FILL_RED, RED, RED, "bold")
        else:
            dash = "5 4" if stroke == LINE else None
            out.append(rect(CX, y, CW, RH, fill, stroke, rx=2, width=1.6, dash=dash))
            out += centred(y + RH / 2 + 6, parts, MUTED if dash else INK,
                           "normal" if dash else "bold")
        out.append(mono(CX - 12, y + RH / 2 + 5, f"{off}(%rsp)" if off else "(%rsp)", 15,
                        INK, anchor="end"))
    # the two frames; the return address closes main's
    edge = row_top(24) + RH
    out += vbrace(CX - 96, TOP + 2, edge - 2, MUTED, "main 的栈帧", 16, right=False)
    out += vbrace(CX - 96, edge + 2, BOTTOM - 2, INK, "echo 的栈帧", 16, right=False)
    # direction of addresses
    out.append(arrow(24, BOTTOM, 24, TOP + 6, LINE, 1.6))
    out.append(text(34, TOP + 16, "高地址", 14, MUTED, anchor="start"))
    out.append(text(34, BOTTOM - 2, "低地址", 14, MUTED, anchor="start"))
    return out


def pointer(y, name, length=62):
    """A name for the lowest byte of the row above y, the byte on the right."""
    return [arrow(PX + length, y, PX + 5, y, INK, 2.2),
            mono(PX + length + 8, y + 5, name, 16, INK, "bold")]


def writes(base, tip, tone):
    """The arrow of the writes of gets, from buf[0] at base up to tip."""
    return [arrow(WX, base, WX, tip, tone, 2.6),
            line(PX + 5, tip, WX - 8, tip, tone, 1.4, dash="4 4")]


def caption(tip, first, second, tone, weight="bold"):
    """Two lines beside the arrow, under the edge the writes reach."""
    return ([text(WX + 16, tip + 20, first, 15, tone, weight, anchor="start")]
            + label_line(WX + 16, tip + 40, [(s, f, tone, weight) for s, f in second], 15))


def build(frame):
    written = STEPS[frame - 2][0] if frame > 1 else 0
    out = stack(ROWS, 8, written)
    base = row_top(8) + RH
    out += pointer(base, "buf") + pointer(BOTTOM, "%rsp")
    for k, (count, first, second) in enumerate(STEPS[:max(frame - 1, 0)]):
        tip = base - count // 8 * RH
        tone = RED if count > 8 else INK
        last = k == frame - 2
        if last:
            out += writes(base, tip, tone)
        out += caption(tip, first, second, tone, "bold" if last else "normal")
    return out


if __name__ == "__main__":
    for frame in (1, 2, 3, 4):
        # one shift for all four: it centres the ink of the last frame
        save(f"echo-stack-{frame}", W, H, build(frame), left=LEFT)
