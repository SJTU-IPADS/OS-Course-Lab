#!/usr/bin/env python3
"""Three small figures, one per computational feature of a Transformer.

matmul   a matrix of token vectors times a weight matrix gives a matrix of the same shape;
weights  the vector of one token passes through the weights of all L layers;
step     a decoding step adds one row to X (orange), the earlier rows stay.
The three share one canvas size, so they line up side by side on the slide.
Run it to refresh ../assets/kv-feature-matmul.svg, kv-feature-weights.svg and
kv-feature-step.svg.
"""

from kv_kit import *
from kv_matrix import WORDS, sub_label

W, H = 330, 150
RH = 17                                   # row height of a matrix drawn as rows
N = 4


def matrix(x, top, w, kinds, prefix):
    out = []
    for i, kind in enumerate(kinds):
        y = top + i * RH
        out.append(rect(x, y, w, RH, *KINDS[kind], rx=2, width=1.3))
        out.append(mono(x + w / 2, y + RH / 2 + 4.3, f"{prefix}{i + 1}", 12, INK, "bold",
                        "middle"))
    return out


def matmul():
    """X (N x d) times W (d x d) gives Q (N x d)."""
    top = 56
    mid = top + N * RH / 2
    out = []
    out += matrix(24, top, 78, ["mid"] * N, "x")
    out.append(text(63, top - 10, "X：N × d", 14, INK, "bold"))
    out.append(text(120, mid + 7, "×", 20, MUTED, "bold"))
    out.append(rect(138, mid - 34, 68, 68, *KINDS["param"], rx=4, width=1.6))
    out += sub_label(172, mid + 6, "W", "Q", 17)
    out.append(text(172, top - 10, "d × d", 14, INK, "bold"))
    out.append(text(224, mid + 7, "=", 20, MUTED, "bold"))
    out += matrix(242, top, 78, ["mid"] * N, "q")
    out.append(text(281, top - 10, "Q：N × d", 14, INK, "bold"))
    return out


def weights():
    """One token vector goes through the weights of every layer."""
    cy = 82
    out = [rect(10, cy - RH / 2, 44, RH, *KINDS["mid"], rx=2, width=1.3),
           mono(32, cy + 4.3, "x", 12, INK, "bold", "middle")]
    x0, bw, pitch = 76, 32, 40
    names = ["1", "2", "⋯", "L"]
    for k, name in enumerate(names):
        x = x0 + k * pitch
        if name == "⋯":
            out.append(text(x + bw / 2, cy + 6, name, 18, MUTED, "bold"))
            continue
        out.append(rect(x, cy - 40, bw, 80, *KINDS["param"], rx=4, width=1.6))
        out.append(text(x + bw / 2, cy - 22, name, 14, INK, "bold"))
    right = x0 + (len(names) - 1) * pitch + bw
    out.append(arrow(56, cy, right + 24, cy, MUTED, 1.6))
    out += brace(x0, right, cy - 46, PURPLE, "L 层的权重", 14, below=False)
    out += cell(right + 28, cy - 14, 54, 28, "", "new")
    out.append(text(right + 55, cy + 5.5, WORDS[N], 15, INK, "bold"))
    return out


def step():
    """X in the prefill and in the first decoding step: one more row."""
    top = 46
    out = []
    for x, n, name in ((70, N, "预填充"), (226, N + 1, "解码第 1 步")):
        kinds = ["mid"] * N + ["new"] * (n - N)
        out += matrix(x, top, 78, kinds, "x")
        out.append(text(x + 39, top - 10, name, 14, INK, "bold"))
        for i in range(n):
            new = i >= N
            out.append(text(x - 7, top + i * RH + RH / 2 + 4.5, WORDS[i], 12,
                            ORANGE if new else INK, "bold" if new else "normal", "end"))
    out.append(arrow(154, top + N * RH / 2, 190, top + N * RH / 2, MUTED, 1.6))
    return out


if __name__ == "__main__":
    save("kv-feature-matmul", W, H, matmul())
    save("kv-feature-weights", W, H, weights())
    save("kv-feature-step", W, H, step())
