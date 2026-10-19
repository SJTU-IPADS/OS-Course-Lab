#!/usr/bin/env python3
"""CF and OF on three 8-bit additions: the carry out of the top bit, and the
sign of the sum against the signs of the addends.

Run it to refresh ../assets/cpu_cf_of.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREY, FILL_ORANGE, INK, LINE, MUTED, ORANGE, RED, WHITE, cells, line,
                    rect, save, text)
from cpu_kit import code, note

W, H = 1120, 300
PW = 350                            # one panel
BW, BH = 22, 26                     # one bit

CASES = [
    ("0xff + 0x01 = 0x00", 0xff, 0x01, "CF = 1，OF = 0",
     ["无符号：255 + 1 = 256，8 位放不下", "有符号：-1 + 1 = 0，正确"]),
    ("0x7f + 0x01 = 0x80", 0x7f, 0x01, "CF = 0，OF = 1",
     ["无符号：127 + 1 = 128，正确", "有符号：127 + 1 得 -128，错误"]),
    ("0x80 + 0x80 = 0x00", 0x80, 0x80, "CF = 1，OF = 1",
     ["无符号：128 + 128 = 256，放不下", "有符号：-128 + -128 得 0，错误"]),
]


def bits(v):
    return [str((v >> i) & 1) for i in range(7, -1, -1)]


def panel(x0, title, a, b, verdict, lines):
    out = [rect(x0, 4, PW, 292, WHITE, LINE, rx=8, width=1.2),
           code(x0 + 16, 30, title, 16, INK, "bold")]
    r = (a + b) & 0xff
    carry = (a + b) >> 8
    bx = x0 + 70
    for k, (v, label) in enumerate(((a, "a"), (b, "b"))):
        y = 48 + k * 32
        out.append(code(x0 + 16, y + 18, label, 14, MUTED))
        fills = [FILL_ORANGE] + [WHITE] * 7
        out += cells(bx, y, bits(v), BW, BH, fills, MUTED, 14)[0]
    y = 112
    out.append(line(bx - 30, y + 2, bx + 8 * BW, y + 2, INK, 1.4))
    out.append(code(x0 + 16, y + 22, "r", 14, MUTED))
    fills = [FILL_ORANGE] + [WHITE] * 7
    out += cells(bx, y + 6, bits(r), BW, BH, fills, MUTED, 14)[0]
    # the carry out
    out += cells(bx - 30, y + 6, [str(carry)], BW, BH, [FILL_GREY], ORANGE if carry else MUTED, 14)[0]
    out.append(note(bx - 19, y + 50, "进位", 12, ORANGE if carry else MUTED, anchor="middle"))
    out.append(note(bx + BW / 2, y + 50, "最高位", 12, ORANGE, anchor="middle"))
    out.append(text(x0 + 16, 192, verdict, 15, ORANGE, "bold", anchor="start"))
    for k, s in enumerate(lines):
        out.append(note(x0 + 16, 218 + k * 22, s, 13, INK))
    out.append(note(x0 + 16, 276, "r 的最高位与 a、b 的最高位相比" if a == b else
                    ("a、b 最高位相同，r 的不同 → OF" if (a ^ b) & 0x80 == 0 else
                     "a、b 最高位不同 → OF 为 0"), 12))
    return out


def build():
    out = []
    for k, (title, a, b, verdict, lines) in enumerate(CASES):
        out += panel(20 + k * (PW + 15), title, a, b, verdict, lines)
    return out


if __name__ == "__main__":
    save("cpu_cf_of", W, H, build())
