#!/usr/bin/env python3
"""An alternative m-bit address split, high to low: set index, tag, offset.

Field widths, colours and typography match cache_address.py. Run this script
to refresh ../assets/cache-address-high-index.svg.
"""

from cache_address import W, WB, WS, WT, X0, Y
from cachekit import FIELD_COLOR, FIELD_FILL, INK, brace, rect, save, text


H = 150


def build():
    out = []
    x = X0
    for name, width, label in (("idx", WS, "组索引 s 位"),
                               ("tag", WT, "标记 t 位"),
                               ("off", WB, "块偏移 b 位")):
        out.append(rect(x, Y, width, 58, FIELD_FILL[name], FIELD_COLOR[name],
                        rx=3, width=2.2))
        out.append(text(x + width / 2, Y + 29 + 22 * 0.36, label,
                        22, FIELD_COLOR[name], "bold"))
        x += width
    out += brace(X0, X0 + WT + WS + WB, Y - 8, INK, "地址 A：m 位",
                 20, below=False)
    return out


if __name__ == "__main__":
    save("cache-address-high-index", W, H, build())
