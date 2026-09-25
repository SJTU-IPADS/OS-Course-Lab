#!/usr/bin/env python3
"""What w, q, d and m are, drawn on number lines.

One worked case: 4-bit unsigned codes q = 0..15 over weights in [-1.0, 2.0],
so d = 3.0 / 15 = 0.2 and m = -1.0, decoded as w = d*q + m (GGUF Q4_1).  The
top panel draws the real line and the integer line with every integer directly
under the grid point it decodes to, which makes q the index of a grid point,
d the spacing and m the value of q = 0.  The lower left enlarges one cell to
show rounding and the d/2 bound; the lower right redraws the grid with d
doubled and with m moved.
Run it to refresh ../assets/affine-grid.svg.
"""

import pathlib

import svgkit as k

W, H = 1100, 512

FILL_HOT = "#f8d5c2"

D, M, QMAX = 0.2, -1.0, 15
SAMPLE, OUTSIDE = 0.53, 2.37            # one weight inside the range, one past it


def wq(q, d=D, m=M):
    return d * q + m


def bracket(x0, x1, y, color, depth=7, up=True):
    s = -1 if up else 1
    return (f'<path d="M {x0:.1f} {y:.1f} L {x0:.1f} {y + s * depth:.1f} '
            f'L {x1:.1f} {y + s * depth:.1f} L {x1:.1f} {y:.1f}" fill="none" '
            f'stroke="{color}" stroke-width="1.4"/>')


def dot(x, y, color, r=5.5):
    return f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r}" fill="{color}"/>'


def arrow(x0, y0, x1, y1, color, dash="4 3"):
    """A dashed shaft with a filled head at (x1, y1)."""
    dx, dy = x1 - x0, y1 - y0
    n = (dx * dx + dy * dy) ** 0.5
    ux, uy = dx / n, dy / n
    hx, hy = x1 - ux * 8, y1 - uy * 8
    pts = f"{x1:.1f},{y1:.1f} {hx - uy * 4:.1f},{hy + ux * 4:.1f} " \
          f"{hx + uy * 4:.1f},{hy - ux * 4:.1f}"
    return [k.line(x0, y0, hx, hy, color, 1.5, dash),
            f'<polygon points="{pts}" fill="{color}"/>']


# ---------------------------------------------------------------- top panel

AX0, AX1, LO, HI = 150, 1060, -1.3, 2.6
REAL_Y, INT_Y = 124, 222


def px(v):
    return AX0 + (v - LO) / (HI - LO) * (AX1 - AX0)


