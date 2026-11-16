#!/usr/bin/env python3
"""tasks: models built from Transformers, one task per row.

1: speech recognition, a waveform in, a transcript out.
2: + speech synthesis, text in, a waveform out.
3: + text to image, a prompt in, a picture out.
4: + machine translation, the task of the original 2017 Transformer.
5: + the variant studied here (orange frame): a passage of text, possibly
   with images or sound, in; a probability distribution over the text that
   comes next out.
The tasks are parallel examples, so the figure is drawn once with all five
rows; frame() keeps the step numbers only to say which rows are drawn.
"""
import math as m

from tk import *

H = 400
TOP, ROW, PITCH = 18, 62, 74
LBL = 22                       # task names, left aligned
IN_R = 460                     # inputs end here
BOX_L, BOX_W = 506, 150        # the Transformer box
OUT_L = 702                    # outputs start here


def wave(x, y, w, h, seed=1.0, color=DATA):
    """A sound waveform: thin vertical strokes under a varying envelope."""
    out = [line(x, y, x + w, y, LINE, 1)]
    n = int(w / 3)
    for i in range(n):
        t = i / n
        env = 0.25 + 0.75 * abs(m.sin(t * 9.0 + seed)) * (0.6 + 0.4 * m.sin(t * 23 + 2 * seed))
        a = h / 2 * env * (0.55 + 0.45 * abs(m.sin(i * 1.7 + seed)))
        xi = x + 1.5 + i * 3
        out.append(line(xi, y - a, xi, y + a, color, 1.6))
    return out


def textbox(x, y, lines, size=17, right=None, pad=12, lh=22):
    """A grey text box. `right` aligns its right edge; else x is its left edge."""
    w = max(width(s, size) for s in lines) + 2 * pad
    h = lh * len(lines) + 12
    left = right - w if right is not None else x
    out = [rect(left, y - h / 2, w, h, FILL_DATA, DATA, rx=6, width=1.4)]
    for i, s in enumerate(lines):
        ty = y - h / 2 + 6 + lh * (i + 0.5) + size * 0.36
        out.append(text(left + pad, ty, s, size, INK, anchor="start"))
    return out


def picture(x, y, w=112, h=58):
    """A framed picture: a ground line and a pi-shaped creature."""
    out = [rect(x, y, w, h, FILL_DATA, DATA, rx=4, width=1.6),
           line(x + 6, y + h - 9, x + w - 6, y + h - 9, LINE, 1.6)]
    cx = x + w / 2
    top = y + 19
    out.append(path(f"M {cx - 26:.1f} {top + 3:.1f} Q {cx:.1f} {top - 4:.1f} {cx + 26:.1f} {top + 2:.1f}",
                    INK, 5, head=False))
    out.append(path(f"M {cx - 10:.1f} {top + 2:.1f} Q {cx - 11:.1f} {top + 20:.1f} {cx - 17:.1f} {y + h - 10:.1f}",
                    INK, 4.5, head=False))
    out.append(path(f"M {cx + 10:.1f} {top + 2:.1f} Q {cx + 9:.1f} {top + 20:.1f} {cx + 15:.1f} {y + h - 10:.1f}",
                    INK, 4.5, head=False))
    for ex in (cx - 6, cx + 6):
        out.append(circle(ex, top - 8, 4, WHITE, INK, 1.2))
        out.append(circle(ex + 1, top - 7.5, 1.6, INK, INK, 0.5))
    return out


def model(cy, sub=None):
    out = [rect(BOX_L, cy - 25, BOX_W, 50, FILL_BLUE, BLUE, rx=6, width=1.6)]
    if sub:
        out.append(text(BOX_L + BOX_W / 2, cy - 3, "Transformer", 18, INK, "bold"))
        out.append(text(BOX_L + BOX_W / 2, cy + 18, sub, 15, MUTED))
    else:
        out.append(text(BOX_L + BOX_W / 2, cy + 6.5, "Transformer", 18, INK, "bold"))
    out.append(arrow(IN_R + 8, cy, BOX_L - 6, cy, INK, 2))
    out.append(arrow(BOX_L + BOX_W + 6, cy, OUT_L - 8, cy, INK, 2))
    return out


def row(i):
    top = TOP + i * PITCH
    cy = top + ROW / 2
    out = []
    if i == 0:
        out.append(label(LBL, cy + 7, "语音识别", 19, anchor="start"))
        out += wave(IN_R - 250, cy, 250, 50, 0.3)
        out += model(cy)
        out += textbox(OUT_L, cy, ["Some models take in audio", "and produce a transcript"])
    elif i == 1:
        out.append(label(LBL, cy + 7, "语音合成", 19, anchor="start"))
        out += textbox(0, cy, ["producing synthetic speech", "just from text"], right=IN_R)
        out += model(cy)
        out += wave(OUT_L, cy, 250, 50, 1.9)
    elif i == 2:
        out.append(label(LBL, cy + 7, "文生图", 19, anchor="start"))
        out += textbox(0, cy, ["1960s photograph of a cute fluffy",
                               "blue wild pi creature ..."], size=16, right=IN_R)
        out += model(cy)
        out += picture(OUT_L, cy - 29)
    elif i == 3:
        out.append(label(LBL, cy + 7, "机器翻译", 19, anchor="start"))
        out += textbox(0, cy, ["Attention is all you need"], size=18, right=IN_R)
        out += model(cy, "2017 年原始结构")
        out += textbox(OUT_L, cy, ["注意力就是你所需要的一切"], size=18)
    else:
        out.append(label(LBL, cy + 7, "预测下文", 19, ORANGE, "bold", anchor="start"))
        out += textbox(0, cy, ["一段文本（可附图像、声音）"], size=18, right=IN_R)
        out += model(cy)
        probs = [0.22, 0.09, 0.05, 0.04, 0.04, 0.04, 0.02, 0.02, 0.01]
        out += vbars(OUT_L + 4, cy + 24, probs, bw=10, gap=4, hmax=46)
        out.append(line(OUT_L, cy + 24, OUT_L + 128, cy + 24, LINE, 1.2))
        out.append(label(OUT_L + 140, cy + 7, "下一段文本的概率分布", 18, anchor="start"))
        out.append(rect(10, top - 6, W - 20, ROW + 12, "none", ORANGE, rx=8, width=2.2))
    return out


def frame(n):
    b = []
    rows = []
    for i in range(min(n, 4)):
        rows += row(i)
    b += rows
    if n == 5:
        b += row(4)
    save("tasks", b, H)


frame(5)
