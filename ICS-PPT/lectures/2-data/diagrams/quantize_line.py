#!/usr/bin/env python3
"""What quantization does to one group of weights.

A 4-bit code can name 16 values. The figure fixes those 16 on the number
line, drops the group's actual weights onto the nearest one, and marks the
distance that gets lost — half a step, which is the whole error budget.
Run it to refresh ../assets/quantize-line.svg.
"""

import pathlib

W, H = 900, 268
AX0, AX1, AXIS_Y = 132, 866, 150   # AX0 leaves room for the row labels
BLUE, ORANGE, INK = "#156082", "#e97132", "#0e2841"
LINE, MUTED = "#9fb5c3", "#5a6b78"
FONT = "PingFang SC, Noto Sans CJK SC, Source Han Sans SC, sans-serif"
MONO = "SFMono-Regular, Menlo, Consolas, monospace"

LEVELS = 16
STEP = (AX1 - AX0) / (LEVELS - 1)
WEIGHTS = [1.4, 4.6, 6.2, 9.35, 11.8, 13.3]      # in code units, off the grid


def text(x, y, s, size=13, fill=INK, weight="normal", anchor="middle", font=FONT):
    return (f'<text x="{x}" y="{y}" font-family="{font}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}">{s}</text>')


def build():
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" '
           f'width="{W}" height="{H}">']

    out.append(text(W / 2, 30, "一组权重的取值范围被 16 个等距的值覆盖", 15.5, INK, "bold"))

    out.append(f'<line x1="{AX0 - 26}" y1="{AXIS_Y}" x2="{AX1 + 26}" y2="{AXIS_Y}" '
               f'stroke="{LINE}" stroke-width="1.6"/>')
    for k in range(LEVELS):
        x = AX0 + k * STEP
        out.append(f'<line x1="{x:.1f}" y1="{AXIS_Y - 9}" x2="{x:.1f}" '
                   f'y2="{AXIS_Y + 9}" stroke="{BLUE}" stroke-width="1.6"/>')
        out.append(text(x, AXIS_Y + 27, str(k), 11.5, BLUE, font=MONO))
    out.append(text(AX0 - 34, AXIS_Y + 27, "编码", 12, MUTED, anchor="end"))
    out.append(text(AX0 - 34, AXIS_Y + 5, "可表示的值", 12.5, MUTED, anchor="end"))

    for w in WEIGHTS:
        x = AX0 + w * STEP
        near = round(w)
        xs = AX0 + near * STEP
        out.append(f'<circle cx="{x:.1f}" cy="{AXIS_Y - 56}" r="5.5" fill="{ORANGE}"/>')
        out.append(f'<path d="M {x:.1f} {AXIS_Y - 50} L {xs:.1f} {AXIS_Y - 12}" '
                   f'stroke="{ORANGE}" stroke-width="1.4" stroke-dasharray="4 3"/>')
    out.append(text(AX0 - 34, AXIS_Y - 52, "实际权重", 12.5, ORANGE, "bold", anchor="end"))

    # half a step: the largest distance a weight can be moved
    hx = AX0 + 9 * STEP
    y = AXIS_Y + 54
    out.append(f'<path d="M {hx:.1f} {y} L {hx + STEP / 2:.1f} {y}" stroke="{ORANGE}" '
               f'stroke-width="2"/>')
    for x in (hx, hx + STEP / 2):
        out.append(f'<line x1="{x:.1f}" y1="{y - 6}" x2="{x:.1f}" y2="{y + 6}" '
                   f'stroke="{ORANGE}" stroke-width="2"/>')
    out.append(text(hx + STEP / 2 + 12, y + 5, "半个步长：格点之间的最大偏移", 13,
                    ORANGE, "bold", anchor="start"))

    out.append(text(26, H - 22, "d = max / -8", 14, INK, "normal", "start",
                    font=MONO))
    out.append(text(148, H - 22, "max 是组内绝对值最大的权重，连同它的符号；组内动态范围越集中，误差越小",
                    13.5, MUTED, anchor="start"))

    out.append("</svg>")
    return "\n".join(out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "quantize-line.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
