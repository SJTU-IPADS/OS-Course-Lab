#!/usr/bin/env python3
"""One attention head in a decoding step, with the shapes of every operand:
s = q K^T / sqrt(d), a = softmax(s), o = a V. Run it to refresh ../assets/kv-attention.svg."""

from kv_kit import *

W, H = 1000, 226
CY = 112                                 # the centre line of the 1 x t rows


def shape(x, w, h, label, dims, kind, y=None):
    y = CY - h / 2 if y is None else y
    out = cell(x, y, w, h, "", kind, size=16)
    out.append(text(x + w / 2, y - 10, label, 19, INK, "bold"))
    out.append(text(x + w / 2, y + h + 22, dims, 16, MUTED))
    return out


def op(x, s):
    return text(x, CY + 7, s, 24, INK, "bold")


def figure():
    out = []
    out += shape(20, 90, 24, "q", "1 × d", "new")
    out.append(op(136, "×"))
    out += shape(160, 150, 90, "Kᵀ", "d × t", "cache")
    out.append(op(334, "="))
    out += shape(356, 150, 24, "s", "1 × t", "mid")
    out.append(arrow(512, CY, 566, CY, INK, 2))
    out.append(text(539, CY - 12, "softmax", 15, MUTED))
    out += shape(572, 150, 24, "a", "1 × t", "mid")
    out.append(op(746, "×"))
    out += shape(770, 90, 150, "V", "t × d", "cache", y=CY - 75)
    out.append(op(886, "="))
    out += shape(906, 90, 24, "o", "1 × d", "new")
    return out


if __name__ == "__main__":
    save("kv-attention", W, H, figure())
