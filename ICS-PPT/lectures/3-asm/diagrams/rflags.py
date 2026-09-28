#!/usr/bin/env python3
"""The low twelve bits of RFLAGS, and how the four common flags are set.

The other names on the strip (PF, AF, IF) are the ones GDB prints in its
eflags line.
Run it to refresh ../assets/rflags.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE,
                    FILL_RED, GREEN, INK, LINE, MUTED, ORANGE, RED, WHITE, mono,
                    rect, save, text)

W, H = 1120, 370
X0, CW = 140, 70

# bit -> (name, highlight colour or None)
BITS = {11: ("OF", RED), 10: ("DF", None), 9: ("IF", None), 8: ("TF", None),
        7: ("SF", ORANGE), 6: ("ZF", BLUE), 5: ("", None), 4: ("AF", None),
        3: ("", None), 2: ("PF", None), 1: ("1", None), 0: ("CF", GREEN)}
FILLS = {RED: FILL_RED, ORANGE: FILL_ORANGE, BLUE: FILL_BLUE, GREEN: FILL_GREEN}

CARDS = [("ZF", BLUE, FILL_BLUE, ["结果 = 0 时置 1", "全部结果位取或非"]),
         ("SF", ORANGE, FILL_ORANGE, ["结果符号位为 1 时置 1", "等于结果最高位"]),
         ("CF", GREEN, FILL_GREEN, ["无符号进位 / 借位", "最高位送出的进位"]),
         ("OF", RED, FILL_RED, ["有符号补码溢出", "进入与离开最高位的进位不同"])]


def build():
    out = [text(60, 76, "RFLAGS", 18, INK, "bold", anchor="start")]
    for k, bit in enumerate(range(11, -1, -1)):
        name, color = BITS[bit]
        x = X0 + k * CW
        out.append(rect(x, 50, CW, 44, FILLS.get(color, WHITE if name else FILL_GREY),
                        color or LINE, rx=2, width=2 if color else 1.2))
        out.append(mono(x + CW / 2, 78, name, 17, color or MUTED,
                        "bold" if color else "normal", anchor="middle"))
        out.append(mono(x + CW / 2, 36, str(bit), 14, MUTED, anchor="middle"))
    out.append(text(X0 + 12 * CW + 16, 78, "…", 18, MUTED, anchor="start"))
    for k, (name, color, fill, lines) in enumerate(CARDS):
        x = 40 + k * 265
        out.append(rect(x, 150, 245, 150, fill, color, rx=8, width=1.6))
        out.append(mono(x + 20, 186, name, 22, color, "bold"))
        for i, s in enumerate(lines):
            out.append(text(x + 20, 226 + i * 34, s, 16, INK if i == 0 else MUTED,
                            "bold" if i == 0 else "normal", anchor="start"))
    out.append(text(560, 340, "每条算术 / 比较指令执行时，由 ALU 的状态输出同时写入这些触发器",
                    16, MUTED))
    return out


if __name__ == "__main__":
    save("rflags", W, H, build())
