#!/usr/bin/env python3
"""Roofline position and end-to-end latency for BERT vs GPT-2.

Left: a schematic roofline (ridge at 100 FLOP/Byte) with the paper's two
workloads on it -- BERT as the 117-266 FLOP/Byte range its sizes and sequence
lengths span, GPT-2 decode at 2 FLOP/Byte, both unlabelled because the page asks
the students for these numbers.  Right: the measured latency gap the left panel
explains.  How quantization moves GPT-2 is roofline_shift.py, on the next slide.
Run it to refresh ../assets/roofline-latency.svg.
"""

import math
import pathlib

import svgkit as k

W, H = 1000, 420

RED, GREEN, GOLD = "#c0392b", "#196b24", "#b8860b"
FILL_RED, FILL_GREEN = "#fbe6e2", "#e7f2e4"

# left panel: a log-log plot, four decades each way
LX0, LX1, LY0, LY1 = 78, 508, 66, 340
DEC_X, DEC_Y = (LX1 - LX0) / 4.0, (LY1 - LY0) / 4.0
PEAK, BW = 100.0, 1.0                       # TFLOP/s and TB/s -> ridge at 100

# right panel: two bars on a broken axis
RX0, RY0, RY1 = 600, 66, 340
BAR_W = 76
BERT_X, GPT_X = 660, 820
BREAK_Y = 212


def px(i):
    return LX0 + (math.log10(i) + 1.0) * DEC_X


def py(v):
    return LY1 - (math.log10(v) + 1.0) * DEC_Y


def attainable(i):
    return min(PEAK, BW * i)


def roofline():
    out = [k.text((LX0 + LX1) / 2, 34, "Roofline 示意：算术强度决定受限于带宽还是算力",
                  14.5, k.INK, "bold")]

    # axes
    out.append(k.line(LX0, LY1, LX1, LY1, k.MUTED, 1.4))
    out.append(k.line(LX0, LY1, LX0, LY0, k.MUTED, 1.4))
    for e, lab in enumerate(("0.1", "1", "10", "100", "1000")):
        x = LX0 + e * DEC_X
        out.append(k.line(x, LY1, x, LY1 + 5, k.MUTED, 1.2))
        out.append(k.text(x, LY1 + 20, lab, 11.5, k.MUTED))
        if e:
            out.append(k.line(x, LY0, x, LY1, "#e4ebef", 1.0, dash="3 5"))
    for e, lab in enumerate(("0.1", "1", "10", "100", "1000")):
        y = LY1 - e * DEC_Y
        out.append(k.line(LX0 - 5, y, LX0, y, k.MUTED, 1.2))
        out.append(k.text(LX0 - 10, y + 4, lab, 11.5, k.MUTED, anchor="end"))
        if e:
            out.append(k.line(LX0, y, LX1, y, "#e4ebef", 1.0, dash="3 5"))
    out.append(k.text((LX0 + LX1) / 2, LY1 + 40, "算术强度 I (FLOP/Byte，对数)",
                      12.5, k.MUTED))
    out.append(k.text(LX0 - 14, LY0 - 12, "可达算力 (TFLOP/s，对数)", 12.5, k.MUTED,
                      anchor="start"))

    # the roof itself: a bandwidth slope meeting a compute ceiling
    ridge_x, ridge_y = px(PEAK / BW), py(PEAK)
    out.append(k.line(px(0.1), py(attainable(0.1)), ridge_x, ridge_y, k.BLUE, 2.6))
    out.append(k.line(ridge_x, ridge_y, LX1, ridge_y, k.BLUE, 2.6))
    out.append(k.text(215, 148, "显存带宽斜顶", 12.5, k.BLUE, "bold"))
    out.append(k.text(ridge_x + 72, ridge_y + 26, "算力平顶", 12.5, k.BLUE, "bold"))

    # GPT-2 decode is one point; BERT is the range its sizes and lengths span.  Neither
    # carries its number: the page leaves that column for the students to compute.
    gx, gy = px(2.0), py(attainable(2.0))
    out.append(f'<circle cx="{gx:.1f}" cy="{gy:.1f}" r="13" fill="{RED}" '
               f'fill-opacity="0.18"/>')
    out.append(f'<circle cx="{gx:.1f}" cy="{gy:.1f}" r="6.5" fill="{RED}"/>')
    out.append(k.text(gx - 6, gy + 30, "GPT-2 Decode", 12.5, RED, "bold", anchor="start"))
    b0, b1 = px(117.0), px(266.0)
    out.append(k.line(b0, ridge_y, b1, ridge_y, GREEN, 9.0))
    out.append(k.text((b0 + b1) / 2, ridge_y - 18, "BERT", 12.5, GREEN, "bold"))

    return out


def latency():
    out = [k.text((RX0 + 920) / 2, 34, "端到端相对时延（序列长 4096）", 14.5,
                  k.INK, "bold")]
    out.append(k.line(RX0, RY1, 940, RY1, k.MUTED, 1.4))

    # BERT: drawn to scale against its own small value
    bert_h = 42.0
    out.append(k.rect(BERT_X - BAR_W / 2, RY1 - bert_h, BAR_W, bert_h,
                      "#dce8ef", k.BLUE, rx=2, width=1.8))
    out.append(k.text(BERT_X, RY1 - bert_h - 12, "84", 15, k.BLUE, "bold"))
    out.append(k.text(BERT_X, RY1 + 22, "BERT-Base", 12.5, k.INK, "bold"))
    out.append(k.text(BERT_X, RY1 + 40, "Encoder 整段并行", 11.5, k.MUTED))

    # GPT-2: off the top of any shared scale, so the bar is broken
    out.append(k.rect(GPT_X - BAR_W / 2, RY0 + 30, BAR_W, RY1 - RY0 - 30,
                      FILL_RED, RED, rx=2, width=1.8))
    out.append(f'<path d="M {GPT_X - BAR_W / 2 - 6:.1f} {BREAK_Y + 8} '
               f'L {GPT_X:.1f} {BREAK_Y - 4} L {GPT_X + BAR_W / 2 + 6:.1f} '
               f'{BREAK_Y + 8}" fill="none" stroke="#ffffff" stroke-width="7"/>')
    out.append(f'<path d="M {GPT_X - BAR_W / 2 - 6:.1f} {BREAK_Y + 8} '
               f'L {GPT_X:.1f} {BREAK_Y - 4} L {GPT_X + BAR_W / 2 + 6:.1f} '
               f'{BREAK_Y + 8}" fill="none" stroke="{RED}" stroke-width="1.4"/>')
    out.append(k.text(GPT_X, RY0 + 18, "2344", 15, RED, "bold"))
    out.append(k.text(GPT_X, RY1 + 22, "GPT-2", 12.5, k.INK, "bold"))
    out.append(k.text(GPT_X, RY1 + 40, "Decoder 自回归", 11.5, k.MUTED))
    out.append(k.text(GPT_X, BREAK_Y + 34, "纵轴截断", 11.5, RED))

    # the gap between them
    out.append(k.line(BERT_X, RY0 + 6, GPT_X, RY0 + 6, GOLD, 1.6, dash="5 4"))
    out.append(k.text((BERT_X + GPT_X) / 2, RY0 - 4, "×28 时延鸿沟", 14, GOLD, "bold"))
    return out


def build():
    return k.svg(W, H, roofline() + latency())


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "roofline-latency.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
