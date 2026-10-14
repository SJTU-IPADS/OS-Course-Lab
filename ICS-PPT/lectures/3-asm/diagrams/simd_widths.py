#!/usr/bin/env python3
"""x86 SIMD register widths by generation, one row each.

Bar length is the register width. MMX reuses the x87 registers; SSE adds
%xmm0-15; AVX/AVX2 widen them to %ymm0-15; AVX-512 has %zmm0-31 and mask
registers %k0-7. The rows are low: the figure shares its page with the
list of the four generations.
Run it to refresh ../assets/simd-widths.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import BLUE, FILL_BLUE, INK, MUTED, mono, rect, save, text

W, H = 1120, 176
X, U = 250, 1.1                      # bar start, pixels per bit
PITCH, BAR = 44, 34                  # row pitch, bar height

X86 = [("MMX", "1996", 64, "借用 x87 浮点寄存器，8/16 位整型"),
       ("SSE 系列", "1999–2006", 128, "%xmm0 ~ %xmm15"),
       ("AVX / AVX2", "2011–2013", 256, "%ymm0 ~ %ymm15"),
       ("AVX-512", "2016 至今", 512, "%zmm0 ~ %zmm31，掩码 %k0 ~ %k7")]


def row(y, name, when, bits, regs):
    w = bits * U
    mid = y + BAR / 2 + 6                # text baseline, centred on the bar
    out = [text(20, mid, name, 18, INK, "bold", anchor="start"),
           text(X - 16, mid, when, 14, MUTED, anchor="end"),
           rect(X, y, w, BAR, FILL_BLUE, BLUE, rx=3, width=1.6),
           text(X + w / 2, mid, f"{bits} 位", 16, INK, "bold")]
    if regs.startswith("%"):
        out.append(mono(X + w + 16, mid, regs, 16, INK))
    else:
        out.append(text(X + w + 16, mid, regs, 15, INK, anchor="start"))
    return out


def build():
    out = []
    for k, (name, when, bits, regs) in enumerate(X86):
        out += row(4 + k * PITCH, name, when, bits, regs)
    return out


if __name__ == "__main__":
    save("simd-widths", W, H, build(), left=4)    # centres the ink on the canvas
