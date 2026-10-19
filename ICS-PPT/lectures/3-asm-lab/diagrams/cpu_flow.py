#!/usr/bin/env python3
"""The decisions of modrm_decode, as a flowchart in three frames: the ModRM
byte and the register case; the memory case with the SIB byte and the two
cases without a base; the displacement and the return value.

Run it to refresh ../assets/cpu_flow-1.svg .. cpu_flow-3.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREY, GREEN, INK, LINE, MUTED, ORANGE, WHITE, arrow, elbow, rect,
                    save, text)
from cpu_kit import DISP, MODRM, SIB, TODO, code, note

W, H = 1120, 430


def node(x, y, w, h, lines, fill, stroke, size=14):
    out = [rect(x, y, w, h, fill, stroke, rx=8, width=1.5)]
    n = len(lines)
    for k, s in enumerate(lines):
        yy = y + h / 2 + (k - (n - 1) / 2) * 20 + 5
        out.append(text(x + w / 2, yy, s, size, INK, "bold" if k == 0 else "normal"))
    return out


def diamond(cx, cy, w, h, label, stroke=INK):
    d = f"M {cx - w / 2} {cy} L {cx} {cy - h / 2} L {cx + w / 2} {cy} L {cx} {cy + h / 2} Z"
    return [f'<path d="{d}" fill="{WHITE}" stroke="{stroke}" stroke-width="1.5"/>',
            text(cx, cy + 5, label, 14, INK, "bold")]


def frame(stage):
    out = []
    # column 1: the ModRM byte and the register case
    out += node(20, 14, 220, 64, ["读 ModRM 字节 p[0]", "mod、sub、reg（加 REX.R）"], *MODRM)
    out.append(arrow(130, 78, 130, 98, INK, 2))
    out += diamond(130, 126, 160, 52, "mod == 11？")
    out.append(arrow(210, 126, 262, 126, INK, 2))
    out.append(text(236, 118, "是", 13, GREEN, "bold"))
    out += node(266, 94, 250, 64, ["rm = rm 字段 + REX.B", "base = index = -1，scale = 1",
                                    "rip = dsize = disp = 0；返回 1"], WHITE, MUTED)
    out.append(arrow(130, 152, 130, 186, INK, 2))
    out.append(text(144, 174, "否", 13, ORANGE, "bold", anchor="start"))
    out += node(20, 190, 220, 52, ["内存操作数：rm = -1", "base、index 由 rm 字段决定"], WHITE, MUTED)
    if stage < 2:
        return out
    # column 2: the SIB byte
    out.append(arrow(240, 216, 296, 216, INK, 2))
    out += diamond(380, 216, 160, 52, "rm 字段 == 100？")
    out.append(arrow(380, 242, 380, 272, INK, 2))
    out.append(text(394, 262, "是", 13, GREEN, "bold", anchor="start"))
    out += node(240, 276, 290, 64, ["读 SIB 字节 p[1]", "scale = 1 << 两位；index = 3 位 + REX.X",
                                     "base = 3 位 + REX.B"], *SIB)
    out.append(arrow(530, 308, 541, 308, INK, 2))
    out += node(545, 276, 260, 64, ["两个特殊值", "index 得到 4：没有变址，index = -1",
                                     "base 字段 101 且 mod 00：没有基址"], WHITE, MUTED)
    # column 3: rip-relative or a plain base
    out.append(arrow(460, 216, 561, 216, INK, 2))
    out.append(text(510, 208, "否", 13, ORANGE, "bold"))
    out += diamond(660, 216, 190, 52, "rm 101 且 mod 00？")
    out.append(arrow(660, 190, 660, 146, INK, 2))
    out.append(text(674, 172, "是", 13, GREEN, "bold", anchor="start"))
    out += node(560, 86, 200, 56, ["rip = 1", "没有基址，没有变址"], WHITE, MUTED)
    out.append(arrow(755, 216, 796, 216, INK, 2))
    out.append(text(776, 208, "否", 13, ORANGE, "bold"))
    out += node(800, 190, 200, 52, ["base = rm 字段 + REX.B"], WHITE, MUTED)
    if stage < 3:
        return out
    # the displacement and the return value
    out.append(arrow(385, 340, 385, 352, INK, 2))
    out.append(arrow(675, 340, 675, 352, INK, 2))
    out.append(arrow(900, 242, 900, 352, INK, 2))
    out.append(elbow([(760, 114), (1040, 114), (1040, 352)], INK, 2))
    out += node(20, 356, 1080, 60,
                ["偏移 dsize：mod 01 为 1 字节，mod 10 为 4 字节，mod 00 为 0 字节，没有基址时 4 字节；"
                 "disp = 随后 dsize 个字节，小端，符号扩展到 64 位",
                 "返回用掉的字节数 1 到 6；读每个字节之前检查它在 n 个字节之内，否则返回 0"],
                *DISP, size=13)
    return out


if __name__ == "__main__":
    for k in (1, 2, 3):
        save(f"cpu_flow-{k}", W, H, frame(k))
