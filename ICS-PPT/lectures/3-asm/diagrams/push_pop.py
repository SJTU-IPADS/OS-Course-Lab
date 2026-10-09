#!/usr/bin/env python3
"""pushq and popq, each as its two equivalent instructions.

High addresses are at the top. push first moves %rsp down by 8 and then
writes the new top slot; pop first reads the top slot and then moves %rsp up
by 8. The dashed arrow is %rsp before the step.
The figure sits under the two text columns of the page, the push half under
the pushq column and the pop half under the popq column. Each half names its
instruction in the free corner beside the S / D box.
Run it to refresh ../assets/push-pop.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_ORANGE, GREEN, INK, LINE, MONO, MUTED, ORANGE,
                    WHITE, arrow, box, line, mono, rect, save, text)

W, H = 1120, 226
CW, SH, Y0 = 140, 40, 26           # column width, slot height, top of column
BOX_Y, BOX_H = 182, 36             # the S / D box under a column
MID = [Y0 + SH * k + SH / 2 for k in range(3)]
EDGE = [Y0 + SH * (k + 1) for k in range(3)]  # low edge of each slot, where %rsp points


def column(x, top):
    """Two slots of older data and the slot at the top of the stack."""
    out = []
    for k in range(2):
        out.append(rect(x, Y0 + SH * k, CW, SH, FILL_ORANGE, ORANGE, rx=2, width=1.4))
        out.append(text(x + CW / 2, MID[k] + 5, "已有数据", 15, MUTED))
    y = Y0 + 2 * SH
    if top is None:
        out.append(rect(x, y, CW, SH, WHITE, LINE, rx=2, width=1.4, dash="5 4"))
    else:
        # the slot being written or read shares the colour of S / D
        label, color = top
        out.append(rect(x, y, CW, SH, FILL_GREEN, GREEN, rx=2, width=1.8))
        out.append(text(x + CW / 2, MID[2] + 5, label, 15, color, "bold"))
    return out


def rsp(x, row, old, sign):
    """%rsp now (solid) and before the step (dashed), with the move between."""
    out = [arrow(x + CW + 56, EDGE[old], x + CW + 6, EDGE[old], LINE, 1.8, dash="5 4"),
           arrow(x + CW + 56, EDGE[row], x + CW + 6, EDGE[row], INK, 2.4),
           mono(x + CW + 62, EDGE[row] + 6, "%rsp", 16, INK, "bold")]
    y0, y1 = EDGE[old], EDGE[row]
    step = 6 if y1 > y0 else -6
    out.append(arrow(x + CW + 34, y0 + step, x + CW + 34, y1 - step, INK, 1.8))
    out.append(mono(x + CW + 42, (y0 + y1) / 2 + 5, sign, 15, INK, "bold"))
    return out


def rsp_still(x, row):
    return [arrow(x + CW + 56, EDGE[row], x + CW + 6, EDGE[row], INK, 2.4),
            mono(x + CW + 62, EDGE[row] + 6, "%rsp", 16, INK, "bold")]


def step_title(x, s):
    return mono(x + CW / 2, 16, s, 15, INK, "bold", anchor="middle")


def half_title(x, name, order):
    """The instruction and its order of steps, in the free corner of a half."""
    return [mono(x + CW / 2, BOX_Y + 14, name, 20, INK, "bold", anchor="middle"),
            text(x + CW / 2, BOX_Y + 35, order, 15, MUTED)]


def push(x0):
    a, b = x0, x0 + 260
    out = half_title(a, "pushq S", "先移动指针，再写入")
    out += [step_title(a, "① subq $8, %rsp")] + column(a, None) + rsp(a, 2, 1, "−8")
    out += [step_title(b, "② movq S, (%rsp)")]
    out += column(b, ("S 的值", INK)) + rsp_still(b, 2)
    out += box(b + 20, BOX_Y, CW - 40, BOX_H, "S", FILL_GREEN, GREEN, 17, font=MONO)
    out.append(arrow(b + CW / 2, BOX_Y - 2, b + CW / 2, Y0 + 3 * SH + 4, GREEN, 2.4))
    out.append(text(b + CW / 2 + 8, BOX_Y - 11, "写入", 14, GREEN, "bold", anchor="start"))
    return out


def pop(x0):
    a, b = x0, x0 + 260
    out = half_title(b, "popq D", "先读出，再移动指针")
    out += [step_title(a, "① movq (%rsp), D")]
    out += column(a, ("栈顶数据", INK)) + rsp_still(a, 2)
    out += box(a + 20, BOX_Y, CW - 40, BOX_H, "D", FILL_GREEN, GREEN, 17, font=MONO)
    out.append(arrow(a + CW / 2, Y0 + 3 * SH + 2, a + CW / 2, BOX_Y - 4, GREEN, 2.4))
    out.append(text(a + CW / 2 + 8, BOX_Y - 14, "读出", 14, GREEN, "bold", anchor="start"))
    out += [step_title(b, "② addq $8, %rsp")] + column(b, None) + rsp(b, 1, 2, "+8")
    out.append(text(b + CW / 2, MID[2] + 5, "已释放", 15, MUTED))
    return out


def build():
    out = push(40) + pop(600)
    out.append(line(570, 4, 570, BOX_Y + BOX_H + 2, LINE, 1))
    return out


if __name__ == "__main__":
    save("push-pop", W, H, build(), left=10)    # centres the ink on the canvas
