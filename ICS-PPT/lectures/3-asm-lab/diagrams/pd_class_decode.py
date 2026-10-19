#!/usr/bin/env python3
"""One Q4_0 block into the registers, three frames: the 16 code bytes
loaded into a 128-bit register; the low and high codes separated; four
groups of 8 codes widened, offset by 8 and converted to 8 floats each,
paired with the 8 floats of x they multiply.

The 18 bytes of the block are drawn above the register so that the 16 code
bytes stand directly over the lanes they load into. Codes of weights 0..15
are filled blue, codes of weights 16..31 outlined blue, as in the Q4_0 row
figure; the scale d is gold. In the last frame each group of 8 codes takes
its own colour, and the %ymm it becomes is drawn below it in the same
colour, joined to it by two dashed lines: the first 8 are widened where
they are, the last 8 after a shift right by 8 bytes. A %ymm holds 32 bytes
against the 16 of the %xmm; it is drawn longer than the %xmm, though not
at scale. From the first frame a box at the top right magnifies the byte
qs0 into its two 4-bit codes, q16-4 high and q0-4 low. A code carries its
width as a suffix: q0-4 in the byte, q0-8 in lo and hi of the last frame,
q0-32 in a %ymm.
Run it to refresh ../assets/pd-class-decode-{1,2,3}.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, INK, MONO, MONO_EM, MUTED, RED, WHITE, _marker,
                    arrow, elbow, line, rect, save, text)
from cpu_kit import FILL_PURPLE, FILL_TEAL, PURPLE, TEAL

W, H = 1120, 452
GOLD, FILL_GOLD = "#b8860b", "#fbf2dc"

CX, CW, CH = 200, 36, 32               # the 16 lanes of the 128-bit register
AY, BY, CY = 42, 114, 180              # the block, the %xmm, lo
LY = 242                               # the two %ymm made from lo
DY = 310                               # hi
HY = 372                               # the two %ymm made from hi
YX, YW, GAP = 40, 64, 16               # the %ymm: x, lane width, gap between the two of a row

# the four groups: (stroke, fill) of lo 0..7, lo 8..15, hi 0..7, hi 8..15;
# hi stays outlined, as everywhere in this part
GROUPS = [(BLUE, FILL_BLUE), (PURPLE, FILL_PURPLE), (TEAL, WHITE), (RED, WHITE)]


def strip(y, labels, fills, strokes, size=10, cw=CW, x=CX):
    out = []
    for i, s in enumerate(labels):
        cx = x + i * cw
        out.append(rect(cx, y, cw, CH, fills[i], strokes[i], rx=3, width=1.3))
        fit = min(size, (cw - 5) / (len(s) * MONO_EM))      # qs10 .. qs15 in a narrow cell
        out.append(text(cx + cw / 2, y + CH / 2 + 4.5, s, fit, INK, font=MONO))
    return out


def label(y, head, sub, sub2=None, color=INK):
    out = [text(20, y + 14, head, 14, color, "bold", anchor="start"),
           text(20, y + 30, sub, 11.5, MUTED, anchor="start")]
    if sub2:
        out.append(text(20, y + 46, sub2, 11.5, MUTED, anchor="start"))
    return out


def frame1():
    out = []
    # the block in memory: 18 bytes, the 16 code bytes over the lanes
    out += label(AY, "块 b（内存）", "18 字节")
    out += strip(AY, ["d", "d"] + [f"qs{i}" for i in range(16)],
                 [FILL_GOLD] * 2 + [FILL_BLUE] * 16, [GOLD] * 2 + [BLUE] * 16, x=CX - 2 * CW)
    out.append(text(CX - CW, AY - 8, "缩放系数", 11.5, GOLD, "bold"))
    out.append(text(CX + 8 * CW, AY - 8, "16 个编码字节", 11.5, BLUE, "bold"))
    # d read on the side
    out.append(rect(880, BY - 2, 220, 36, FILL_GOLD, GOLD, rx=4, width=1.4))
    out.append(text(990, BY + 14, "d = q4_0_scale(blk)", 12.5, INK, "bold", font=MONO))
    out.append(text(990, BY + 29, "一个 float，第 5 步乘以它", 11, MUTED))
    # the 128-bit load
    out += label(BY, "%xmm：16 个编码字节", "_mm_loadu_si128(blk+2)")
    out += strip(BY, [f"qs{i}" for i in range(16)], [FILL_BLUE] * 16, [BLUE] * 16)
    for i in (0, 7, 15):
        x = CX + i * CW + CW / 2
        out.append(arrow(x, AY + CH + 2, x, BY - 4, BLUE, 1.2))
    # qs0 magnified: one byte, two 4-bit codes
    zx, zy, zw, zh = 800, 6, 300, 86
    out.append(rect(CX, AY, CW, CH, "none", BLUE, rx=3, width=2.4))
    out.append(elbow([(CX + CW / 2, AY - 2), (CX + CW / 2, 18), (zx - 3, 18)], BLUE, 1.3, dash="4 3"))
    out.append(rect(zx, zy, zw, zh, WHITE, BLUE, rx=6, width=1.4))
    out.append(text(zx + 16, zy + 20, "qs0 放大：一个字节，两个 4 位编码", 12.5, INK, "bold", anchor="start"))
    for k, (name, fill, bits) in enumerate([("q16-4", WHITE, "高 4 位"), ("q0-4", FILL_BLUE, "低 4 位")]):
        x = zx + 40 + k * 110
        out.append(rect(x, zy + 30, 110, 28, fill, BLUE, rx=3, width=1.4))
        out.append(text(x + 55, zy + 49, name, 13, INK, "bold", font=MONO))
        out.append(text(x + 55, zy + 74, bits, 11, MUTED))
    return out


def frame2(grouped=False, suffix=""):
    out = frame1()
    if grouped:
        strokes = [GROUPS[i // 8][0] for i in range(16)], [GROUPS[2 + i // 8][0] for i in range(16)]
        fills = [GROUPS[i // 8][1] for i in range(16)], [GROUPS[2 + i // 8][1] for i in range(16)]
    else:
        strokes = [BLUE] * 16, [BLUE] * 16
        fills = [FILL_BLUE] * 16, [WHITE] * 16
    out += label(CY, "lo = 字节 & 0x0f", "_mm_and_si128，权重 0..15")
    out += strip(CY, [f"q{i}{suffix}" for i in range(16)], fills[0], strokes[0])
    out += label(DY, "hi = 字节 >> 4 & 0x0f", "srli_epi16 4 位，再 & 0x0f")
    out += strip(DY, [f"q{i + 16}{suffix}" for i in range(16)], fills[1], strokes[1])
    for i in (0, 7, 15):
        x = CX + i * CW + CW / 2
        out.append(arrow(x, BY + CH + 2, x, CY - 4, BLUE, 1.2))
    return out


def frame3():
    out = frame2(grouped=True, suffix="-8")
    # what a %ymm holds, in the room right of lo and hi
    rx = CX + 16 * CW + 16
    out.append(text(rx, CY + 12, "%ymm：每 8 个编码成为 8 个 float", 13, INK, "bold", anchor="start"))
    out.append(text(rx, CY + 27, "cvtepu8_epi32 扩展为 int32，减 8，cvtepi32_ps 转为 float",
                    10.5, MUTED, anchor="start"))
    out.append(text(rx, DY + 12, "%ymm：hi 的两组同样处理", 13, INK, "bold", anchor="start"))
    out.append(text(rx, DY + 27, "后缀是位数：q0-4、q0-8、q0-32 分别占 4、8、32 位", 11, MUTED, anchor="start"))
    for g, (stroke, fill) in enumerate(GROUPS):
        src_y, y = (CY, LY) if g < 2 else (DY, HY)
        half = g % 2
        sx = CX + half * 8 * CW                    # the 8 codes in lo or hi
        gx = YX + half * (8 * YW + GAP)            # the %ymm they become
        out += strip(y, [f"q{8 * g + k}-32" for k in range(8)], [fill] * 8, [stroke] * 8, 11, YW, gx)
        out.append(line(sx, src_y + CH + 1, gx, y - 1, stroke, 1.1, dash="4 3"))
        out.append(line(sx + 8 * CW, src_y + CH + 1, gx + 8 * YW, y - 1, stroke, 1.1, dash="4 3"))
        how = "低 8 个字节直接扩展" if half == 0 else "先右移 8 个字节（_mm_srli_si128）"
        out.append(text(gx + 4 * YW, y + CH + 15, f"{how}，与 x[c+{8 * g}..c+{8 * g + 7}] 相乘", 11.5, stroke))
    out.append(text(560, HY + CH + 40, "4 组各 8 个权重：权重 c + 8g + k（k = 0..7）与 x[c + 8g + k] 对应，c = 32b",
                    12.5, INK, "bold"))
    return out


if __name__ == "__main__":
    defs = (_marker(PURPLE), _marker(TEAL))
    save("pd-class-decode-1", W, H, frame1(), defs)
    save("pd-class-decode-2", W, H, frame2(), defs)
    save("pd-class-decode-3", W, H, frame3(), defs)
