#!/usr/bin/env python3
"""scale: from two parameters to GPT-3's 175 billion.

One frame. Left: linear regression, a line set by two knobs. Right: GPT-3's
parameters as a wall of weights; under it the two risks of a model that
large and the answer deep learning gives to both.
"""
import hashlib
import math as m

from tk import *

H = 380


def rnd(seed, i):
    return hashlib.md5(f"{seed}:{i}".encode()).digest()[0] / 255.0


def knob(cx, cy, a, r=20, color=BLUE):
    tx = cx + (r - 4) * m.sin(m.radians(a))
    ty = cy - (r - 4) * m.cos(m.radians(a))
    out = [circle(cx, cy, r, WHITE, INK, 1.8), line(cx, cy, tx, ty, color, 3)]
    for t in range(-120, 121, 40):
        ox, oy = cx + (r + 4) * m.sin(m.radians(t)), cy - (r + 4) * m.cos(m.radians(t))
        ix, iy = cx + (r + 8) * m.sin(m.radians(t)), cy - (r + 8) * m.cos(m.radians(t))
        out.append(line(ox, oy, ix, iy, LINE, 1.4))
    return out


b = []
# left: two parameters
x0, y0, w, h = 56, 216, 250, 176
b += axes(x0, y0, w, h)
a, k = 0.2, 0.6
for i in range(14):
    u = 0.12 + 0.82 * rnd("sx", i)
    v = a + k * u + 0.14 * (rnd("sy", i) - 0.5) * 2
    b.append(dot(x0 + u * (w - 16), y0 - v * (h - 20), 4, DATA))
b.append(line(x0, y0 - a * (h - 20), x0 + (w - 16), y0 - (a + k) * (h - 20), BLUE, 2.6))
for cx, ang, s in ((128, 35, "斜率"), (236, -50, "截距")):
    b += knob(cx, 270, ang)
    b.append(label(cx, 322, s, 18, INK))
b.append(label(182, 362, "2 个参数", 20, INK, "bold"))

b.append(arrow(336, 150, 392, 150, INK, 2.4))

# right: GPT-3
gx, gy, cols, rows, cs = 420, 54, 84, 20, 7
b.append(label(gx + cols * cs / 2, 34, "GPT-3：175,181,291,520 个参数", 20, INK, "bold"))
b += weights(gx, gy, rows, cols, cs, cs, seed="gpt3-wall")
mid = gx + cols * cs / 2
r1, r2 = mid - 150, mid + 150
for cx, s in ((r1, "过拟合训练数据"), (r2, "无法有效训练")):
    pw = width(s, 19) + 32
    b.append(arrow(cx, gy + rows * cs + 6, cx, 224, RED, 1.8))
    b.append(rect(cx - pw / 2, 228, pw, 36, FILL_RED, RED, rx=6, width=1.6))
    b.append(label(cx, 252, s, 19, RED, "bold"))
    b.append(arrow(cx, 268, mid + (cx - mid) * 0.35, 300, GREEN, 1.8))
s = "深度学习：可扩展的结构 ＋ 反向传播"
pw = width(s, 20) + 40
b.append(rect(mid - pw / 2, 304, pw, 42, FILL_GREEN, GREEN, rx=6, width=1.6))
b.append(label(mid, 332, s, 20, GREEN, "bold"))
save("scale", b, H)
