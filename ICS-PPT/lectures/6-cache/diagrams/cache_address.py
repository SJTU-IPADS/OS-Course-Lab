#!/usr/bin/env python3
"""An m-bit address as three fields, high to low: tag (t bits), set index (s bits),
block offset (b bits), each with the question it answers and the way its width
is computed. Run it to refresh ../assets/cache-address.svg.
"""

from cachekit import *

W, H = 1000, 310
X0, Y = 110, 66
WT, WS, WB = 380, 230, 190


def build():
    out = []
    shapes, boxes = addr_fields(X0, Y, WT, WS, WB, h=58, size=22)
    out += shapes
    out += brace(X0, X0 + WT + WS + WB, Y - 8, INK, "地址 A：m 位", 20, below=False)
    roles = {"tag": ("在选中的组内与各行的标记比较", "t = m − s − b"),
             "idx": ("选择第几个组", "s = log₂ S"),
             "off": ("选择块内的字节", "b = log₂ B")}
    for name, (role, formula) in roles.items():
        x0, x1 = boxes[name]
        cx = (x0 + x1) / 2
        col = FIELD_COLOR[name]
        out.append(arrow(cx, Y + 64, cx, Y + 118, col, 2.4))
        out.append(text(cx, Y + 150, role, 19, col, "bold"))
        out.append(text(cx, Y + 192, formula, 21, INK, font=MONO))
    return out


if __name__ == "__main__":
    save("cache-address", W, H, build())
