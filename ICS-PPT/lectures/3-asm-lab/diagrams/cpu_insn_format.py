#!/usr/bin/env python3
"""The seven segments of an x86-64 instruction in the order they are read.

One box per segment, in the segment colours of this part: prefix, REX,
opcode, ModRM, SIB, displacement, immediate, each with its length in bytes.
The boxes are not to scale. Arrows above say which byte decides whether a
later segment is present and how long it is; brackets below say which part
of mini-cpu reads each segment: x86_begin up to the opcode, modrm(&x) in a
case the ModRM, SIB and displacement, fetch_imm and its kin the immediate.
Lengths after Intel SDM vol. 2, fig. 2-1 and section 2.2.1 (64-bit mode).
Run it to refresh ../assets/cpu_insn_format.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, MUTED, ORANGE, GREEN, brace, elbow, rect, save, text
from cpu_kit import DISP, IMM, MODRM, OPCODE, PREFIX, SIB

W, H = 1120, 164
BY, BH = 64, 58                     # the row of segments

SEGS = [("前缀", "0–4 字节", PREFIX, 150),
        ("REX", "0 或 1 字节", PREFIX, 120),
        ("操作码", "1–3 字节", OPCODE, 140),
        ("ModRM", "0 或 1 字节", MODRM, 120),
        ("SIB", "0 或 1 字节", SIB, 120),
        ("偏移", "0、1 或 4 字节", DISP, 150),
        ("立即数", "0、1、2、4 或 8 字节", IMM, 180)]

X0 = (W - sum(s[3] for s in SEGS)) / 2


def spans():
    x, out = X0, []
    for s in SEGS:
        out.append((x, x + s[3]))
        x += s[3]
    return out


def build():
    xs = spans()
    mid = [(a + b) / 2 for a, b in xs]
    out = []
    for (name, size, (fill, stroke), w), (x, _) in zip(SEGS, xs):
        out.append(rect(x, BY, w, BH, fill, stroke, rx=4, width=1.6))
        out.append(text(x + w / 2, BY + 25, name, 16, INK, "bold"))
        out.append(text(x + w / 2, BY + 45, size, 12.5, INK))
    out.append(text(X0, 26, "一条指令最长 15 字节，", 13, MUTED, anchor="start"))
    out.append(text(X0, 46, "除操作码外，每一段都可以没有", 13, MUTED, anchor="start"))

    # what decides the segments after the opcode
    lo, hi = BY - 22, 18
    out.append(elbow([(mid[2] + 20, BY - 3), (mid[2] + 20, lo), (mid[3], lo), (mid[3], BY - 4)],
                     ORANGE, 1.5))
    out.append(text((mid[2] + mid[3]) / 2 + 10, lo - 6, "有没有 ModRM", 12, ORANGE, "bold"))
    out.append(elbow([(mid[3] + 20, BY - 3), (mid[3] + 20, lo), (mid[4], lo), (mid[4], BY - 4)],
                     GREEN, 1.5))
    out.append(elbow([(mid[4], lo), (mid[5], lo), (mid[5], BY - 4)], GREEN, 1.5))
    out.append(text((mid[4] + mid[5]) / 2 + 10, lo - 6, "有没有 SIB，偏移几个字节", 12, GREEN, "bold"))
    out.append(elbow([(mid[2] - 20, BY - 3), (mid[2] - 20, hi), (mid[6], hi), (mid[6], BY - 4)],
                     ORANGE, 1.5))
    out.append(text((mid[4] + mid[5]) / 2 + 40, hi - 6, "有没有立即数、几个字节；部分操作码还看操作数宽度", 12,
                    ORANGE, "bold"))

    # who reads them in mini-cpu
    by = BY + BH + 8
    out += brace(xs[0][0] + 2, xs[2][1] - 2, by, INK, "x86_begin 读取，返回 op", 13.5)
    out += brace(xs[3][0] + 2, xs[5][1] - 2, by, INK, "case 中的 modrm(&x) 读取", 13.5)
    out += brace(xs[6][0] + 2, xs[6][1] - 2, by, INK, "case 中的 fetch_imm 等读取", 13.5)
    return out


if __name__ == "__main__":
    save("cpu_insn_format", W, H, build())
