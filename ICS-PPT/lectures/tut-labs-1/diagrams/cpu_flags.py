#!/usr/bin/env python3
"""alu.c: the ten functions in their tiers, the four already written, and
the five flags they set.

Run it to refresh ../assets/cpu_flags.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE, arrow, cells,
                    elbow, rect, save, text)
from cpu_kit import GIVEN, TODO, code, note

W, H = 1120, 300
BW, BH = 140, 44


def fn(x, y, name, sub, kind, w=BW):
    fill, stroke = kind
    return [rect(x, y, w, BH, fill, stroke, rx=6, width=1.4),
            code(x + w / 2, y + 19, name, 14, INK, "bold", anchor="middle"),
            note(x + w / 2, y + 36, sub, 12, anchor="middle")]


def build():
    out = []
    # tier 0
    out += fn(20, 240, "x86_szp", "ZF SF PF，由结果设置", TODO, 200)
    # tier 1
    out += fn(20, 150, "x86_add", "已经写出：示例", GIVEN)
    out += fn(180, 150, "x86_sub", "CF：借位；OF：见右", TODO)
    out += fn(340, 150, "x86_logic", "清零 CF OF", TODO)
    for x in (90, 250, 410):
        out.append(elbow([(x, 194), (x, 220), (120, 220), (120, 236)], MUTED, 1.6, head=False))
    out.append(arrow(120, 218, 120, 236, MUTED, 1.6))
    # tier 2
    out += fn(20, 60, "x86_inc", "已经写出；CF 不变", GIVEN)
    out += fn(180, 60, "x86_dec", "x86_sub；CF 不变", TODO)
    out += fn(340, 60, "x86_neg", "x86_sub(0, v)", TODO)
    out.append(arrow(90, 104, 90, 146, MUTED, 1.6))
    out.append(arrow(250, 104, 250, 146, MUTED, 1.6))
    out.append(arrow(410, 104, 330, 146, MUTED, 1.6))
    # shift, mul, div
    out += fn(540, 60, "x86_shift", "shl 已写出；shr、sar 需要完成", TODO, 200)
    out += fn(540, 150, "x86_mul", "CF OF：乘积放不下", TODO, 200)
    out += fn(540, 240, "x86_div", "已经写出；不改标志位", GIVEN, 200)
    out.append(elbow([(540, 82), (500, 82), (500, 262), (480, 262)], MUTED, 1.6, head=False))
    out.append(elbow([(540, 172), (500, 172)], MUTED, 1.6, head=False))
    out.append(arrow(500, 262, 224, 262, MUTED, 1.6))
    out.append(note(362, 247, "ZF SF PF 都来自 x86_szp", 12, anchor="middle"))
    # the flags
    fx = 780
    out.append(text(fx, 36, "每个函数只改 *f 的这五位", 15, INK, "bold", anchor="start"))
    flags = [("CF", "0", "无符号结果超出 size 字节"), ("PF", "2", "低 8 位中 1 的个数为偶数"),
             ("ZF", "6", "低 size 字节为 0"), ("SF", "7", "结果的最高位"),
             ("OF", "11", "有符号结果超出 size 字节")]
    for k, (f, bit, what) in enumerate(flags):
        y = 54 + k * 30
        out.append(rect(fx, y, 44, 24, FILL_ORANGE, ORANGE, rx=3, width=1.2))
        out.append(code(fx + 22, y + 17, f, 14, INK, "bold", anchor="middle"))
        out.append(code(fx + 54, y + 17, f"位 {bit}", 13, MUTED))
        out.append(note(fx + 110, y + 17, what, 13, INK))
    out.append(note(fx, 226, "size 是操作数的字节数：1、2、4、8", 13))
    out.append(note(fx, 248, "辅助：trunc_val、sext、msb、parity、flag", 13))
    # legend
    out.append(rect(780, 272, 22, 16, *TODO, rx=3, width=1.2))
    out.append(text(808, 285, "需要完成", 13, INK, anchor="start"))
    out.append(rect(880, 272, 22, 16, *GIVEN, rx=3, width=1.2))
    out.append(text(908, 285, "已经写出的示例", 13, INK, anchor="start"))
    return out


if __name__ == "__main__":
    save("cpu_flags", W, H, build())
