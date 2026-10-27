#!/usr/bin/env python3
"""The encoding of an x86-64 arithmetic/logic instruction on two registers.

Counterpart of the Y86 box "opq rA, rB | 6 fn rA rB" (ICS1 lecture 14, page
20): one opcode byte, then the operand byte split 2 + 3 + 3 bits as lecture 3
taught it (mm, rrr, bbb; mm = 11 means B is a register). This is the form
without a REX prefix (both registers among %eax..%edi); the slide text says so.
The two bytes are drawn the same width, 8 bit cells each. Both bytes are what
fetch reads, so they take the fetch colour of timegrid.TONE.
Run it to refresh ../assets/op-encoding.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, FILL_BLUE, INK, MUTED, line, mono, rect, save, text

W, H = 400, 140
BIT, GAP, CH = 20, 12, 44     # one bit, between the bytes, cell height
BW = 8 * BIT                  # one byte
X0 = (W - 2 * BW - GAP) / 2
Y0 = 46


def build():
    out = [mono(W / 2, 28, "OPl R, B", 22, INK, "bold", anchor="middle")]
    out.append(rect(X0, Y0, BW, CH, FILL_BLUE, BLUE, rx=3, width=1.6))
    out.append(mono(X0 + BW / 2, Y0 + CH / 2 + 6, "op", 18, INK, "bold", anchor="middle"))
    out.append(text(X0 + BW / 2, Y0 + CH + 28, "操作码", 15, MUTED))
    x = X0 + BW + GAP
    out.append(rect(x, Y0, BW, CH, FILL_BLUE, BLUE, rx=3, width=1.6))
    out.append(text(x + BW / 2, Y0 + CH + 28, "操作数字节", 15, MUTED))
    for bits, name in ((2, "11"), (3, "rrr"), (3, "bbb")):
        if x > X0 + BW + GAP:
            out.append(line(x, Y0 + 6, x, Y0 + CH - 6, BLUE, 1.2, "3 3"))
        out.append(mono(x + bits * BIT / 2, Y0 + CH / 2 + 6, name, 18, INK, "bold",
                        anchor="middle"))
        x += bits * BIT
    return out


if __name__ == "__main__":
    save("op-encoding", W, H, build())
