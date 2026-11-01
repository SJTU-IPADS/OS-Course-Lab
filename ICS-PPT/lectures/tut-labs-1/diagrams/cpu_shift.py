#!/usr/bin/env python3
"""Shifts on one 8-bit value: the bit shifted out is CF; sar fills the top
with the sign bit. On the right, the two halves of a product and the two
halves of a dividend, as x86_mul and x86_div see them.

Run it to refresh ../assets/cpu_shift.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE,
                    arrow, box, cells, line, rect, save, text)
from cpu_kit import code, note

W, H = 1120, 320
BW, BH = 22, 26


def bits(v):
    return [str((v >> i) & 1) for i in range(7, -1, -1)]


def shift_row(y, name, before, after, cf, of, how, given=False):
    out = [code(20, y + 18, name, 15, MUTED if given else INK, "bold")]
    out += cells(130, y, bits(before), BW, BH, [WHITE] * 8, MUTED, 14)[0]
    out.append(arrow(130 + 8 * BW + 6, y + BH / 2, 130 + 8 * BW + 34, y + BH / 2, INK, 1.8))
    fills = [FILL_ORANGE] + [WHITE] * 7
    out += cells(350, y, bits(after), BW, BH, fills, MUTED, 14)[0]
    cx = 350 + 8 * BW + 10
    out += cells(cx, y, [str(cf)], BW, BH, [FILL_ORANGE if cf else FILL_GREY], ORANGE, 14)[0]
    out.append(note(cx + BW + 8, y + 18, f"CF 是移出的那一位；OF = {of}", 13, INK))
    out.append(note(130, y + 42, how, 12))
    return out


def build():
    out = [text(20, 24, "x86_shift，size 1，移动 1 位", 15, INK, "bold", anchor="start"),
           note(290, 24, "sub：4 shl、5 shr、6 sal（同 shl）、7 sar")]
    out += shift_row(44, "shl 0x81", 0x81, 0x02, 1, 1, "已经写出：左移，低位补 0；OF = 结果最高位与 CF 不同", True)
    out += shift_row(114, "shr 0x81", 0x81, 0x40, 1, 1, "右移，高位补 0；CF = v 的第 n-1 位；OF = v 的最高位")
    out += shift_row(184, "sar 0x81", 0x81, 0xc0, 1, 0, "右移，高位补 v 的最高位（sext）；CF 同 shr；OF = 0")
    out.append(note(130, 252, "ZF、SF、PF 由结果经 x86_szp 设置；最高位着色的一格是 SF", 13))
    out.append(note(130, 274, "位数先取低 5 位（size 8 取低 6 位）；位数为 0 时不改标志位", 13))
    # mul and div
    x = 760
    out.append(line(x - 20, 10, x - 20, H - 10, LINE, 1.2, "5 4"))
    out.append(text(x, 24, "x86_mul 与 x86_div", 15, INK, "bold", anchor="start"))
    out += box(x, 40, 100, 36, "a", FILL_BLUE, BLUE, 14)
    out += box(x + 120, 40, 100, 36, "b", FILL_BLUE, BLUE, 14)
    out.append(note(x + 110, 64, "×", 16, INK, anchor="middle"))
    out.append(arrow(x + 110, 80, x + 110, 100, INK, 1.8))
    out += box(x, 104, 110, 36, "*hi", FILL_ORANGE, ORANGE, 14)
    out += box(x + 110, 104, 110, 36, "*lo", FILL_ORANGE, ORANGE, 14)
    out.append(note(x + 230, 128, "乘积 2 × size 字节", 13, INK))
    out.append(note(x, 160, "mul：无符号（trunc_val）；imul：有符号（sext）", 13, INK))
    out.append(note(x, 180, "CF = OF = 乘积超出 size 字节；ZF SF PF 由 *lo 设置", 13, INK))
    out.append(note(x, 200, "size 8 时用 unsigned __int128、__int128", 13))
    out += box(x, 222, 110, 36, "hi", FILL_BLUE, BLUE, 14)
    out += box(x + 110, 222, 110, 36, "lo", FILL_BLUE, BLUE, 14)
    out.append(note(x + 230, 246, "÷ v → *q、*r", 13, INK))
    out.append(note(x, 280, "x86_div 已经写出：除数 0 或商放不下时返回 -1，", 13, INK))
    out.append(note(x, 300, "框架报告 divide error；不改标志位", 13, INK))
    return out


if __name__ == "__main__":
    save("cpu_shift", W, H, build())
