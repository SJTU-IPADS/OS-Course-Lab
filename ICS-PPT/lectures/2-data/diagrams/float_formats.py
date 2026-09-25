#!/usr/bin/env python3
"""One picture for the whole family of floating-point formats.

Every format is the same three fields in the same order; what a format
chooses is how many bits each field gets. Drawing them to a common bit scale
makes that the only visible difference — and makes it visible that BF16 is
FP32's leading half: the conversion keeps those 16 bits and rounds on the rest.
FP64 is left out: at this scale it is twice as wide as anything else on the
slide, and this lecture never uses it. Run it to refresh
../assets/float-formats.svg.
"""

import pathlib

# The whole picture has to fit the box the slide gives it (about 260px tall
# under the bullets), so the rows are short and the bit scale is small.
BIT = 10.0                       # px per bit, shared by every row
X0, W, ROW_H, ROW_GAP = 104, 700, 20, 5
TOP = 62

BLUE, ORANGE, INK = "#156082", "#e97132", "#0e2841"
LINE, MUTED = "#9fb5c3", "#5a6b78"
FILL_BLUE, FILL_ORANGE, FILL_SIGN = "#cfe3ec", "#f8d5c0", "#dde5ea"
FONT = "PingFang SC, Noto Sans CJK SC, Source Han Sans SC, sans-serif"

# (name, exponent bits, mantissa bits). FP32 and BF16 are adjacent so the cut
# between them is one short dashed line rather than a rule crossing the chart.
FORMATS = [
    ("FP32", 8, 23),
    ("BF16", 8, 7),
    ("FP16", 5, 10),
    ("TF32", 8, 10),
    ("FP8 E4M3", 4, 3),
    ("FP8 E5M2", 5, 2),
    ("FP4 E2M1", 2, 1),
]


def text(x, y, s, size=13, fill=INK, weight="normal", anchor="middle"):
    return (f'<text x="{x}" y="{y}" font-family="{FONT}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}">{s}</text>')


def seg(x, y, bits, fill, stroke, label):
    """One field of one format; the bit count sits inside when it fits."""
    w = bits * BIT
    out = [f'<rect x="{x:.1f}" y="{y}" width="{w:.1f}" height="{ROW_H}" rx="3" '
           f'fill="{fill}" stroke="{stroke}" stroke-width="1.3"/>']
    if w >= 18:
        out.append(text(x + w / 2, y + ROW_H / 2 + 4, label, 12, INK))
    return out


def build():
    h = TOP + len(FORMATS) * (ROW_H + ROW_GAP) + 10
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {h}" '
           f'width="{W}" height="{h}">']

    x = X0
    for label, fill, stroke in (("符号 1 位", FILL_SIGN, MUTED),
                                ("阶码 · 决定范围", FILL_BLUE, BLUE),
                                ("尾数 · 决定精度", FILL_ORANGE, ORANGE)):
        out.append(f'<rect x="{x}" y="14" width="15" height="15" rx="3" fill="{fill}" '
                   f'stroke="{stroke}" stroke-width="1.3"/>')
        out.append(text(x + 21, 26, label, 12, MUTED, anchor="start"))
        x += 40 + 12.4 * len(label)

    for i, (name, exp, man) in enumerate(FORMATS):
        y = TOP + i * (ROW_H + ROW_GAP)
        total = 1 + exp + man
        out.append(text(X0 - 10, y + ROW_H / 2 + 4, name, 13, INK, "bold", "end"))
        out += seg(X0, y, 1, FILL_SIGN, MUTED, "")
        out += seg(X0 + BIT, y, exp, FILL_BLUE, BLUE, str(exp))
        out += seg(X0 + (1 + exp) * BIT, y, man, FILL_ORANGE, ORANGE, str(man))
        end = X0 + total * BIT
        out.append(text(end + 10, y + ROW_H / 2 + 4, f"{total} 位", 12, MUTED,
                        anchor="start"))
        out.append(text(end + 56, y + ROW_H / 2 + 4, f"1 · {exp} · {man}", 12,
                        MUTED, anchor="start"))

    # FP32's leading 16 bits are BF16: the cut between the first two rows.
    cut = X0 + 16 * BIT
    y_top = TOP - 5
    y_bot = TOP + (ROW_H + ROW_GAP) + ROW_H + 5
    out.append(f'<path d="M {cut} {y_top} L {cut} {y_bot}" stroke="{ORANGE}" '
               f'stroke-width="1.8" stroke-dasharray="5 4"/>')
    out.append(text(cut + 10, TOP - 12, "BF16 取 FP32 的前 16 位，再舍入", 12.5, ORANGE,
                    "bold", "start"))

    out.append("</svg>")
    return "\n".join(out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "float-formats.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
