#!/usr/bin/env python3
"""gpt2-gpt3: the same seed text continued by a small and a large model.

One frame. The seed "Behold, a wild pi creature, foraging in its native"
(the seed of both scenes in 3b1b generation.py; grey) feeds GPT-2 (a small block) and GPT-3 (the same
block scaled up). Each story is drawn as lines of text bars, since the
stories themselves are not recorded: GPT-2's lines break off (red marks),
GPT-3's run on and contain the phrase the narration points out.
"""
import hashlib

from tk import *

H = 390
TEXTBAR = "#c9d6de"


def rnd(seed, i):
    h = hashlib.md5(f"{seed}:{i}".encode()).digest()
    return h[0] / 255.0


def block(cx, bottom, w, h, d, name, size):
    """A box drawn with a top and a side face, front face centred on cx."""
    x0, y0 = cx - w / 2, bottom - h
    top = f"M {x0:.1f} {y0:.1f} L {x0 + d:.1f} {y0 - d:.1f} L {x0 + w + d:.1f} {y0 - d:.1f} L {x0 + w:.1f} {y0:.1f} Z"
    side = (f"M {x0 + w:.1f} {y0:.1f} L {x0 + w + d:.1f} {y0 - d:.1f} "
            f"L {x0 + w + d:.1f} {bottom - d:.1f} L {x0 + w:.1f} {bottom:.1f} Z")
    return [path(top, BLUE, 1.6, fill="#d5e6ee", head=False),
            path(side, BLUE, 1.6, fill="#c3dbe6", head=False),
            rect(x0, y0, w, h, FILL_BLUE, BLUE, rx=0, width=1.6),
            label(cx, y0 + h / 2 + size * 0.36, name, size, INK, "bold")]


def zigzag(x, y, color=RED):
    pts = [(x - 7, y + 7), (x - 2, y - 7), (x + 2, y + 7), (x + 7, y - 7)]
    d = "M " + " L ".join(f"{a:.1f} {b_:.1f}" for a, b_ in pts)
    return path(d, color, 2.2, head=False)


def text_line(x0, x1, y, seed, stop=None):
    """Word bars from x0 towards x1 (or `stop`); returns (elements, end x)."""
    out, x, i = [], x0, 0
    end = stop if stop is not None else x1
    while True:
        w = 18 + 46 * rnd(seed, i)
        if x + w > end:
            break
        out.append(rect(x, y - 4, w, 8, TEXTBAR, "none", rx=4, width=0))
        x += w + 8
        i += 1
    return out, x


def story(x, y, w, coherent):
    out = []
    h = 128
    out.append(rect(x, y, w, h, WHITE, LINE, rx=8, width=1.4))
    if coherent:
        out.append(label(x + 18, y + 26, "前后连贯", 18, GREEN, "bold", anchor="start"))
    else:
        out.append(label(x + 18, y + 26, "前后不连贯", 18, RED, "bold", anchor="start"))
    x0, x1 = x + 18, x + w - 18
    ys = [y + 50, y + 72, y + 94, y + 116]
    if coherent:
        for k, ly in enumerate(ys):
            if k == 2:
                els, xe = text_line(x0, x1, ly, "g3-2a", stop=x0 + 96)
                out += els
                pw = width("数学与计算之地", 17) + 16
                out.append(rect(xe, ly - 13, pw, 26, FILL_ORANGE, ORANGE, rx=4, width=1.4))
                out.append(label(xe + pw / 2, ly + 6, "数学与计算之地", 17, INK))
                els, _ = text_line(xe + pw + 8, x1, ly, "g3-2b")
                out += els
            else:
                els, _ = text_line(x0, x1 if k < 3 else x0 + 210, ly, f"g3-{k}")
                out += els
    else:
        # lines that stop and restart; a red mark at every break
        breaks = {0: (150, None), 1: (110, 230), 2: (None, None), 3: (190, None)}
        for k, ly in enumerate(ys):
            first, second = breaks[k]
            if first is None:
                els, _ = text_line(x0, x0 + 260, ly, f"g2-{k}")
                out += els
                continue
            els, xe = text_line(x0, x1, ly, f"g2-{k}a", stop=x0 + first)
            out += els
            out.append(zigzag(xe + 10, ly))
            if second:
                els, xe2 = text_line(xe + 28, x1, ly, f"g2-{k}b", stop=x0 + second)
                out += els
                out.append(zigzag(xe2 + 10, ly))
                els, _ = text_line(xe2 + 28, x1, ly, f"g2-{k}c", stop=x1 - 40)
                out += els
            else:
                els, _ = text_line(xe + 28, x1, ly, f"g2-{k}b", stop=x1 - 70)
                out += els
    return out


b = []
# the seed, shared
SEED = "Behold, a wild pi creature, foraging in its native"
sw, sh = width(SEED, 19) + 36, 40
sx, sy = 520 - sw / 2, 12
b.append(rect(sx, sy, sw, sh, FILL_DATA, DATA, rx=6, width=1.4))
b.append(label(520, sy + 27, SEED, 19, INK))

L, R = 260, 780          # column centres
BOTTOM = 186
b.append(line(520, sy + sh, 520, 66, INK, 2))
b.append(line(L, 66, R, 66, INK, 2))
b.append(arrow(L, 66, L, BOTTOM - 46 - 12 - 4, INK, 2))
b.append(arrow(R, 66, R, BOTTOM - 83 - 22 - 4, INK, 2))
b += block(L - 6, BOTTOM, 110, 46, 12, "GPT-2", 18)
b += block(R - 11, BOTTOM, 198, 83, 22, "GPT-3", 24)

PY = 206
b.append(arrow(L - 6, BOTTOM + 2, L - 6, PY - 4, INK, 2))
b.append(arrow(R - 11, BOTTOM + 2, R - 11, PY - 4, INK, 2))
b += story(L - 210, PY, 420, False)
b += story(R - 210, PY, 420, True)

b.append(label(520, 374, "同一种模型，规模不同", 20, INK, "bold"))
save("gpt2-gpt3", b, H)
