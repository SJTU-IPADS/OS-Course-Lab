#!/usr/bin/env python3
"""The exercise after symmetric-failure: six weights quantized to 4 bits twice.

The weights are -0.40, 0.00, 0.31, 0.58, 0.87 and 1.40, all in [-0.4, 1.4].
With m = 0, 1.40 maps to 7, so d = 0.2 and the 16 codes -8..7 span
[-1.6, 1.4]; -8 is drawn although no weight can reach it.  With m != 0 the
range is the data's own, q runs 0..15, d = 0.12 and m = -0.4.
Each figure has two lines: the real line w on top, marked every 0.2 from -1.6
to 1.4 and carrying the weights, and the integer line q below, whose ticks
stand at d*q + m on the same scale.  The real line carries no grid ticks; an
arrow runs from each weight to the grid point it rounds to.  Both figures use
the same real line, so the dots stay put and flipping between the two slides
shows the grid move.  Grid points outside the data range are grey and q = 0 is
orange; the label at the top counts the grid points inside the data range, and
a bracket over the grey ones counts those outside it.  The slide's bullet
states m, d and the range of q, so the figure repeats none of them.
The exercise slide shows the real line alone, drawn by the same code at the same
height, so the solution slides add the integer line below it.
Run it to refresh ../assets/quant-example-task.svg, quant-example-zero.svg and
quant-example-offset.svg.
"""

import pathlib

import svgkit as k

W, H, H_TASK = 1100, 186, 90         # the exercise figure keeps only the real line

FILL_DATA = "#fdf1e8"
GREY = "#b4c0c8"

WEIGHTS = (-0.40, 0.00, 0.31, 0.58, 0.87, 1.40)
LO, HI = -0.4, 1.4

AX0, AX1, VLO, VHI = 90, 1090, -1.7, 1.5
MARKS = [i / 5 for i in range(-8, 8)]      # the real line is marked every 0.2
LABEL_Y, REAL_Y, INT_Y = 16, 56, 150
BAND_TOP = 24


def px(v):
    return AX0 + (v - VLO) / (VHI - VLO) * (AX1 - AX0)


def minus(v, fmt):
    return format(v, fmt).replace("-", "−")


def arrow(x0, y0, x1, y1, color):
    """A dashed shaft with a filled head at (x1, y1)."""
    dx, dy = x1 - x0, y1 - y0
    n = (dx * dx + dy * dy) ** 0.5
    ux, uy = dx / n, dy / n
    hx, hy = x1 - ux * 9, y1 - uy * 9
    pts = f"{x1:.1f},{y1:.1f} {hx - uy * 4.5:.1f},{hy + ux * 4.5:.1f} " \
          f"{hx + uy * 4.5:.1f},{hy - ux * 4.5:.1f}"
    return [k.line(x0, y0, hx, hy, color, 1.6, "4 3"),
            f'<polygon points="{pts}" fill="{color}"/>']


def bracket(x0, x1, color):
    """A bracket just above the integer line, from x0 to x1."""
    y0, y1 = INT_Y - 16, INT_Y - 23
    return (f'<path d="M {x0:.1f} {y0} L {x0:.1f} {y1} L {x1:.1f} {y1} L {x1:.1f} {y0}" '
            f'fill="none" stroke="{color}" stroke-width="1.5"/>')


def real_line(label, h):
    """The data range, the real line marked every 0.2, and the weights with their values."""
    x0, x1 = px(LO), px(HI)
    out = [k.rect(x0, BAND_TOP, x1 - x0, h - BAND_TOP, FILL_DATA, "none", rx=0, width=0),
           k.text((x0 + x1) / 2, LABEL_Y, label, 16, k.ORANGE, "bold")]
    out.append(k.line(AX0, REAL_Y, AX1, REAL_Y, k.LINE, 1.5))
    out.append(k.rich(AX0 - 12, REAL_Y + 5, ["实数 ", ("v", "w")], 15, k.INK, "bold", "end"))
    for v in MARKS:
        out.append(k.line(px(v), REAL_Y - 5, px(v), REAL_Y + 5, k.MUTED, 1.2))
        out.append(k.text(px(v), REAL_Y - 11, minus(v, ".1f"), 12.5,
                          k.INK if v == 0 else k.MUTED, "bold" if v == 0 else "normal",
                          font=k.MONO))
    for w in WEIGHTS:
        out.append(f'<circle cx="{px(w):.1f}" cy="{REAL_Y}" r="6.5" fill="{k.ORANGE}"/>')
        out.append(k.text(px(w), REAL_Y + 27, minus(w, ".2f"), 16, k.ORANGE, "bold",
                          font=k.MONO))
    return out


def task():
    return k.svg(W, H_TASK, real_line("数据范围 [−0.4, 1.4]", H_TASK))


def figure(d, m, codes, step_at, m_note=None):
    grid = [(q, d * q + m) for q in codes]
    inside = [q for q, v in grid if LO - 1e-9 <= v <= HI + 1e-9]
    outside = [q for q in codes if q not in inside]
    out = real_line(f"数据范围 [−0.4, 1.4] 内 {len(inside)} 个格点", H)

    # the integer line: each code at the real value it decodes to
    out.append(k.line(AX0, INT_Y, AX1, INT_Y, k.LINE, 1.5))
    out.append(k.rich(AX0 - 12, INT_Y + 5, ["整数 ", ("v", "q")], 15, k.INK, "bold", "end"))
    for q, v in grid:
        c = k.ORANGE if q == 0 else (k.BLUE if q in inside else GREY)
        out.append(k.line(px(v), INT_Y - 11, px(v), INT_Y + 11, c, 2.8 if q == 0 else 2.2))
        out.append(k.text(px(v), INT_Y + 26, minus(q, "d"), 16, c,
                          "bold" if q == 0 else "normal", font=k.MONO))
    if m_note:
        out.append(k.rich(px(m) - 14, INT_Y + 26, m_note, 15, k.ORANGE, "bold", "end"))

    # the step, bracketed above two grid points where no arrow runs, and the grid
    # points outside the data range, bracketed where no arrow reaches
    a, b = px(grid[step_at][1]), px(grid[step_at + 1][1])
    out.append(bracket(a, b, k.BLUE))
    out.append(k.rich((a + b) / 2, INT_Y - 30, [("v", "d"), f" = {d:g}"], 17, k.BLUE,
                      "bold"))
    if outside:
        a, b = px(d * min(outside) + m), px(d * max(outside) + m)
        out.append(bracket(a, b, k.MUTED))
        out.append(k.text((a + b) / 2, INT_Y - 30, f"数据范围外 {len(outside)} 个格点", 16,
                          k.MUTED, "bold"))

    # each weight and the grid point it rounds to
    for w in WEIGHTS:
        q = min(codes, key=lambda c: abs(d * c + m - w))
        out += arrow(px(w), REAL_Y + 34, px(d * q + m), INT_Y - 13, k.ORANGE)
    return k.svg(W, H, out)


def zero():
    return figure(0.2, 0.0, range(-8, 8), 13)


def offset():
    return figure(0.12, -0.4, range(0, 16), 12,
                  [("v", "q"), " = 0 对应 ", ("v", "m"), " = −0.4"])


if __name__ == "__main__":
    assets = pathlib.Path(__file__).resolve().parent.parent / "assets"
    for name, build in (("quant-example-task.svg", task),
                        ("quant-example-zero.svg", zero),
                        ("quant-example-offset.svg", offset)):
        path = assets / name
        path.write_text(build(), encoding="utf-8")
        print(path)
