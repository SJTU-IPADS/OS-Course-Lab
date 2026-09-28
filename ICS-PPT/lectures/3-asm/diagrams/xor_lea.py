#!/usr/bin/env python3
"""x*5 as imull (3 cycles) against leal (%rax,%rax,4) (1 cycle) on one clock.

The figure of the integer multiply page. It once also drew the xor zero idiom
on its left, hence the file name.
Run it to refresh ../assets/xor-lea.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_ORANGE, GREEN, INK, LINE, MUTED, ORANGE,
                    line, mono, path, rect, save, text)

W, H = 1120, 262
X0, CW = 230, 290                       # clock origin, width of one cycle


def build():
    out = [mono(20, 30, "x * 5", 20, INK, "bold"),
           text(92, 30, "的两种写法", 18, MUTED, anchor="start")]
    # clock
    d = f"M {X0} 92"
    for k in range(3):
        a = X0 + k * CW
        d += f" L {a} 64 L {a + CW / 2} 64 L {a + CW / 2} 92 L {a + CW} 92"
    out.append(path(d, MUTED, 1.8, head=False))
    for k in range(3):
        out.append(text(X0 + k * CW + CW / 2, 54, f"周期 {k + 1}", 16, MUTED))
    for k in range(4):
        out.append(line(X0 + k * CW, 98, X0 + k * CW, 252, LINE, 1, "3 3"))
    out.append(mono(20, 142, "imull", 20, INK, "bold"))
    out.append(rect(X0, 114, 3 * CW, 42, FILL_ORANGE, ORANGE, rx=4, width=1.6))
    out.append(text(X0 + 1.5 * CW, 141, "乘法器：3 个周期", 17, INK))
    out.append(mono(20, 208, "leal", 20, INK, "bold"))
    out.append(mono(20, 232, "(%rax,%rax,4)", 15, MUTED))
    out.append(rect(X0, 184, CW, 42, FILL_GREEN, GREEN, rx=4, width=1.6))
    out.append(text(X0 + CW / 2, 211, "1 个周期", 17, INK))
    out.append(text(X0 + 1.5 * CW, 211, "x + x × 4", 17, MUTED))
    return out


if __name__ == "__main__":
    save("xor-lea", W, H, build())
