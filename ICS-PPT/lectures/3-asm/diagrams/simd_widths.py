#!/usr/bin/env python3
"""x86 SIMD register widths by generation, with ARM NEON and SVE below.

Bar length is the register width. MMX reuses the x87 registers; SSE adds
%xmm0-15; AVX/AVX2 widen them to %ymm0-15; AVX-512 has %zmm0-31 and mask
registers %k0-7. NEON is fixed at 128 bits; SVE's length is chosen by the
implementation, drawn dashed.
Run it to refresh ../assets/simd-widths.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, GREEN, INK, LINE,
                    MUTED, WHITE, line, mono, rect, save, text)

W, H = 1120, 470
X, U = 250, 1.1                      # bar start, pixels per bit

X86 = [("MMX", "1996", 64, "借用 x87 浮点寄存器，8/16 位整型"),
       ("SSE 系列", "1999–2006", 128, "%xmm0 ~ %xmm15"),
       ("AVX / AVX2", "2011–2013", 256, "%ymm0 ~ %ymm15"),
       ("AVX-512", "2016 至今", 512, "%zmm0 ~ %zmm31，掩码 %k0 ~ %k7")]


def row(y, name, when, bits, regs, fill, stroke, dash=None):
    w = bits * U
    out = [text(20, y + 22, name, 18, INK, "bold", anchor="start"),
           text(20, y + 44, when, 14, MUTED, anchor="start"),
           rect(X, y + 4, w, 42, fill, stroke, rx=3, width=1.6, dash=dash),
           text(X + w / 2, y + 31, f"{bits} 位", 16, INK, "bold")]
    font_mono = regs.startswith("%")
    if font_mono:
        out.append(mono(X + w + 16, y + 31, regs, 16, INK))
    else:
        out.append(text(X + w + 16, y + 31, regs, 15, INK, anchor="start"))
    return out


def build():
    out = [text(20, 24, "x86 SIMD 扩展", 16, MUTED, "bold", anchor="start")]
    for k, (name, when, bits, regs) in enumerate(X86):
        out += row(40 + k * 62, name, when, bits, regs, FILL_BLUE, BLUE)
    out.append(line(20, 300, 1100, 300, LINE, 1, "4 4"))
    out.append(text(20, 328, "ARM", 16, MUTED, "bold", anchor="start"))
    out += row(340, "ARM NEON", "固定宽度", 128, "移动端处理器与 Apple Silicon",
               FILL_GREEN, GREEN)
    out.append(text(20, 424, "ARM SVE / SVE2", 18, INK, "bold", anchor="start"))
    out.append(text(20, 446, "可伸缩向量", 14, MUTED, anchor="start"))
    out.append(rect(X, 406, 128 * U, 42, FILL_GREEN, GREEN, rx=3, width=1.6))
    out.append(rect(X + 128 * U, 406, 384 * U, 42, WHITE, GREEN, rx=3, width=1.6,
                    dash="6 4"))
    out.append(text(X + 256 * U, 433, "长度由硬件实现决定，程序中不写死", 15, GREEN, "bold"))
    return out


if __name__ == "__main__":
    save("simd-widths", W, H, build())
