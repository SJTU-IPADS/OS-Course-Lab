#!/usr/bin/env python3
"""The memory mountain drawn from mountain.txt: a heat map, the ridge chart
(throughput against working set size at stride 1 and 8) and the slope chart
(throughput against stride for a working set in memory and one in L2).
mountain.txt is the output of `./mountain` with 16 strides; the capacity
boundaries are the cache sizes in the parameter listing (48 KB, 1280 KB, 24 MB).
Run it to refresh ../assets/mountain-image.svg, mountain-ridges.svg and
mountain-slopes.svg.
"""

import pathlib

from cdraw import *
from svgkit import vbrace

DATA = pathlib.Path(__file__).resolve().parent / "mountain.txt"


def load():
    """Returns (sizes, table): sizes from 8K to 128M, table[size][stride - 1] in GB/s."""
    lines = DATA.read_text().splitlines()[1:]
    sizes, table = [], {}
    for line in lines:
        tok = line.split()
        size = tok[0] + tok[1]                      # '128' 'M' -> '128M'
        sizes.append(size)
        table[size] = [float(v) for v in tok[2:]]
    sizes.reverse()                                  # small to large
    return sizes, table


SIZES, TABLE = load()
NSTRIDE = len(TABLE[SIZES[0]])
# capacities: the last size that fits in each level, from the parameter listing
LEVELS = [("L1", "48 KB", "32K"), ("L2", "1280 KB", "1M"), ("L3", "24 MB", "16M")]
LEVEL_FIRST = {"L1": 0}                              # filled below


def size_label(s):
    return s[:-1] + " " + s[-1] + "B"


