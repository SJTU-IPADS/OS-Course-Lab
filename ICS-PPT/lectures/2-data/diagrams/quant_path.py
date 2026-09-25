#!/usr/bin/env python3
"""Where a weight travels, and which hop quantization shortens.

One row of stores: Disk → main memory → GPU memory → compute units.
The CPU sits above the host side and starts the two loads (dashed pointers);
those happen once. The last hop repeats on every token, and that is the one
quantization shortens, paid for by a dequantize on the compute units.
Writes ../assets/quant-path.svg.
"""

import math
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MUTED,
                    ORANGE, line, rect, svg, text)

W, H = 1060, 320

BW, BH, BY = 165, 110, 150          # store boxes
XS = [50, 315, 580, 845]            # their left edges; gaps of 100 hold the arrows


def head(x, y, dx, dy, color, size=8.0, half=4.6):
    """A triangle whose tip is at (x, y), pointing along (dx, dy)."""
    n = math.hypot(dx, dy) or 1.0
    ux, uy = dx / n, dy / n
    bx, by = x - size * ux, y - size * uy
    px, py = -uy * half, ux * half
    return (f'<polygon points="{x:.1f},{y:.1f} {bx + px:.1f},{by + py:.1f} '
            f'{bx - px:.1f},{by - py:.1f}" fill="{color}"/>')


def data_arrow(x1, x2, y, color, label, width=1.8):
    return [line(x1, y, x2 - 8, y, color, width),
            head(x2, y, 1, 0, color),
            text((x1 + x2) / 2, y - 11, label, 12.5, color, "bold")]


def pointer(x1, y1, x2, y2, color):
    """A dashed control line from the CPU down to a data arrow."""
    return [line(x1, y1, x2, y2, color, 1.3, dash="4 3"),
            head(x2, y2, x2 - x1, y2 - y1, color, 7.0, 4.0)]


def box(x, title, lines, fill, stroke, title_fill, body_fill, width=1.4):
    out = [rect(x, BY, BW, BH, fill, stroke, rx=8, width=width),
           text(x + BW / 2, BY + 34, title, 17, title_fill, "bold")]
    for i, (s, fl) in enumerate(lines):
        out.append(text(x + BW / 2, BY + 62 + i * 23, s, 13, fl or body_fill))
    return out


def build():
    out = []

    # GPU frame around the last two stores.
    out.append(rect(XS[2] - 22, BY - 32, XS[3] + BW + 22 - (XS[2] - 22), BH + 54,
                    FILL_BLUE, BLUE, rx=10, width=2.0))
    out.append(text(XS[3] + BW + 4, BY - 14, "GPU", 14, BLUE, "bold", anchor="end"))

    out += box(XS[0], "Disk", [("模型文件 1.9 GB", None), ("只读一次", MUTED)],
               FILL_GREY, LINE, MUTED, INK)
    out += box(XS[1], "主存", [("CPU 的内存", None), ("中转一次", MUTED)],
               FILL_GREY, LINE, MUTED, INK)
    out += box(XS[2], "显存", [("权重常驻", None), ("4 位比 16 位短", BLUE)],
               "#ffffff", BLUE, INK, INK, 1.6)
    out += box(XS[3], "计算单元", [("反量化：乘回格宽", ORANGE), ("再做矩阵乘", None)],
               FILL_ORANGE, ORANGE, ORANGE, INK, 1.8)

    # Data path: three hops.
    ym = BY + BH / 2
    out += data_arrow(XS[0] + BW, XS[1], ym, MUTED, "① 装入主存")
    out += data_arrow(XS[1] + BW, XS[2], ym, MUTED, "② 拷到显存")
    out += data_arrow(XS[2] + BW, XS[3], ym, ORANGE, "③ 每 token 一次", 2.4)

    # CPU above the host side, starting ① and ②.
    cx, cy, cw, ch = XS[1], 26, BW, 62
    out.append(rect(cx, cy, cw, ch, FILL_GREY, LINE, rx=8, width=1.4))
    out.append(text(cx + cw / 2, cy + 26, "CPU", 16, MUTED, "bold"))
    out.append(text(cx + cw / 2, cy + 48, "发起 ① 与 ②", 13, MUTED))
    out += pointer(cx + 14, cy + ch, XS[1] - 40, ym - 22, MUTED)
    out += pointer(cx + cw - 14, cy + ch, XS[2] - 40, ym - 22, MUTED)

    out.append(text(W / 2, 302,
                    "① ② 只在加载时各走一遍；③ 每生成一个 token 都走一遍，量化减少的是 ③ 的字节数",
                    14.5, ORANGE, "bold"))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "quant-path.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
