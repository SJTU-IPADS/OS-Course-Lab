#!/usr/bin/env python3
"""The figure part-5.md asks for on its page 9: two conversion datapaths.

Left, FP32 to BF16: the upper 16 bits go straight through; the lower 16 bits
feed one adder (+ 0x7FFF + LSB) whose carry-out is the only thing that
reaches the result, as +1 on the upper half.  Right, FP32 to FP16 for
contrast: the exponent has to be re-biased, range cases picked by a
multiplexer, and the mantissa shifted before it can be rounded.  Run it to
refresh ../assets/bf16-datapath.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, INK, LINE, MONO, MUTED,
                    ORANGE, line, rect, svg, text)

W, H = 940, 262
RED, GREEN = "#c0392b", "#196b24"
FILL_GREEN = "#cfe8d2"


def arrow_down(x, y1, y2, color=INK, width=2.0, head=8):
    return [line(x, y1, x, y2 - head, color, width),
            f'<polygon points="{x:.1f},{y2:.1f} {x - head * 0.6:.1f},{y2 - head:.1f} '
            f'{x + head * 0.6:.1f},{y2 - head:.1f}" fill="{color}"/>']


def arrow_left(x1, x2, y, color=INK, width=2.0, head=8):
    return [line(x1, y, x2 + head, y, color, width),
            f'<polygon points="{x2:.1f},{y:.1f} {x2 + head:.1f},{y - head * 0.6:.1f} '
            f'{x2 + head:.1f},{y + head * 0.6:.1f}" fill="{color}"/>']


def bf16_side():
    hx, top, hw, rh = 64, 40, 240, 34
    lx, lw = hx + hw + 14, 186
    out = [text(270, 18, "FP32 → BF16：高 16 位直通，一次加法完成舍入", 14, GREEN, "bold")]
    out.append(rect(hx, top, hw, rh, FILL_BLUE, BLUE, width=1.6))
    out.append(text(hx + hw / 2, top + 22, "高 16 位：符号 · 阶码 · 尾数前 7 位", 12.5, INK))
    out.append(rect(lx, top, lw, rh, FILL_GREY, MUTED, width=1.6))
    out.append(text(lx + lw / 2, top + 22, "低 16 位", 12.5, INK))
    out.append(text(hx - 6, top + 22, "FP32", 12.5, INK, "bold", "end"))

    # the adder under the low half
    ax, ay, aw, ah = lx + 13, 112, lw - 26, 44
    out += arrow_down(lx + lw / 2, top + rh, ay)
    out.append(rect(ax, ay, aw, ah, FILL_ORANGE, ORANGE, width=1.6))
    out.append(text(ax + aw / 2, ay + 19, "16 位加法", 13, INK, "bold"))
    out.append(text(ax + aw / 2, ay + 36, "+ 0x7FFF + LSB", 12.5, INK, font=MONO))
    # LSB of the upper half into the adder
    out.append(line(hx + hw - 6, top + rh, hx + hw - 6, top + rh + 16, ORANGE, 1.4))
    out.append(line(hx + hw - 6, top + rh + 16, ax + aw - 26, top + rh + 16, ORANGE, 1.4))
    out += arrow_down(ax + aw - 26, top + rh + 16, ay, ORANGE, 1.4, 6)
    out.append(text(hx + hw + 8, top + rh + 13, "LSB", 11.5, ORANGE, "bold", "start",
                    font=MONO))
    # low 16 bits of the sum are dropped
    out += arrow_down(ax + aw / 2, ay + ah, ay + ah + 26, MUTED, 1.4, 6)
    out.append(text(ax + aw / 2, ay + ah + 42, "低 16 位丢弃", 12, MUTED))

    # upper half straight down through +1
    cx, cy = hx + hw / 2, ay + ah / 2
    out += arrow_down(cx, top + rh, cy - 15, BLUE, 5, 11)
    out.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="15" fill="#fff" stroke="{INK}" '
               f'stroke-width="1.8"/>')
    out.append(text(cx, cy + 6, "+", 18, INK, "bold"))
    out += arrow_left(ax, cx + 15, cy, RED, 2.2)
    out.append(text((ax + cx + 15) / 2, cy - 8, "进位", 12.5, RED, "bold"))
    oy = 206
    out += arrow_down(cx, cy + 15, oy, BLUE, 5, 11)
    out.append(rect(hx, oy, hw, rh, FILL_GREEN, GREEN, width=1.6))
    out.append(text(hx + hw / 2, oy + 22, "BF16 结果", 13, INK, "bold"))
    return out


def fp16_side():
    x, w, y, h, gap = 600, 290, 34, 30, 12
    out = [text(x + w / 2, 18, "FP32 → FP16：逐项改写", 14, RED, "bold"),
           line(540, 10, 540, H - 10, LINE, 1, dash="4 4")]
    steps = ["拆出符号、阶码、尾数三个字段",
             "阶码重偏置：E − 127 + 15",
             "多路选择：上溢 / 下溢 / 非规格化",
             "尾数移位器阵列：23 位 → 10 位",
             "舍入，再拼装为 FP16"]
    for i, s in enumerate(steps):
        yy = y + i * (h + gap)
        out.append(rect(x, yy, w, h, FILL_GREY, MUTED, width=1.4))
        out.append(text(x + w / 2, yy + 20, s, 12.5, INK))
        if i:
            out += arrow_down(x + w / 2, yy - gap, yy, MUTED, 1.4, 6)
    return out


def build():
    return svg(W, H, bf16_side() + fp16_side())


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "bf16-datapath.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
