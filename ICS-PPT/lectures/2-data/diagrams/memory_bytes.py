#!/usr/bin/env python3
"""Memory as one numbered run of bytes, and what an object's address means.

Shares the cell geometry with byte_order.py so that the byte-order figure
reads as the same row of addresses with the contents reordered.
Writes ../assets/memory-bytes.svg.
"""

import pathlib

import svgkit as k

W, H = 800, 208
CW, CH = 56.0, 36.0
X0, Y = 58, 72
N = 12


def build():
    out = []
    for i in range(N):
        x = X0 + i * CW
        out.append(k.text(x + CW / 2, Y - 12, f"0x{i:x}", 11, k.MUTED,
                          font=k.MONO))

    body, _ = k.cells(X0, Y, [""] * 1, CW, CH, k.FILL_BLUE, k.BLUE)
    out += body
    body, _ = k.cells(X0 + CW, Y, [""] * 3, CW, CH, "#ffffff", k.LINE)
    out += body
    body, _ = k.cells(X0 + 4 * CW, Y, [""] * 4, CW, CH, k.FILL_ORANGE, k.ORANGE)
    out += body
    body, right = k.cells(X0 + 8 * CW, Y, [""] * 4, CW, CH, "#ffffff", k.LINE)
    out += body
    out.append(k.text(right + 16, Y + CH / 2 + 5, "…", 18, k.MUTED, anchor="start"))

    out += k.brace(X0, X0 + CW, Y + CH + 6, k.BLUE, "char c")
    out += k.brace(X0 + 4 * CW, X0 + 8 * CW, Y + CH + 6, k.ORANGE, "int x · 4 个连续字节")

    out.append(k.text(W / 2, H - 20,
                      "&x = 0x4：多字节对象的地址，取它占用的最小的那一个",
                      14, k.INK, "bold"))
    return k.svg(W, H, out)


if __name__ == "__main__":
    p = pathlib.Path(__file__).resolve().parent.parent / "assets" / "memory-bytes.svg"
    p.write_text(build(), encoding="utf-8")
    print(p)
