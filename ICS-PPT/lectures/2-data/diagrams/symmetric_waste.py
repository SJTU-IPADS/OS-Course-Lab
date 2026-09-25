#!/usr/bin/env python3
"""Where the 16 grid points of symmetric quantization and Q4_1 land on all-positive weights.

The data is the real sample the next demo runs on: the 2048 BF16 weights of
Llama-3.2-1B's final norm (../examples/ext/w-final-norm.bf16), all positive.
The top row is their histogram; the two rows below put the 16 grid points of
each scheme on the same real axis, with one scale for the whole tensor as the
page computes it.  The symmetric row follows the exercise after it: the
largest magnitude maps to q = 7, so d = max|w| / 7 and the points are
-8d .. 7d.  (The measured Q4_0 in quant_compare.c takes d = extreme / -8
instead, which gives -7|d| .. 8|d|; the demo's notes say so.)
Run it to refresh ../assets/symmetric-waste.svg.
"""

import pathlib
import struct

import svgkit as k

W, H = 1100, 552

HERE = pathlib.Path(__file__).resolve().parent
SAMPLE = HERE.parent / "examples" / "ext" / "w-final-norm.bf16"

FILL_DATA = "#fdf1e8"
GREY = "#b4c0c8"


def load():
    raw = SAMPLE.read_bytes()
    return [struct.unpack("<f", struct.pack("<I", h << 16))[0]
            for (h,) in struct.iter_unpack("<H", raw)]


X = load()
LO, HI = min(X), max(X)
D_SYM = abs(max(X, key=abs)) / 7
SYM = [i * D_SYM for i in range(-8, 8)]
D_ASYM = (HI - LO) / 15
ASYM = [LO + j * D_ASYM for j in range(16)]

AX0, AX1, VLO, VHI = 30, 1075, -3.5, 3.1
HIST_Y, SYM_Y, ASYM_Y, AXIS_Y = 160, 272, 446, 520
BAND_TOP, BAND_BOTTOM = 44, 503
BIN = 0.1


def px(v):
    return AX0 + (v - VLO) / (VHI - VLO) * (AX1 - AX0)


def heading(y, parts, note):
    """A row's name and parameters, above its left end where the axis is empty."""
    return [k.rich(20, y, parts, 23, k.INK, "bold", "start"),
            k.rich(20, y + 28, note, 19, k.MUTED, "normal", "start")]


def band():
    x0, x1 = px(LO), px(HI)
    return [k.rect(x0, BAND_TOP, x1 - x0, BAND_BOTTOM - BAND_TOP, FILL_DATA, "none", rx=0,
                   width=0),
            k.line(x0, BAND_TOP, x0, BAND_BOTTOM, k.ORANGE, 1.3, "4 3"),
            k.line(x1, BAND_TOP, x1, BAND_BOTTOM, k.ORANGE, 1.3, "4 3"),
            k.text((x0 + x1) / 2, BAND_TOP - 12, f"数据范围 [+{LO:.4f}, +{HI:.4f}]", 21,
                   k.ORANGE, "bold")]


def histogram():
    out = heading(30, ["权重分布"], ["2048 个，每柱宽 0.1"])
    counts = {}
    for v in X:
        counts[int(v // BIN)] = counts.get(int(v // BIN), 0) + 1
    top = max(counts.values())
    for b, c in counts.items():
        h = max(2.5, c / top * 100)
        x0, x1 = px(b * BIN), px((b + 1) * BIN)
        out.append(k.rect(x0 + 0.5, HIST_Y - h, x1 - x0 - 1, h, k.BLUE, "none", rx=0, width=0))
    out.append(k.line(AX0, HIST_Y, AX1, HIST_Y, k.LINE, 1.5))
    out.append(k.rich(px(-1.55), HIST_Y - 16, [("v", "w"), " < 0 的区间内没有权重"], 19,
                      k.MUTED, "bold"))
    return out


def grid(y, points, inside, name, note, step, step_at):
    out = heading(y - 70, name, note)
    out.append(k.line(AX0, y, AX1, y, k.LINE, 1.5))
    for v in points:
        c = k.BLUE if inside(v) else GREY
        out.append(k.line(px(v), y - 12, px(v), y + 12, c, 2.6))
        out.append(k.rect(px(v) - 4, y - 4, 8, 8, c, "none", rx=4, width=0))
    a, b = px(points[step_at]), px(points[step_at + 1])
    out.append(f'<path d="M {a:.1f} {y - 17:.1f} L {a:.1f} {y - 24:.1f} L {b:.1f} '
               f'{y - 24:.1f} L {b:.1f} {y - 17:.1f}" fill="none" stroke="{k.BLUE}" '
               f'stroke-width="1.5"/>')
    out.append(k.rich((a + b) / 2, y - 31, [("v", "d"), f" = {step:.3f}"], 19.5, k.BLUE,
                      "bold"))
    return out


def symmetric():
    inside = lambda v: LO <= v <= HI
    out = grid(SYM_Y, SYM, inside, ["对称量化"],
               [("v", "d"), " = 2.9219 / 7，格点 −8", ("v", "d"), " ~ 7", ("v", "d")],
               D_SYM, 12)
    out += k.brace(px(SYM[0]), px(SYM[8]), SYM_Y + 18, k.MUTED,
                   "9 个格点在数据范围之外", 19.5)
    out.append(k.text((px(SYM[0]) + px(SYM[8])) / 2, SYM_Y + 72, "负半轴 8 个，0 处 1 个",
                      17.5, k.MUTED))
    out += k.brace(px(SYM[9]), px(SYM[15]), SYM_Y + 18, k.BLUE,
                   "数据范围内只有 7 个格点", 19.5)
    return out


def asymmetric():
    inside = lambda v: True
    out = grid(ASYM_Y, ASYM, inside, ["非对称量化 Q4_1"],
               [("v", "m"), " = 0.0417，", ("v", "d"), " = (2.9219 − 0.0417) / 15"],
               D_ASYM, 13)
    out += k.brace(px(ASYM[0]), px(ASYM[15]), ASYM_Y + 18, k.BLUE,
                   "16 个格点全部在数据范围内", 19.5)
    return out


def axis():
    out = [k.line(AX0, AXIS_Y, AX1, AXIS_Y, k.LINE, 1.5)]
    for v in range(-3, 4):
        x = px(v)
        out.append(k.line(x, AXIS_Y - 6, x, AXIS_Y + 6, k.MUTED, 1.5))
        out.append(k.text(x, AXIS_Y + 27, f"{v:+d}".replace("-", "−") if v else "0", 18,
                          k.MUTED, font=k.MONO))
    out.append(k.rich(20, AXIS_Y - 12, ["实数 ", ("v", "w")], 18, k.MUTED, "bold", "start"))
    return out


def build():
    return k.svg(W, H, band() + histogram() + symmetric() + asymmetric() + axis())


if __name__ == "__main__":
    path = HERE.parent / "assets" / "symmetric-waste.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
