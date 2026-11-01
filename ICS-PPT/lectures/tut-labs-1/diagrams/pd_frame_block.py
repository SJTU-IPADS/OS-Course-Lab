#!/usr/bin/env python3
"""The 18 bytes of one Q4_0 block, as the scalar matvec_q4_0 reads them.

Bytes 0 and 1 are the scale d (fp16, gold, as in lecture 2's figures).
Byte 2 + j holds two 4-bit codes: the high 4 bits are the code of weight
j + 16 (outlined blue), the low 4 bits the code of weight j (filled blue),
the colours every figure of this part gives the two halves of a block.
Run it to refresh ../assets/pd-frame-block.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, INK, MONO, MUTED, WHITE, brace, rect,
                    save, text)

W, H = 1120, 150
GOLD, FILL_GOLD = "#b8860b", "#fbf2dc"          # the scale d, as in lecture 2

CW, CH = 56, 48                                 # one byte
X0, Y0 = (W - 18 * CW) / 2, 30                  # the block


def build():
    out = [text(X0 - 10, Y0 - 9, "字节", 12, MUTED, anchor="end")]
    for i in range(18):
        x = X0 + i * CW
        out.append(text(x + CW / 2, Y0 - 9, str(i), 12, MUTED, font=MONO))
        if i < 2:
            out.append(rect(x, Y0, CW, CH, FILL_GOLD, GOLD, rx=3, width=1.3))
            out.append(text(x + CW / 2, Y0 + CH / 2 + 5, "d", 15, GOLD, "bold", font=MONO))
            continue
        j = i - 2
        out.append(rect(x, Y0, CW / 2, CH, WHITE, BLUE, rx=0, width=1))
        out.append(rect(x + CW / 2, Y0, CW / 2, CH, FILL_BLUE, BLUE, rx=0, width=1))
        out.append(rect(x, Y0, CW, CH, "none", BLUE, rx=3, width=1.3))
        out.append(text(x + CW / 4, Y0 + CH / 2 + 4, f"q{j + 16}", 11.5, INK, font=MONO))
        out.append(text(x + 3 * CW / 4, Y0 + CH / 2 + 4, f"q{j}", 11.5, INK, font=MONO))
    out += brace(X0, X0 + 2 * CW, Y0 + CH + 6, GOLD, "d：fp16，低字节在前", 13.5)
    out += brace(X0 + 2 * CW, X0 + 18 * CW, Y0 + CH + 6, BLUE,
                 "blk[2 + j]，j = 0 .. 15：高 4 位是 q[j + 16]（空心），低 4 位是 q[j]（实心）", 13.5)
    out.append(text(W / 2, Y0 + CH + 62, "编码 q 取 0 .. 15，第 i 个权重 = d · (q[i] − 8)；q4_0_scale(blk) 读出 d",
                    15, INK, "bold"))
    return out


if __name__ == "__main__":
    save("pd-frame-block", W, H, build())
