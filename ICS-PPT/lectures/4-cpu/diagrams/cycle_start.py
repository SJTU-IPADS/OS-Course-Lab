#!/usr/bin/env python3
"""Four instructions, one per clock cycle, at the beginning of cycle 3.

After "周期 3 开始时" of ICS1 lecture 14 (page 36), with the Y86 program
replaced by x86-64 (gcc -c, objdump -d):
    0x000: bb 00 01 00 00   movl $0x100, %ebx
    0x005: ba 00 02 00 00   movl $0x200, %edx
    0x00a: 01 d3            addl %edx, %ebx
    0x00c: 74 ..            je   dest        (ZF = 0, not taken)
The clock edge that starts cycle 3 is drawn in thick ink, as in clock.py.
Rows 1 and 2 are done (grey), row 3 starts now (ink frame), row 4 is to come.
Run it to refresh ../assets/cycle-start.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import FILL_GREY, INK, LINE, MUTED, WHITE, line, mono, path, rect, save, text

W, H = 1120, 330
X0, CYC = 220, 190            # first clock edge, one cycle
HI, LO = 50, 80               # waveform levels
ROW0, RH, PITCH = 116, 44, 52
NOW = 2                       # the edge that starts cycle 3
ROWS = [
    "0x000: movl $0x100, %ebx   # %ebx <- 0x100",
    "0x005: movl $0x200, %edx   # %edx <- 0x200",
    "0x00a: addl %edx, %ebx     # %ebx <- 0x300, CF ZF SF OF <- 0 0 0 0",
    "0x00c: je   dest           # not taken",
]


def build():
    out = [text(X0 - 20, (HI + LO) / 2 + 6, "时钟", 17, INK, anchor="end")]
    edges = [X0 + k * CYC for k in range(5)]
    for e in edges:
        out.append(line(e, HI - 18, e, LO + 8, LINE, 1.2, "4 3"))
    d = f"M {X0 - 10} {LO} "
    for e in edges[:4]:
        d += f"L {e} {LO} L {e} {HI} L {e + CYC / 2} {HI} L {e + CYC / 2} {LO} "
    d += f"L {edges[4]} {LO} L {edges[4]} {HI} L {edges[4] + 20} {HI}"
    out.append(path(d, INK, 2, head=False))
    for k in range(4):
        out.append(text(edges[k] + CYC / 2, HI - 22, f"周期 {k + 1}", 15,
                        INK if k == NOW else MUTED, "bold" if k == NOW else "normal"))
    out.append(line(edges[NOW], HI - 34, edges[NOW], LO + 14, INK, 3))
    for k, s in enumerate(ROWS):
        y = ROW0 + k * PITCH
        if k < NOW:
            fill, stroke, ink, weight, width = FILL_GREY, LINE, INK, "normal", 1.4
        elif k == NOW:
            fill, stroke, ink, weight, width = WHITE, INK, INK, "bold", 2.4
        else:
            fill, stroke, ink, weight, width = WHITE, LINE, MUTED, "normal", 1.4
        out.append(text(X0 - 20, y + RH / 2 + 6, f"周期 {k + 1}", 15, MUTED, anchor="end"))
        out.append(rect(X0, y, 4 * CYC + 40, RH, fill, stroke, rx=4, width=width))
        out.append(mono(X0 + 16, y + RH / 2 + 6, s, 17, ink, weight))
    return out


if __name__ == "__main__":
    save("cycle-start", W, H, build())