def overview():
    out = [k.rich(20, 26, ["① 4 位无符号量化，权重范围 [−1.0, 2.0]：整数 ", ("v", "q"),
                         " 是实数轴上等距格点的编号"], 16.5, k.INK, "bold", "start")]

    # each grid point owns the half-open cell of width d around it
    for q in range(QMAX + 1):
        x0, x1 = px(wq(q) - D / 2), px(wq(q) + D / 2)
        fill = FILL_HOT if q == 8 else (k.FILL_BLUE if q % 2 else "#f7fafb")
        out.append(k.rect(x0, REAL_Y - 12, x1 - x0, 24, fill, "none", rx=0, width=0))
    out.append(k.line(AX0 - 12, REAL_Y, AX1 + 12, REAL_Y, k.LINE, 1.6))
    out.append(k.line(AX0 - 12, INT_Y, AX1 + 12, INT_Y, k.LINE, 1.6))
    out.append(k.rich(AX0 - 26, REAL_Y + 5, ["实数 ", ("v", "w")], 15, k.INK, "bold", "end"))
    out.append(k.rich(AX0 - 26, INT_Y + 5, ["整数 ", ("v", "q")], 15, k.INK, "bold", "end"))

    for q in range(QMAX + 1):
        x = px(wq(q))
        hot = q in (0, 8)
        c = k.ORANGE if hot else k.BLUE
        out.append(k.line(x, REAL_Y - 12, x, REAL_Y + 12, c, 2.0 if hot else 1.4))
        out.append(k.text(x, REAL_Y + 32, f"{wq(q):.1f}".replace("-", "−"), 12.5,
                          k.MUTED, font=k.MONO))
        out.append(k.line(x, INT_Y - 10, x, INT_Y + 10, c, 2.0 if hot else 1.4))
        out.append(k.text(x, INT_Y + 29, str(q), 14, c, "bold" if hot else "normal",
                          font=k.MONO))
        if q == 0:
            out.append(k.line(x, REAL_Y + 40, x, INT_Y - 14, k.ORANGE, 2.4))
        elif q != 8:
            out.append(k.line(x, REAL_Y + 40, x, INT_Y - 14, k.LINE, 1.0, "2 4"))
    x8 = px(wq(8))
    out.append(k.line(x8, REAL_Y + 40, x8, INT_Y - 14, k.ORANGE, 1.4, "5 3"))

    # the range and its 15 equal steps
    x_lo, x_hi = px(wq(0)), px(wq(QMAX))
    out.append(bracket(x_lo, x_hi, 76, k.BLUE))
    out.append(k.rich((x_lo + x_hi) / 2, 62,
                    [("v", "w"), ("_", "max"), " − ", ("v", "w"), ("_", "min"),
                     " = 3.0，等分为 ", ("v", "q"), ("_", "max"), " − ", ("v", "q"),
                     ("_", "min"), " = 15 段，每段长 ", ("v", "d"), " = 0.2"],
                    14.5, k.BLUE, "bold"))
    xa, xb = px(wq(1)), px(wq(2))
    out.append(bracket(xa, xb, REAL_Y - 16, k.BLUE))
    out.append(k.rich((xa + xb) / 2, REAL_Y - 29, [("v", "d")], 15, k.BLUE, "bold"))

    for q, name in ((0, "min"), (QMAX, "max")):
        out.append(k.rich(px(wq(q)), INT_Y + 50, [("v", "q"), ("_", name)], 13.5, k.MUTED))
    out.append(k.rich(px(wq(0)) + 32, INT_Y + 50,
                    [("v", "m"), " = −1.0：", ("v", "q"), " = 0 对应的实数值"],
                    14, k.ORANGE, "bold", "start"))

    # one weight inside the range, rounded to the nearest grid point
    xs = px(SAMPLE)
    out.append(dot(xs, REAL_Y - 40, k.ORANGE))
    out += arrow(xs, REAL_Y - 34, x8, REAL_Y - 13, k.ORANGE)
    out.append(k.rich(xs - 10, REAL_Y - 36, [("v", "w"), " = 0.53"], 14, k.ORANGE,
                    "bold", "end"))
    out.append(k.rich(x8 + 12, INT_Y - 32, ["→ ", ("v", "q"), " = 8"], 14, k.ORANGE,
                    "bold", "start"))

    # one weight past w_max, clipped to the last grid point
    xo = px(OUTSIDE)
    out.append(dot(xo, REAL_Y - 40, k.MUTED))
    out += arrow(xo, REAL_Y - 34, x_hi + 2, REAL_Y - 13, k.MUTED)
    out.append(k.rich(xo, REAL_Y - 52, [("v", "w"), " = 2.37"], 14, k.MUTED, "bold"))
    out.append(k.text(x_hi + 30, REAL_Y + 32, "超出范围", 13.5, k.MUTED, anchor="start"))
    out.append(k.rich(x_hi + 30, REAL_Y + 52, ["clip 到 ", ("v", "q"), " = 15"], 13.5,
                    k.MUTED, "normal", "start"))
    return out


# ------------------------------------------------------------ lower left

ZX0, ZX1, ZLO, ZHI, ZY = 40, 520, 0.3, 0.9, 400


def zx(v):
    return ZX0 + (v - ZLO) / (ZHI - ZLO) * (ZX1 - ZX0)


