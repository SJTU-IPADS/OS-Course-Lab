#!/usr/bin/env python3
"""The stages of the rendering pipeline, and the data passed between them.

Left column, top down: vertices, the vertex shader, primitives (triangles),
the geometry shader, rasterization. Right column, top down: fragments, the
fragment shader, depth test and blending, the image. The three shader stages
run programs the developer writes and are filled orange; rasterization and
the tests and blending are fixed-function stages, drawn white. The four data
items are drawn as small pictures of the same two triangles: their vertices,
the triangles, the pixel cells they cover, the shaded cells.
Redrawn after the pipeline figure of Mingcong Han's GPU slides.
Run it to refresh ../assets/render-pipeline.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_ORANGE, INK, LINE, MUTED, ORANGE, WHITE, arrow,
                    circle, elbow, rect, save, text)

W, H = 560, 396
XL, XR = 150, 420                 # centres of the two columns
SW, SH = 220, 42                  # a stage box
IW, IH = 104, 70                  # a data picture
AL = 28                           # a down arrow
TOP = 8
CHANNEL = 285                     # x of the line from rasterization to the fragments

# The two triangles ABD and BCD, in the unit square of a picture.
PTS = {"A": (0.08, 0.80), "B": (0.38, 0.06), "C": (0.92, 0.30), "D": (0.66, 0.94)}
TRIS = [("A", "B", "D"), ("B", "C", "D")]
GRID = (13, 8)                    # cells of the fragment and image pictures

LEFT = [("data", "顶点"), ("stage", "顶点着色器", True), ("data", "图元"),
        ("stage", "几何着色器", True), ("stage", "光栅化", False)]
RIGHT = [("data", "片元"), ("stage", "片元着色器", True),
         ("stage", "深度测试与颜色混合", False), ("data", "图像")]
LABELS = {"顶点": ["顶点"], "图元": ["图元", "（三角形）"], "片元": ["片元"], "图像": ["图像"]}


def mix(c0, c1, t):
    a = [int(c0[k:k + 2], 16) for k in (1, 3, 5)]
    b = [int(c1[k:k + 2], 16) for k in (1, 3, 5)]
    return "#" + "".join(f"{round(p + (q - p) * t):02x}" for p, q in zip(a, b))


def inside(px, py, tri):
    (x1, y1), (x2, y2), (x3, y3) = (PTS[v] for v in tri)
    d1 = (px - x2) * (y1 - y2) - (x1 - x2) * (py - y2)
    d2 = (px - x3) * (y2 - y3) - (x2 - x3) * (py - y3)
    d3 = (px - x1) * (y3 - y1) - (x3 - x1) * (py - y1)
    neg, pos = d1 < 0 or d2 < 0 or d3 < 0, d1 > 0 or d2 > 0 or d3 > 0
    return not (neg and pos)


def picture(x, y, kind):
    """The data picture with its top-left corner at (x, y)."""
    out = [rect(x, y, IW, IH, WHITE, LINE, rx=3, width=1.2)]
    ix, iy, iw, ih = x + 10, y + 7, IW - 20, IH - 14

    def at(v):
        u, w = PTS[v]
        return ix + u * iw, iy + w * ih

    if kind in ("顶点", "图元"):
        if kind == "图元":
            for tri in TRIS:
                pts = " ".join(f"{at(v)[0]:.1f},{at(v)[1]:.1f}" for v in tri)
                out.append(f'<polygon points="{pts}" fill="none" stroke="{INK}" '
                           f'stroke-width="1.4" stroke-linejoin="round"/>')
        for v in PTS:
            out.append(circle(*at(v), 3.4, INK, INK, 1))
        return out
    cols, rows = GRID
    cw, ch = iw / cols, ih / rows
    for r in range(rows):
        for c in range(cols):
            u, w = (c + 0.5) / cols, (r + 0.5) / rows
            hit = [k for k, tri in enumerate(TRIS) if inside(u, w, tri)]
            fill = WHITE
            if hit and kind == "片元":
                fill = LINE
            elif hit:                       # shaded: one triangle lit, the other darker
                fill = mix(FILL_BLUE, BLUE, 0.25 + 0.45 * u if hit[0] == 0 else 0.6 + 0.35 * w)
            out.append(rect(ix + c * cw, iy + r * ch, cw, ch, fill, LINE, rx=0, width=0.6))
    return out


def column(xc, items, label_side):
    out, y, ends = [], TOP, []
    for k, item in enumerate(items):
        if k:
            out.append(arrow(xc, y, xc, y + AL - 2, INK, 2))
            y += AL
        if item[0] == "data":
            name = item[1]
            out += picture(xc - IW / 2, y, name)
            lines = LABELS[name]
            lx = xc - IW / 2 - 12 if label_side == "left" else xc + IW / 2 + 12
            anchor = "end" if label_side == "left" else "start"
            ly = y + IH / 2 + 6 - (len(lines) - 1) * 10
            for n, s in enumerate(lines):
                out.append(text(lx, ly + n * 20, s, 16, INK, "bold", anchor))
            ends.append((y, y + IH))
            y += IH
        else:
            _, name, shader = item
            fill, stroke = (FILL_ORANGE, ORANGE) if shader else (WHITE, INK)
            out.append(rect(xc - SW / 2, y, SW, SH, fill, stroke, rx=6, width=1.6))
            out.append(text(xc, y + SH / 2 + 6, name, 16, INK, "bold"))
            ends.append((y, y + SH))
            y += SH
    return out, ends, y


def build():
    out = []
    left, lends, _ = column(XL, LEFT, "left")
    right, rends, bottom = column(XR, RIGHT, "right")
    out += left + right
    # rasterization -> fragments
    ry = (lends[-1][0] + lends[-1][1]) / 2
    fy = (rends[0][0] + rends[0][1]) / 2
    out.append(elbow([(XL + SW / 2, ry), (CHANNEL, ry), (CHANNEL, fy),
                      (XR - IW / 2 - 3, fy)], INK, 2))
    # legend, under the right column
    lx, ly = XR - SW / 2, bottom + 40
    for n, (fill, stroke, s) in enumerate([(FILL_ORANGE, ORANGE, "可编程阶段：执行着色器程序"),
                                           (WHITE, INK, "固定功能阶段")]):
        y = ly + n * 28
        out.append(rect(lx, y - 13, 30, 17, fill, stroke, rx=3, width=1.4))
        out.append(text(lx + 40, y, s, 15, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("render-pipeline", W, H, build())
