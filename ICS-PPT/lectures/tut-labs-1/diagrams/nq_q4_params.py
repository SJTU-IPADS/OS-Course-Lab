#!/usr/bin/env python3
"""The two parameters of q4_0_quantize laid over the Q4_0 block of lecture 2.

Top, `const float *x`: the 32 floats of one block, one cell each. Bottom,
`uint8_t *blk`: the 18 bytes of the block as lecture 2's gguf_q4_blocks.py
draws them, d in orange and qs in blue, with byte indexes above. One qs byte,
blk[2], is blown up into its two nibbles exactly as in that figure, and two
arrows show which floats land in it: x[0] in the low 4 bits, x[16] in the
high 4 bits. Run it to refresh ../assets/nq_q4_params.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MONO, MUTED,
                    ORANGE, arrow, line, mono, rect, save, text)

W, H = 1120, 336
XX, XY, XC, XH = 140, 46, 30, 34         # x: 32 cells
BX, BY, BC, BH = 260, 226, 40, 36        # blk: 18 cells
ZX, ZY, ZW, ZH = 430, 112, 160, 54       # the zoomed byte: two nibbles
FOCUS = 2                                 # blk[2] = qs[0]


def build():
    out = [text(XX, XY - 12, "x：一块的 32 个 float，x[0] … x[31]（第二讲：一个块覆盖 32 个权重）",
                13.5, INK, "bold", anchor="start"),
           mono(XX - 10, XY + XH / 2 + 5, "const float *x", 14, INK, "bold", "end")]
    for i in range(32):
        out.append(rect(XX + i * XC, XY, XC, XH, FILL_GREY, LINE, rx=2, width=1.0))
        out.append(mono(XX + i * XC + XC / 2, XY + XH / 2 + 4, str(i), 10.5, MUTED, "normal",
                        "middle"))

    out.append(mono(BX - 10, BY + BH / 2 + 5, "uint8_t *blk", 14, INK, "bold", "end"))
    for i in range(18):
        x = BX + i * BC
        d = i < 2
        out.append(rect(x, BY, BC, BH, FILL_ORANGE if d else FILL_BLUE,
                        ORANGE if d else BLUE, rx=3, width=2.2 if i == FOCUS else 1.3))
        out.append(mono(x + BC / 2, BY + BH / 2 + 4.5, "d" if d else "qs", 11.5, MUTED,
                        "normal", "middle"))
        out.append(mono(x + BC / 2, BY - 8, str(i), 10.5, MUTED, "normal", "middle"))
    out.append(text(BX - 10, BY - 8, "下标", 10.5, MUTED, anchor="end"))
    out.append(text(BX + 18 * BC + 12, BY + BH / 2 + 5, "合计 18 字节", 12.5, INK, "bold",
                    anchor="start"))
    out.append(text(BX, BY + BH + 18, "d：fp16，低字节在 blk[0]", 12.5, ORANGE, "bold",
                    anchor="start"))
    out.append(text(BX + 10 * BC, BY + BH + 18, "qs：16 个字节，blk[2] … blk[17]", 12.5, BLUE,
                    "bold"))

    # blk[2] blown up into its two nibbles
    fx = BX + FOCUS * BC
    out.append(line(fx, BY, ZX, ZY + ZH, LINE, 1.1, dash="4 4"))
    out.append(line(fx + BC, BY, ZX + 2 * ZW, ZY + ZH, LINE, 1.1, dash="4 4"))
    out.append(rect(ZX, ZY, ZW, ZH, FILL_BLUE, BLUE, rx=3, width=1.6))
    out.append(rect(ZX + ZW, ZY, ZW, ZH, FILL_ORANGE, ORANGE, rx=3, width=1.6))
    out.append(mono(ZX + ZW / 2, ZY + 23, "q[16]", 15, INK, "bold", "middle"))
    out.append(mono(ZX + 1.5 * ZW, ZY + 23, "q[0]", 15, INK, "bold", "middle"))
    out.append(text(ZX + ZW / 2, ZY + 43, "高 4 位（位 7..4）← x[16]", 12, BLUE, "bold"))
    out.append(text(ZX + 1.5 * ZW, ZY + 43, "低 4 位（位 3..0）← x[0]", 12, ORANGE, "bold"))
    out.append(arrow(XX + XC / 2, XY + XH + 2, ZX + 1.5 * ZW, ZY - 2, ORANGE, 1.6, dash="5 4"))
    out.append(arrow(XX + 16 * XC + XC / 2, XY + XH + 2, ZX + ZW / 2, ZY - 2, BLUE, 1.6,
                     dash="5 4"))

    out.append(text(W / 2, H - 14, "第 j 个编码字节 blk[2 + j]：低 4 位存 q[j]，高 4 位存 q[j + 16]，"
                    "j = 0..15；编码 q 取 0..15", 13.5, INK, "bold"))
    return out


if __name__ == "__main__":
    save("nq_q4_params", W, H, build())
