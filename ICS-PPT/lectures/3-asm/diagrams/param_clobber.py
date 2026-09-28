#!/usr/bin/env python3
"""dot_sum's two calls to dot_product: the registers at four moments.

Four columns: entry to dot_sum, entry to the first dot_product, return from
it to dot_sum, entry to the second dot_product. Between the first two columns
dot_sum copies n into %edx (gcc -Og emits movl %ecx, %edx), which overwrites
y. The figure assumes dot_product leaves %rdi, %rsi, %rdx and %rcx alone, so
the one argument the second call lacks is y; the text slide says what happens
when the callee writes them too.
Run it to refresh ../assets/param-clobber.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_RED, FONT, GREEN,
                    INK, LINE, MONO, MUTED, ORANGE, RED, WHITE, arrow, label_line,
                    mono, path, rect, save, text)
from svgkit import _advance

W, H = 1120, 452
CW, GAP = 164, 108                  # column width, gap between columns
CX = [97 + k * (CW + GAP) for k in range(4)]
RH, RG = 54, 12                     # row height, gap between rows
TOP = 74
REGS = ["%rdi", "%rsi", "%rdx", "%rcx", "%rax"]

# cell kinds: (fill, stroke, dash, value tone, note tone)
KIND = {"arg": (FILL_BLUE, BLUE, None, INK, INK),
        "ret": (FILL_GREEN, GREEN, None, INK, GREEN),
        "lost": (FILL_RED, RED, None, RED, RED),
        "other": (FILL_GREY, LINE, None, MUTED, MUTED),
        "empty": (WHITE, LINE, "5 4", MUTED, MUTED)}

# per column: header parts (text, mono?), subtitle, cells as (value, kind, note)
COLS = [
    ([("① 进入 ", False), ("dot_sum", True)], "参数 w, x, y, n",
     [("w", "arg", ""), ("x", "arg", ""), ("y", "arg", ""), ("n", "arg", ""),
      ("—", "empty", "")]),
    ([("② 进入 ", False), ("dot_product", True)], "第一次调用：w, x, n",
     [("w", "arg", ""), ("x", "arg", ""), ("n", "lost", "覆盖了 y"),
      ("n", "other", ""), ("—", "empty", "")]),
    ([("③ 返回 ", False), ("dot_sum", True)], "第一次调用返回时",
     [("w", "other", ""), ("x", "other", ""), ("n", "other", ""), ("n", "other", ""),
      ("第一次结果", "ret", "返回值")]),
    ([("④ 进入 ", False), ("dot_product", True)], "第二次调用：w, y, n",
     [("w", "arg", ""), ("y", "lost", "已丢失"), ("n", "arg", ""), ("n", "other", ""),
      ("第一次结果", "other", "")]),
]

# what happens between two columns: (first line, mono?), second line
STEPS = [(("dot_sum", True), "准备参数"),
         (("dot_product", True), "执行并返回"),
         (("dot_sum", True), "准备参数")]

LEGEND = [("arg", "本次调用的参数"), ("ret", "返回值"), ("lost", "y 被覆盖而丢失"),
          ("other", "其他值")]

# the assumption the figure rests on, under the dot_product step
ASSUME = [("假定不改写", False), ("%rdi, %rsi,", True), ("%rdx, %rcx", True)]


def row_y(i):
    return TOP + i * (RH + RG)


def header(x, parts, sub):
    width = sum(_advance(s, 15, MONO if m else FONT) for s, m in parts)
    x0 = x + CW / 2 - width / 2
    out = label_line(x0, 28, [(s, MONO if m else FONT, INK, "bold") for s, m in parts], 15)
    out.append(text(x + CW / 2, 52, sub, 14, MUTED))
    return out


def cell(x, y, value, kind, note):
    fill, stroke, dash, tone, note_tone = KIND[kind]
    out = [rect(x, y, CW, RH, fill, stroke, rx=3, width=1.6, dash=dash)]
    font = MONO if value.isascii() else FONT
    vy = y + 23 if note else y + RH / 2 + 6
    out.append(text(x + CW / 2, vy, value, 16, tone, "bold", font=font))
    if note:
        out.append(text(x + CW / 2, y + 44, note, 13, note_tone, "bold"))
    return out


def step(k):
    """The action between column k and column k + 1."""
    x0, x1 = CX[k] + CW, CX[k + 1]
    cx = (x0 + x1) / 2
    (first, is_mono), second = STEPS[k]
    y = row_y(1) + 18
    out = [arrow(x0 + 14, 22, x1 - 14, 22, MUTED, 1.8),
           text(cx, y, first, 13 if is_mono else 14, INK, "bold",
                font=MONO if is_mono else FONT),
           text(cx, y + 20, second, 14, INK, "bold")]
    if k == 1:
        for i, (s, is_mono) in enumerate(ASSUME):
            out.append(text(cx, y + 56 + 20 * i, s, 13, ORANGE, "bold",
                            font=MONO if is_mono else FONT))
    return out


def copy_n():
    """dot_sum moves n from %ecx to %edx for the first call."""
    x0, x1 = CX[0] + CW, CX[1]
    ya, yb = row_y(3) + RH / 2, row_y(2) + RH / 2
    cx = (x0 + x1) / 2
    return [path(f"M {x0 + 4} {ya} C {cx + 10} {ya}, {cx - 10} {yb}, {x1 - 4} {yb}",
                 BLUE, 2.2)]


def legend(y):
    items = [(kind, s, 24 + 8 + _advance(s, 14, FONT)) for kind, s in LEGEND]
    gap = 26
    x = (W - sum(w for _, _, w in items) - gap * (len(items) - 1)) / 2
    out = []
    for kind, s, w in items:
        fill, stroke, dash, _, _ = KIND[kind]
        out.append(rect(x, y - 13, 24, 16, fill, stroke, rx=2, width=1.4, dash=dash))
        out.append(text(x + 32, y, s, 14, INK, anchor="start"))
        x += w + gap
    return out


def build():
    out = []
    for i, r in enumerate(REGS):
        out.append(mono(CX[0] - 12, row_y(i) + RH / 2 + 6, r, 17, INK, "bold", anchor="end"))
    for x, (parts, sub, cells) in zip(CX, COLS):
        out += header(x, parts, sub)
        for i, (value, kind, note) in enumerate(cells):
            out += cell(x, row_y(i), value, kind, note)
    for k in range(3):
        out += step(k)
    out += copy_n()
    out += legend(row_y(4) + RH + 42)
    return out


if __name__ == "__main__":
    save("param-clobber", W, H, build())