def zoom():
    out = [k.text(20, 314, "② 放大一格：四舍五入与误差上界", 16.5, k.INK, "bold",
                  "start")]
    for q in (7, 8, 9):
        x0, x1 = zx(wq(q) - D / 2), zx(wq(q) + D / 2)
        fill = FILL_HOT if q == 8 else (k.FILL_BLUE if q % 2 else "#f7fafb")
        out.append(k.rect(x0, ZY - 16, x1 - x0, 32, fill, "none", rx=0, width=0))
    out.append(k.line(ZX0 - 8, ZY, ZX1 + 8, ZY, k.LINE, 1.6))
    for q in (7, 8, 9):
        x = zx(wq(q))
        c = k.ORANGE if q == 8 else k.BLUE
        out.append(k.line(x, ZY - 16, x, ZY + 16, c, 2.0))
        out.append(k.text(x, ZY + 36, f"{wq(q):.1f}", 13, k.MUTED, font=k.MONO))
        out.append(k.rich(x, ZY + 58, [("v", "q"), f" = {q}"], 14, c,
                        "bold" if q == 8 else "normal"))

    # the sample, its distance to the grid point, and the half-cell bound
    xs, x8, xe = zx(SAMPLE), zx(wq(8)), zx(wq(8) + D / 2)
    out.append(dot(xs, ZY, k.ORANGE, 6))
    out.append(k.rich(xs, ZY - 26, [("v", "w"), " = 0.53"], 14, k.ORANGE, "bold"))
    out.append(bracket(xs, x8, ZY - 46, k.ORANGE, 6))
    out.append(k.rich((xs + x8) / 2 - 20, ZY - 58, ["误差 0.07"], 13.5, k.ORANGE,
                    "bold"))
    out.append(bracket(x8, xe, ZY - 46, k.BLUE, 6))
    out.append(k.rich((x8 + xe) / 2 + 18, ZY - 58, [("v", "d"), "/2 = 0.1"], 13.5,
                    k.BLUE, "bold"))

    out.append(k.rich(20, 484, ["编码：(0.53 − (−1.0)) / 0.2 = 7.65，四舍五入得 ",
                              ("v", "q"), " = 8"], 14, k.INK, "normal", "start"))
    out.append(k.rich(20, 506, ["解码：", ("v", "ŵ"), " = 0.2 × 8 + (−1.0) = 0.6；",
                              "橙色格内的 ", ("v", "w"), " 误差都不超过 ", ("v", "d"),
                              "/2"], 14, k.INK, "normal", "start"))
    return out


# ------------------------------------------------------------ lower right

CX0, CX1, CLO, CHI = 720, 1084, -1.5, 5.5
ROWS = ((0.2, -1.0, "基准"), (0.4, -1.0, "间距与范围加倍"), (0.2, 0.0, "网格整体平移"))


def cx(v):
    return CX0 + (v - CLO) / (CHI - CLO) * (CX1 - CX0)


def variants():
    out = [k.rich(570, 314, ["③ 改变 ", ("v", "d"), " 或 ", ("v", "m"), " 时网格的变化"],
                16.5, k.INK, "bold", "start")]
    x0 = cx(M)
    out.append(k.text(x0, 344, "−1.0", 13, k.MUTED, font=k.MONO))
    # broken under each row so it does not run through the end labels
    top = 350
    for i in range(len(ROWS)):
        y = 364 + i * 56
        out.append(k.line(x0, top, x0, y + 12, k.MUTED, 1.2, "3 3"))
        top = y + 30

    for i, (d, m, note) in enumerate(ROWS):
        y = 364 + i * 56
        out.append(k.rich(570, y + 1, [("v", "d"), f" = {d}，", ("v", "m"),
                                       " = " + f"{m:.1f}".replace("-", "−")],
                        14, k.INK, "bold", "start"))
        out.append(k.text(570, y + 21, note, 12.5, k.MUTED, anchor="start"))
        out.append(k.line(CX0 - 6, y, CX1 + 6, y, k.LINE, 1.4))
        for q in range(QMAX + 1):
            x = cx(wq(q, d, m))
            c = k.ORANGE if q == 0 else k.BLUE
            out.append(k.line(x, y - 9, x, y + 9, c, 2.2 if q == 0 else 1.3))
        for q in (0, QMAX):
            v = wq(q, d, m)
            out.append(k.text(cx(v), y + 24, f"{v:.1f}".replace("-", "−"), 11.5, k.MUTED,
                              font=k.MONO))
    return out


def build():
    return k.svg(W, H, overview() + zoom() + variants())


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "affine-grid.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