def lerp(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


def hexc(c):
    return "#%02x%02x%02x" % tuple(int(round(v)) for v in c)


STOPS = [(0.0, (240, 246, 250)), (0.5, (91, 155, 184)), (1.0, (14, 40, 65))]
VMAX = 50.0


def colour(v):
    t = min(max(v / VMAX, 0), 1)
    for (t0, c0), (t1, c1) in zip(STOPS, STOPS[1:]):
        if t <= t1:
            return hexc(lerp(c0, c1, (t - t0) / (t1 - t0))), t
    return hexc(STOPS[-1][1]), t


def heat():
    W, H = 1100, 424
    x0, top, cw, rh = 124, 60, 50, 20
    rows = list(reversed(SIZES))                     # 128M at the top
    body = []
    body.append(text(x0 - 14, 50, "工作集", 17, MUTED, "bold", "end"))
    body.append(text(x0 + NSTRIDE * cw / 2, 20, "步长（单位：8 字节）", 17, MUTED, "bold"))
    for s in range(NSTRIDE):
        body.append(text(x0 + s * cw + cw / 2, 50, f"{s + 1}", 16, MUTED, "bold"))
    for r, size in enumerate(rows):
        y = top + r * rh
        body.append(text(x0 - 14, y + rh / 2 + 5, size_label(size), 16, INK, "bold", "end"))
        for s, v in enumerate(TABLE[size]):
            fill, t = colour(v)
            body.append(rect(x0 + s * cw, y, cw, rh, fill, WHITE, rx=0, width=1))
            body.append(text(x0 + s * cw + cw / 2, y + rh / 2 + 5, f"{v:.1f}", 14.5,
                             WHITE if t > 0.45 else INK))
    right = x0 + NSTRIDE * cw
    # capacity boundaries and level names
    edges = []
    names = []
    for i, (name, cap, last) in enumerate(LEVELS):
        edges.append(top + (len(rows) - SIZES.index(last) - 1) * rh)
    bounds = [top] + edges[::-1] + [top + len(rows) * rh]
    # bounds: top, L3/main, L2/L3, L1/L2, bottom
    labels = ["内存", "L3：24 MB", "L2：1280 KB", "L1：48 KB"]
    for y in edges:
        body.append(line(x0 - 4, y, right + 14, y, ORANGE, 2.4, dash="7 4"))
    for k, name in enumerate(labels):
        y0, y1 = bounds[k], bounds[k + 1]
        body += vbrace(right + 22, y0 + 3, y1 - 3, MUTED, name, 16, depth=8)
    # colour bar
    bx, by, bw = x0 + 160, 372, 480
    grad = ('<linearGradient id="mt" x1="0" x2="1" y1="0" y2="0">' +
            "".join(f'<stop offset="{t}" stop-color="{hexc(c)}"/>' for t, c in STOPS) +
            "</linearGradient>")
    body.append(rect(bx, by, bw, 18, "url(#mt)", LINE, rx=0, width=1))
    for v in range(0, 51, 10):
        x = bx + bw * v / VMAX
        body.append(line(x, by + 18, x, by + 24, LINE, 1.2))
        body.append(text(x, by + 40, f"{v}", 15, MUTED))
    body.append(text(bx - 14, by + 15, "读吞吐率 GB/s", 16, MUTED, "bold", "end"))
    save("mountain-image", W, H, body, defs=[grad])


# ---- line charts -------------------------------------------------------------

def axes(x0, x1, y0, y1, ymax, step, ylabel):
    """Plot frame with horizontal grid lines every `step`; returns shapes and y(v)."""
    body = []

    def y(v):
        return y1 - (y1 - y0) * v / ymax

    v = 0
    while v <= ymax + 1e-9:
        body.append(line(x0, y(v), x1, y(v), LINE if v else INK, 1 if v else 1.8,
                         dash=None if v == 0 else "3 4"))
        body.append(text(x0 - 10, y(v) + 5, f"{v:g}", 16, MUTED, anchor="end"))
        v += step
    body.append(text(x0 - 10, y0 - 14, ylabel, 16, MUTED, "bold", "end"))
    return body, y


def ridges():
    W, H = 1100, 352
    x0, x1, y0, y1 = 84, 1060, 70, 278
    n = len(SIZES)
    step = (x1 - x0) / n
    xc = lambda i: x0 + step * (i + 0.5)
    body, y = axes(x0, x1, y0, y1, 50, 10, "GB/s")
    # bands: the sizes up to each capacity belong to the level
    edge = [0] + [SIZES.index(last) + 1 for _, _, last in LEVELS] + [n]
    names = [("L1", "≤ 48 KB"), ("L2", "≤ 1280 KB"), ("L3", "≤ 24 MB"), ("内存", "")]
    fills = [FILL_BLUE, FILL_GREY, FILL_BLUE, FILL_GREEN]
    bands = []
    for k, (name, cap) in enumerate(names):
        bx0, bx1 = x0 + edge[k] * step, x0 + edge[k + 1] * step
        bands.append(rect(bx0, y0, bx1 - bx0, y1 - y0, fills[k], "none", rx=0, width=0))
        bands.append(text((bx0 + bx1) / 2, y0 - 30, name, 19, INK, "bold"))
        if cap:
            bands.append(text((bx0 + bx1) / 2, y0 - 10, cap, 15, MUTED))
    body = bands + body
    for k in range(1, 4):
        bx = x0 + edge[k] * step
        body.append(line(bx, y0, bx, y1, ORANGE, 2, dash="7 4"))
    for i, size in enumerate(SIZES):
        body.append(text(xc(i), y1 + 26, size[:-1] + size[-1], 16, INK))
    body.append(text((x0 + x1) / 2, y1 + 58, "工作集大小（字节）", 17, MUTED, "bold"))
    for stride, col, dy in ((1, BLUE, -10), (8, ORANGE, 20)):
        pts = [(xc(i), y(TABLE[s][stride - 1])) for i, s in enumerate(SIZES)]
        d = "M " + " L ".join(f"{px:.1f} {py:.1f}" for px, py in pts)
        body.append(f'<path d="{d}" fill="none" stroke="{col}" stroke-width="2.6"/>')
        for (px, py), s in zip(pts, SIZES):
            body.append(circle(px, py, 4.2, col, WHITE, 1.2))
    # legend
    lx, ly = x1 - 300, y1 + 54
    for k, (col, label) in enumerate(((BLUE, "步长 1"), (ORANGE, "步长 8"))):
        body.append(line(lx + k * 150, ly, lx + k * 150 + 36, ly, col, 2.6))
        body.append(circle(lx + k * 150 + 18, ly, 4.2, col, WHITE, 1.2))
        body.append(text(lx + k * 150 + 46, ly + 5, label, 17, INK, "normal", "start"))
    save("mountain-ridges", W, H, body)


def slopes():
    W, H = 1100, 396
    x0, x1, y0, y1 = 84, 1060, 44, 290
    nst = NSTRIDE
    step = (x1 - x0) / nst
    xc = lambda i: x0 + step * (i + 0.5)
    axes_body, y = axes(x0, x1, y0, y1, 50, 10, "GB/s")
    # from stride 8 on, every access is to a new 64-byte block
    bx = x0 + 7 * step
    body = [rect(bx, y0, x1 - bx, y1 - y0, FILL_ORANGE, "none", rx=0, width=0)] + axes_body
    body.append(text((bx + x1) / 2, y0 + 24, "步长 ≥ 8：每次访问一个新的块", 18, ORANGE, "bold"))
    for i in range(nst):
        body.append(text(xc(i), y1 + 26, f"{i + 1}", 16, INK))
    body.append(text((x0 + x1) / 2, y1 + 56, "步长（单位：8 字节）", 17, MUTED, "bold"))
    for size, col, label in (("256K", PURPLE, "工作集 256 KB（在 L2 中）"),
                             ("128M", GREEN, "工作集 128 MB（在内存中）")):
        vals = TABLE[size]
        pts = [(xc(i), y(v)) for i, v in enumerate(vals)]
        d = "M " + " L ".join(f"{px:.1f} {py:.1f}" for px, py in pts)
        body.append(f'<path d="{d}" fill="none" stroke="{col}" stroke-width="2.6"/>')
        for (px, py), v in zip(pts, vals):
            body.append(circle(px, py, 4.2, col, WHITE, 1.2))
    for k, (col, label) in enumerate(((PURPLE, "工作集 256 KB（在 L2 中）"),
                                      (GREEN, "工作集 128 MB（在内存中）"))):
        lx, ly = x0 + 120 + k * 380, y1 + 90
        body.append(line(lx, ly, lx + 36, ly, col, 2.6))
        body.append(circle(lx + 18, ly, 4.2, col, WHITE, 1.2))
        body.append(text(lx + 46, ly + 5, label, 17, INK, "normal", "start"))
    save("mountain-slopes", W, H, body)


def circle(cx, cy, r, fill, stroke, width=1.4):
    return (f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r:.1f}" fill="{fill}" '
            f'stroke="{stroke}" stroke-width="{width}"/>')


if __name__ == "__main__":
    heat()
    ridges()
    slopes()
