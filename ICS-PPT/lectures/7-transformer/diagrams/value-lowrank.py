#!/usr/bin/env python3
"""value-lowrank: the value map factored through the 128-dim space.

1: to scale against each other: W_Q and W_K (128 x 12,288, thin strips) and a
   full value map W_V (12,288 x 12,288 = 150,994,944, a square).
2: the factorisation E (12,288) -> Value_down (128 x 12,288) -> a 128-dim
   vector -> Value_up (12,288 x 128) -> value vector (12,288); the whole map
   still goes from the embedding space to itself, with rank at most 128.
   (The chain is drawn at its own ratio, 128 : 12,288 = 12 : 150 px.)
3: one head: four matrices of equal size, 4 x 1,572,864 = 6,291,456.
"""
from tk import *

H = 420
S = 192                     # 12,288 in frame 1: a strip is S / 96 = 2 px thick
SX, SY = 50, 76             # the W_V square
TX = SX + S + 44            # the strips
CY = 166                    # middle line of the chain
L, T = 150, 12              # 12,288 and 128 in the chain


def strip(x, y, w, h, seed, n):
    """A matrix too thin for cells: n shaded slices, no grid lines."""
    out = []
    for k in range(n):
        out.append(rect(x + k * w / n, y, w / n + 0.3, h, weight_fill(pseudo(seed, 0, k)),
                        "none", rx=0, width=0))
    return out


def slices_v(x, y, w, h, seed, n):
    out = []
    for k in range(n):
        out.append(rect(x, y + k * h / n, w, h / n + 0.3, weight_fill(pseudo(seed, k, 0)),
                        "none", rx=0, width=0))
    return out


def frame(n):
    b = []
    # ---- 1: sizes to scale
    b += weights(SX, SY, 16, 16, S / 16, S / 16, seed="WVfull")
    b += math(SX + S / 2, SY - 16, "W_V", 26, V)
    b.append(note(SX + S / 2, SY + S + 28, "12,288 × 12,288"))
    b.append(text(SX + S / 2, SY + S + 56, "150,994,944", 20, V, "bold"))
    for k, (name, color, seed) in enumerate((("W_Q", Q, "WQ"), ("W_K", K, "WK"))):
        y = SY + 40 + k * 70
        b += strip(TX, y, S, S / 96, seed, 48)
        b += math(TX, y - 12, name, 22, color, anchor="start")
        b.append(text(TX + S, y - 12, "1,572,864", 18, color, "bold", anchor="end"))
        b.append(note(TX + S / 2, y + 24, "128 × 12,288"))

    # ---- 2: the factored value map
    if n >= 2:
        ex = 530
        # E
        b.append(rect(ex, CY - L / 2, T, L, FILL_DATA, DATA, rx=1, width=1.6))
        b += math(ex + T / 2, CY + L / 2 + 28, "E", 22, DATA)
        b.append(note(ex + T / 2, CY + L / 2 + 52, "12,288"))
        x = ex + T + 8
        b.append(arrow(x, CY, x + 24, CY, INK, 1.8))
        # Value_down: 128 x 12,288
        dx = x + 32
        b += strip(dx, CY - T / 2, L, T, "Vdown", 30)
        b.append(rect(dx, CY - T / 2, L, T, "none", INK, rx=1, width=1.4))
        b += math(dx + L / 2, CY - T / 2 - 14, "\\up{Value}_↓", 22, V)
        b.append(note(dx + L / 2, CY + T / 2 + 26, "128 × 12,288"))
        x = dx + L + 8
        b.append(arrow(x, CY, x + 24, CY, INK, 1.8))
        # the 128-dim vector
        hx = x + 32
        b.append(rect(hx, CY - T / 2, T, T, FILL_V, V, rx=1, width=1.6))
        b.append(note(hx + T / 2, CY + T / 2 + 26, "128 维"))
        x = hx + T + 8
        b.append(arrow(x, CY, x + 24, CY, INK, 1.8))
        # Value_up: 12,288 x 128
        ux = x + 32
        b += slices_v(ux, CY - L / 2, T, L, "Vup", 30)
        b.append(rect(ux, CY - L / 2, T, L, "none", INK, rx=1, width=1.4))
        b += math(ux + T / 2, CY + L / 2 + 28, "\\up{Value}_↑", 22, V)
        b.append(note(ux + T / 2, CY + L / 2 + 52, "12,288 × 128"))
        x = ux + T + 8
        b.append(arrow(x, CY, x + 72, CY, INK, 1.8))
        # the value vector
        vx = x + 80
        b.append(rect(vx, CY - L / 2, T, L, FILL_V, V, rx=1, width=1.6))
        b.append(text(vx + T / 2, CY + L / 2 + 28, "值向量", 19, V))
        b.append(note(vx + T / 2, CY + L / 2 + 52, "12,288"))
        b += math((ex + vx + T) / 2, 58, "W_V = \\up{Value}_↑ \\up{Value}_↓", 24, V)
        b.append(text((ex + vx + T) / 2, CY + L / 2 + 118,
                      "整体仍是嵌入空间到嵌入空间的映射，秩至多为 128", 18, INK))

    # ---- 3: one head
    if n >= 3:
        b.append(text(dx + L / 2, CY + T / 2 + 50, "1,572,864", 18, V, "bold"))
        b.append(text(ux + T / 2, CY + L / 2 + 76, "1,572,864", 18, V, "bold"))
        b.append(rect(W / 2 - 330, H - 46, 660, 38, FILL_ORANGE, "none", rx=6, width=0))
        b.append(text(W / 2, H - 20, "一个头的四个矩阵大小相同：4 × 1,572,864 = 6,291,456", 20,
                      INK, "bold"))
    save(f"value-lowrank-{n}", b, H)


for n in (1, 2, 3):
    frame(n)
