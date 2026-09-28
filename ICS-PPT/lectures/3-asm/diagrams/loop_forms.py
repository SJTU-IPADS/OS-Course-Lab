#!/usr/bin/env python3
"""The three machine-level loop shapes as control-flow graphs.

do-while tests at the bottom and translates a C do-while loop. The other two
translate an ordinary while loop, which also tests before the first pass:
jump-to-middle (what gcc -Og emits) enters with a jmp over the body to the
test, guarded-do (what gcc -O2 emits) tests once at the entry and then runs a
bottom-tested loop. Blocks name the instruction kinds, not one listing.
Run it to refresh ../assets/loop-forms.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MONO, MUTED, ORANGE, WHITE, arrow, elbow, mono,
                    path, rect, save, text)

W, H = 1120, 500
BW, BH = 220, 64


def block(x, y, lines, fill=FILL_BLUE, stroke=BLUE, h=BH):
    out = [rect(x, y, BW, h, fill, stroke, rx=6, width=1.6)]
    n = len(lines)
    for k, s in enumerate(lines):
        yy = y + h / 2 + (k - (n - 1) / 2) * 22 + 6
        font_mono = s.isascii()
        out.append(text(x + BW / 2, yy, s, 15 if font_mono else 16, INK,
                        "bold" if k == 0 and not font_mono else "normal",
                        font=MONO if font_mono else
                        "PingFang SC, Noto Sans CJK SC, Source Han Sans SC, sans-serif"))
    return out


def column(cx, title, sub):
    return [text(cx, 30, title, 19, INK, "bold"), text(cx, 56, sub, 16, MUTED)]


def do_while(cx):
    x = cx - BW / 2
    out = column(cx, "do-while", "翻译 do-while 循环")
    out += block(x, 90, ["循环体"], FILL_GREY, LINE)
    out += block(x, 230, ["cmp", "jcc 回到循环体"], FILL_ORANGE, ORANGE)
    out += block(x, 390, ["退出"], WHITE, LINE, 50)
    out.append(arrow(cx, 72, cx, 86, INK, 2))
    out.append(arrow(cx, 154, cx, 226, INK, 2))
    out.append(arrow(cx, 294, cx, 386, INK, 2))
    out.append(elbow([(x + BW, 262), (x + BW + 30, 262), (x + BW + 30, 122),
                      (x + BW + 4, 122)], ORANGE, 2.4))
    out.append(text(x + BW + 36, 196, "满足", 14, ORANGE, "bold", anchor="start"))
    return out


def jump_middle(cx):
    x = cx - BW / 2
    out = column(cx, "jump-to-middle", "翻译 while 循环，-Og 采用")
    out += block(x, 90, ["jmp 到判断"], WHITE, INK, 44)
    out += block(x, 170, ["循环体"], FILL_GREY, LINE)
    out += block(x, 290, ["判断", "cmp", "jcc 回到循环体"], FILL_ORANGE, ORANGE, 80)
    out += block(x, 420, ["退出"], WHITE, LINE, 50)
    out.append(arrow(cx, 72, cx, 86, INK, 2))
    # the entry jumps over the body to the test
    out.append(elbow([(x, 112), (x - 30, 112), (x - 30, 330), (x - 4, 330)], INK, 2.4))
    out.append(text(x - 36, 262, "入口跳到判断", 14, INK, "bold", anchor="end"))
    out.append(arrow(cx, 234, cx, 286, MUTED, 2))
    out.append(arrow(cx, 370, cx, 416, INK, 2))
    out.append(elbow([(x + BW, 330), (x + BW + 30, 330), (x + BW + 30, 202),
                      (x + BW + 4, 202)], ORANGE, 2.4))
    out.append(text(x + BW + 36, 270, "满足", 14, ORANGE, "bold", anchor="start"))
    return out


def guarded(cx):
    x = cx - BW / 2
    out = column(cx, "guarded-do", "翻译 while 循环，-O2 采用")
    out += block(x, 90, ["cmp", "jcc 跳过循环"], FILL_GREEN, GREEN)
    out += block(x, 200, ["循环体"], FILL_GREY, LINE)
    out += block(x, 310, ["cmp", "jcc 回到循环体"], FILL_ORANGE, ORANGE)
    out += block(x, 420, ["退出"], WHITE, LINE, 50)
    out.append(arrow(cx, 72, cx, 86, INK, 2))
    out.append(arrow(cx, 154, cx, 196, INK, 2))
    out.append(text(cx + 8, 180, "满足", 14, MUTED, anchor="start"))
    out.append(arrow(cx, 264, cx, 306, MUTED, 2))
    out.append(arrow(cx, 374, cx, 416, INK, 2))
    out.append(elbow([(x + BW, 342), (x + BW + 26, 342), (x + BW + 26, 232),
                      (x + BW + 4, 232)], ORANGE, 2.4))
    out.append(text(x + BW + 30, 292, "满足", 14, ORANGE, "bold", anchor="start"))
    # the guard skips the loop entirely
    out.append(elbow([(x, 122), (x - 26, 122), (x - 26, 445), (x - 4, 445)], GREEN, 2.4))
    out.append(text(x - 32, 290, "不满足", 14, GREEN, "bold", anchor="end"))
    return out


def build():
    return do_while(170) + jump_middle(555) + guarded(935)


if __name__ == "__main__":
    save("loop-forms", W, H, build())
