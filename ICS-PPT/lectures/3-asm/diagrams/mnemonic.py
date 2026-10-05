#!/usr/bin/env python3
"""vpaddd and vpmulld taken apart, and the element-width suffixes.

Each part has its own fill; the label under it is what the part means. On
the right, the suffix sets the element width and so the lane count of a
256-bit register. The three groups stand in one row: the figure sits under
the text of its page and has about 170 px of height.
Run it to refresh ../assets/mnemonic.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE,
                    FILL_RED, GREEN, INK, LINE, MONO, MUTED, ORANGE, RED, WHITE,
                    arrow, mono, rect, save, text)

W, H = 1120, 170
TONE = {"v": (FILL_BLUE, BLUE), "p": (FILL_GREEN, GREEN), "op": (FILL_ORANGE, ORANGE),
        "l": (FILL_GREY, MUTED), "d": (FILL_RED, RED)}


def word(x, y, parts, size=34):
    """parts = [(letters, tone, label, sub)]; boxes sit side by side."""
    out = []
    for letters, tone, label, sub in parts:
        w = len(letters) * size * 0.6 + 30
        fill, stroke = TONE[tone]
        out.append(rect(x, y, w, 56, fill, stroke, rx=6, width=2))
        out.append(mono(x + w / 2, y + 40, letters, size, INK, "bold", anchor="middle"))
        out.append(arrow(x + w / 2, y + 60, x + w / 2, y + 82, stroke, 2))
        out.append(text(x + w / 2, y + 101, label, 15, stroke, "bold"))
        if sub:
            out.append(text(x + w / 2, y + 120, sub, 13, MUTED))
        x += w + 26
    return out


def build():
    out = [text(20, 20, "vpaddd", 16, INK, "bold", anchor="start", font=MONO)]
    out += word(44, 32, [("v", "v", "VEX 前缀", None),
                         ("p", "p", "Packed", "整型打包"),
                         ("add", "op", "加法操作", None),
                         ("d", "d", "32 位双字", "doubleword")])
    out.append(text(386, 20, "vpmulld", 16, INK, "bold", anchor="start", font=MONO))
    out += word(410, 32, [("v", "v", "Vector", None),
                          ("p", "p", "Packed", None),
                          ("mul", "op", "Multiply", None),
                          ("l", "l", "Low", "保留乘积低 32 位"),
                          ("d", "d", "Doubleword", None)])
    # the suffix table
    x0, tw = 842, 258
    out.append(text(x0, 20, "元素位宽后缀（256 位寄存器）", 16, INK, "bold", anchor="start"))
    rows = [("b", "byte", "8 位", 32), ("w", "word", "16 位", 16),
            ("d", "doubleword", "32 位", 8), ("q", "quadword", "64 位", 4)]
    for k, (s, name, bits, n) in enumerate(rows):
        y = 32 + k * 33
        hi = s == "d"
        out.append(rect(x0, y, tw, 29, FILL_RED if hi else WHITE, RED if hi else LINE,
                        rx=4, width=1.6))
        out.append(mono(x0 + 14, y + 21, s, 19, INK, "bold"))
        out.append(text(x0 + 42, y + 20, name, 14, INK, anchor="start"))
        out.append(text(x0 + 138, y + 20, bits, 14, INK, anchor="start"))
        out.append(text(x0 + tw - 12, y + 20, f"{n} 个通道", 14, MUTED, anchor="end"))
    return out


if __name__ == "__main__":
    save("mnemonic", W, H, build())
