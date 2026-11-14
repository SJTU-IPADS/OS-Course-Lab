#!/usr/bin/env python3
"""tensors: the data of a deep network are arrays of real numbers.

1: a list (1D), a grid (2D) and a block (3D) of numbers: tensors.
2: + an input array passes through layer after layer, each one an array;
   the last array is a probability distribution over the next token.
"""
import hashlib

from tk import *

H = 400
NUMS = [[8.8, 3.3, 8.1, 0.4, 1.1, 5.9], [4.3, 7.3, 5.1, 5.7, 6.4, 9.8],
        [0.5, 7.1, 7.9, 7.3, 7.0, 5.4], [7.1, 9.8, 2.5, 6.6, 5.9, 7.1],
        [7.4, 7.2, 4.0, 9.8, 4.5, 3.7]]


def numgrid(x, y, rows, cols, cw, ch, size=15, r0=0):
    out = []
    for i in range(rows):
        for j in range(cols):
            out.append(rect(x + j * cw, y + i * ch, cw, ch, WHITE, LINE, rx=0, width=1))
            out.append(text(x + (j + .5) * cw, y + (i + .5) * ch + size * 0.36,
                            f"{NUMS[(i + r0) % 5][j]:.1f}", size, INK, font=MONO))
    out.append(rect(x - 3, y - 3, cols * cw + 6, rows * ch + 6, "none", DATA, rx=4, width=1.8))
    return out


def grey(seed, i, j):
    h = hashlib.md5(f"{seed}:{i}:{j}".encode()).digest()[0] / 255.0
    g = round(240 - 110 * h)
    return f"#{g - 10:02x}{g - 2:02x}{min(255, g + 6):02x}"


def cube(x, y, rows, cols, depth, cw, ch, dx, dy):
    """A block of cells: front face (numbers) at (x, y), receding by (dx, dy)."""
    out = []
    # top face
    for k in range(depth, 0, -1):
        for j in range(cols):
            x1, y1 = x + j * cw + k * dx, y + k * dy
            d = (f"M {x1:.1f} {y1:.1f} L {x1 + cw:.1f} {y1:.1f} L {x1 + cw - dx:.1f} {y1 - dy:.1f} "
                 f"L {x1 - dx:.1f} {y1 - dy:.1f} Z")
            out.append(path(d, LINE, 1, fill="#e4eaee", head=False))
    # right face
    for k in range(depth, 0, -1):
        for i in range(rows):
            x1, y1 = x + cols * cw + k * dx, y + i * ch + k * dy
            d = (f"M {x1:.1f} {y1:.1f} L {x1:.1f} {y1 + ch:.1f} L {x1 - dx:.1f} {y1 + ch - dy:.1f} "
                 f"L {x1 - dx:.1f} {y1 - dy:.1f} Z")
            out.append(path(d, LINE, 1, fill="#d8e1e7", head=False))
    out += numgrid(x, y, rows, cols, cw, ch)[:-1]
    edge = (f"M {x:.1f} {y:.1f} L {x + depth * dx:.1f} {y + depth * dy:.1f} "
            f"L {x + cols * cw + depth * dx:.1f} {y + depth * dy:.1f} "
            f"L {x + cols * cw + depth * dx:.1f} {y + rows * ch + depth * dy:.1f} "
            f"L {x + cols * cw:.1f} {y + rows * ch:.1f} L {x:.1f} {y + rows * ch:.1f} Z")
    out.append(path(edge, DATA, 1.8, head=False))
    out.append(line(x + cols * cw, y, x + cols * cw + depth * dx, y + depth * dy, DATA, 1.4))
    out.append(line(x + cols * cw, y, x + cols * cw, y + rows * ch, DATA, 1.4))
    out.append(line(x, y, x + cols * cw, y, DATA, 1.4))
    return out


def layer(x, y, seed, rows=5, cols=3, c=16):
    out = []
    for i in range(rows):
        for j in range(cols):
            out.append(rect(x + j * c, y + i * c, c, c, grey(seed, i, j), WHITE, rx=0, width=1))
    out.append(rect(x - 3, y - 3, cols * c + 6, rows * c + 6, "none", DATA, rx=4, width=1.8))
    return out


def frame(n):
    b = []
    # 1: three tensors
    cw, ch = 42, 26
    els, _ = hvector(110, 108, NUMS[0][:5], cw=cw, ch=30, size=15)
    b += els
    b += numgrid(410, 72, 4, 5, cw, ch)
    b += cube(712, 92, 3, 4, 3, cw, ch, 13, -9)
    for x, s in ((110 + 2.5 * cw, "一维"), (410 + 2.5 * cw, "二维"), (712 + 2.5 * cw, "三维")):
        b.append(label(x, 204, s, 18, MUTED))
    b += brace(100, 900, 50, INK, "张量", size=20, below=False)

    if n >= 2:
        y0 = 268
        x = 120
        b.append(label(x + 76, y0 - 16, "输入", 18, MUTED))
        b += numgrid(x, y0, 4, 4, 38, 24, size=15, r0=1)
        x += 4 * 38 + 10
        stages = [("第 1 层", "l1"), ("第 2 层", "l2"), ("⋯", None), ("最后一层", "lN")]
        for name, seed in stages:
            b.append(arrow(x, y0 + 48, x + 30, y0 + 48, INK, 2))
            x += 40
            if seed is None:
                b.append(label(x + 10, y0 + 55, "⋯", 22, MUTED))
                x += 30
                continue
            b += layer(x, y0 + 8, seed)
            b.append(label(x + 24, y0 - 16, name, 18, MUTED))
            x += 48 + 10
        b.append(arrow(x, y0 + 48, x + 30, y0 + 48, INK, 2))
        wx = x + 116
        b.append(label(wx + 70, y0 - 16, "下一个 token 的概率", 18, MUTED))
        items = [("land", .22), ("forest", .09), ("country", .05), ("habitat", .04)]
        els, hgt = hbars(wx, y0 + 2, items, bar_w=130, bar_h=18, gap=6, size=17)
        b += els
        b.append(label(wx - 30, y0 + hgt + 22, "⋮", 18, MUTED))
    save(f"tensors-{n}", b, H)


for n in (1, 2):
    frame(n)
