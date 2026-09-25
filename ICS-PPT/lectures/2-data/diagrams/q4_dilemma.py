#!/usr/bin/env python3
"""The question kquants-superblock opens with, drawn as two points and a gap.

Q4_0 and Q4_1 on the plane of bits per weight against error, measured on the
all-positive final-norm weights that symmetric-failure-cont runs
`./quant_compare norm` on (4.50 bits at 4.25%, 5.00 bits at 0.94%).  The
dashed circle is the corner neither reaches: Q4_0's size with Q4_1's error.
Q4_K is left off; its row is in the table two slides later.  Run it to refresh
../assets/q4-dilemma.svg.
"""

import pathlib

import svgkit as k

W, H = 800, 290

RED, GREEN, GOLD = "#c0392b", "#196b24", "#b8860b"

BIT_LO, BIT_HI = 4.25, 5.25                 # x axis: bits per weight
ERR_HI = 5.0                                # y axis: 0 .. 5 %
X0, X1 = 160, 660
Y0, Y1 = 24, 238

Q4_0 = (4.50, 4.25)
Q4_1 = (5.00, 0.94)


def px(bits):
    return X0 + (bits - BIT_LO) / (BIT_HI - BIT_LO) * (X1 - X0)


def py(err):
    return Y1 - err / ERR_HI * (Y1 - Y0)


def axes():
    out = [k.line(X0, Y1, X1, Y1, k.MUTED, 1.3), k.line(X0, Y1, X0, Y0 - 6, k.MUTED, 1.3)]
    for b in (4.5, 5.0):
        out.append(k.line(px(b), Y1, px(b), Y1 + 5, k.MUTED, 1.2))
        out.append(k.text(px(b), Y1 + 20, f"{b:.1f}", 12.5, k.MUTED))
    for e in range(0, 6):
        out.append(k.line(X0 - 5, py(e), X0, py(e), k.MUTED, 1.2))
        out.append(k.text(X0 - 9, py(e) + 4, f"{e}%", 12, k.MUTED, anchor="end"))
    out.append(k.text((X0 + X1) / 2, Y1 + 42, "每个权重占用的位数", 13, k.MUTED))
    out.append(k.text(X0 - 48, (Y0 + Y1) / 2 - 8, "相对均方根误差", 13, k.MUTED,
                      anchor="end"))
    out.append(k.text(X0 - 48, (Y0 + Y1) / 2 + 10, "实测数据", 11.5, k.MUTED,
                      anchor="end"))
    out.append(k.text(X0 - 48, (Y0 + Y1) / 2 + 26, "norm 权重，全为正", 11.5, k.MUTED,
                      anchor="end"))
    return out


def point(bits, err, color, head, body, dx=16):
    x, y = px(bits), py(err)
    return [f'<circle cx="{x:.1f}" cy="{y:.1f}" r="7" fill="{color}"/>',
            k.text(x + dx, y - 3, head, 14, color, "bold", anchor="start"),
            k.text(x + dx, y + 15, body, 12, k.MUTED, anchor="start")]


def arrow(x1, y1, x2, y2, color):
    """A dashed shaft ending in a small head, drawn axis-aligned only."""
    s = 7.0
    if x1 == x2:
        d = 1 if y2 > y1 else -1
        head = [(x2, y2), (x2 - s * 0.6, y2 - d * s), (x2 + s * 0.6, y2 - d * s)]
        shaft = k.line(x1, y1, x2, y2 - d * s, color, 1.8, dash="6 5")
    else:
        d = 1 if x2 > x1 else -1
        head = [(x2, y2), (x2 - d * s, y2 - s * 0.6), (x2 - d * s, y2 + s * 0.6)]
        shaft = k.line(x1, y1, x2 - d * s, y2, color, 1.8, dash="6 5")
    pts = " ".join(f"{a:.1f},{b:.1f}" for a, b in head)
    return [shaft, f'<polygon points="{pts}" fill="{color}"/>']


def build():
    out = axes()
    out += point(*Q4_0, RED, "Q4_0：4.50 位，4.25%", "每 32 个权重 18 字节，只有步长")
    out += point(*Q4_1, GREEN, "Q4_1：5.00 位，0.94%", "每 32 个权重 20 字节，多存一个偏移 m")

    # the corner neither format reaches
    tx, ty = px(Q4_0[0]), py(Q4_1[1])
    out.append(f'<circle cx="{tx:.1f}" cy="{ty:.1f}" r="19" fill="#fbf2dc" '
               f'stroke="{GOLD}" stroke-width="2" stroke-dasharray="5 4"/>')
    out.append(k.text(tx, ty + 8, "?", 24, GOLD, "bold"))
    out.append(k.text(tx - 30, ty - 3, "Q4_0 的体积", 13.5, GOLD, "bold", anchor="end"))
    out.append(k.text(tx - 30, ty + 15, "Q4_1 的精度", 13.5, GOLD, "bold", anchor="end"))
    out += arrow(px(Q4_0[0]), py(Q4_0[1]) + 12, tx, ty - 23, GOLD)
    out += arrow(px(Q4_1[0]) - 12, py(Q4_1[1]), tx + 23, ty, GOLD)
    return k.svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "q4-dilemma.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
