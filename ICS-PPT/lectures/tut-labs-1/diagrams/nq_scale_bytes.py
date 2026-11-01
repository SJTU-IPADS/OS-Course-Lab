#!/usr/bin/env python3
"""The 12 scale bytes of Q4_K: where the 6 bits of each sc[j] and m[j] go.

Top, the twelve bytes scales[0..11] as get_scale_min in impl/nano_quant.cpp
reads them: the low 6 bits of bytes 0..3 hold sc[0..3], of bytes 4..7 hold
m[0..3]; the four pairs j = 4..7 are split, their high 2 bits in the top of
bytes 0..7 and their low 4 bits in bytes 8..11. sc is green and m gold, as
lecture 2's q4k_scale_bits.py colours them; a split field is drawn with a
dashed edge. Below, the three bytes that hold the pairs 0 and 4, bit by
bit. Run it to refresh ../assets/nq_scale_bytes.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import INK, LINE, MONO, MUTED, line, rect, save, text

W, H = 1120, 200
GOLD, FILL_GOLD = "#b8860b", "#fbf2dc"
GREEN, FILL_GREEN = "#196b24", "#e7f2e4"
SPLIT_GOLD, SPLIT_GREEN = "#fdf7e3", "#f1f8ef"

BX, BY, BW, BH = 56, 14, 84, 40            # the twelve bytes
BIT = BW / 8
ZY, ZH, ZBIT = 112, 40, 30                 # the three bytes blown up
ZXS = (56, 416, 776)


def byte(x, y, w, parts):
    """One byte split into runs of bits: parts = [(bits, fill, stroke, label, dashed)]."""
    out = []
    bit = w / 8
    for bits, fill, stroke, label, dashed in parts:
        bw = bits * bit
        out.append(rect(x, y, bw, BH if y == BY else ZH, fill, stroke, rx=2, width=1.3,
                        dash="4 3" if dashed else None))
        if label:
            out.append(text(x + bw / 2, y + (BH if y == BY else ZH) / 2 + 4.5, label,
                            11.5 if y == BY else 13, INK, "bold" if not dashed else "normal",
                            font=MONO))
        x += bw
    return out


def build():
    out = []
    for i in range(12):
        x = BX + i * BW
        if i < 4:
            parts = [(2, SPLIT_GREEN, GREEN, "", True), (6, FILL_GREEN, GREEN, f"sc[{i}]", False)]
        elif i < 8:
            parts = [(2, SPLIT_GOLD, GOLD, "", True), (6, FILL_GOLD, GOLD, f"m[{i - 4}]", False)]
        else:
            parts = [(4, SPLIT_GOLD, GOLD, "", True), (4, SPLIT_GREEN, GREEN, "", True)]
        out += byte(x, BY, BW, parts)
        out.append(text(x + BW / 2, BY + BH + 16, f"字节 {i}", 11.5, MUTED))

    zoom = [(0, [(2, SPLIT_GREEN, GREEN, "sc[4]", True), (6, FILL_GREEN, GREEN, "sc[0]", False)],
             "位 7..6：sc[4] 的位 5..4", "位 5..0：sc[0]"),
            (4, [(2, SPLIT_GOLD, GOLD, "m[4]", True), (6, FILL_GOLD, GOLD, "m[0]", False)],
             "位 7..6：m[4] 的位 5..4", "位 5..0：m[0]"),
            (8, [(4, SPLIT_GOLD, GOLD, "m[4]", True), (4, SPLIT_GREEN, GREEN, "sc[4]", True)],
             "位 7..4：m[4] 的位 3..0", "位 3..0：sc[4] 的位 3..0")]
    for (i, parts, hi, lo), zx in zip(zoom, ZXS):
        zw = 8 * ZBIT
        out += byte(zx, ZY, zw, parts)
        for b in range(8):
            out.append(text(zx + b * ZBIT + ZBIT / 2, ZY - 7, f"b{7 - b}", 10.5, MUTED))
        out.append(text(zx, ZY + ZH + 18, f"字节 {i}", 12.5, INK, "bold", anchor="start"))
        out.append(text(zx + zw, ZY + ZH + 18, hi, 11.5, MUTED, anchor="end"))
        out.append(text(zx + zw, ZY + ZH + 36, lo, 11.5, MUTED, anchor="end"))
        x = BX + i * BW
        out.append(line(x, BY + BH, zx, ZY, LINE, 1.1, dash="4 4"))
        out.append(line(x + BW, BY + BH, zx + zw, ZY, LINE, 1.1, dash="4 4"))
    return out


if __name__ == "__main__":
    save("nq_scale_bytes", W, H, build())
