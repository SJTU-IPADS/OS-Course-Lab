#!/usr/bin/env python3
"""An access of 8 bytes at 0x1237c spans two lines.

The example of the TODO of cache_read: 8 bytes at line offset 60 are the
last 4 bytes of the line 0x12340 (set 13, tag 0x12 with 64 sets) and the
first 4 of the line 0x12380 (set 14, tag 0x12). cache_read and cache_write
handle one line per pass: count = min(64 - offset, bytes left), one
cache_access per pass, so the access counts twice. cache_read copies out
of the lines into dst; cache_write copies src into them and sets dirty in
both lines.
Run this script to refresh ../assets/cache-span.svg.
"""

from cache_kit import *

W, H = 1040, 300
LX, LY, BW, LH = 60, 128, 4, 34          # the two lines: 64 bytes of BW pixels each
SPAN = [(60, 63), (64, 67)]              # byte numbers from 0x12340
CW = 26                                  # a byte of dst or src
GAP = 8                                  # drawn between the two lines


def bx_of(byte):
    """x of a byte numbered from 0x12340; the second line starts after GAP."""
    return LX + byte * BW + (GAP if byte >= 64 else 0)


def lines():
    out = []
    for k, (addr, set_index) in enumerate((("0x12340", 13), ("0x12380", 14))):
        x = bx_of(64 * k)
        out.append(rect(x, LY, 64 * BW, LH, LINE_FILL, LINE_EDGE, rx=0, width=2))
        for b in range(4, 64, 4):
            out.append(line(x + b * BW, LY + LH - 6, x + b * BW, LY + LH, LINE_EDGE, 1))
        out.append(mono(x + 64 * BW / 2, LY + LH + 22, addr, 15, INK, "bold", anchor="middle"))
        out.append(text(x + 64 * BW / 2, LY + LH + 42, f"组 {set_index}", 15, IDX, "bold"))
    for first, last in SPAN:
        out.append(rect(bx_of(first), LY + 4, (last - first + 1) * BW, LH - 8, ORANGE,
                        ORANGE, rx=1, width=1))
    return out


def buffer(y, name, into_lines):
    """dst or src: 8 cells centred over the boundary of the two lines."""
    out = []
    bx = LX + 64 * BW + GAP / 2 - 4 * CW
    for k in range(8):
        out.append(rect(bx + k * CW, y, CW, 26, FILL_OFF, OFF, rx=0, width=1.2))
    out.append(mono(bx + 8 * CW + 10, y + 18, name, 15, OFF, "bold"))
    for half, (first, last) in enumerate(SPAN):
        sx = bx_of(first) + (last - first + 1) * BW / 2
        hx = bx + (half * 4 + 2) * CW
        if into_lines:
            out.append(arrow(hx, y - 2, sx, LY + LH + 3, ORANGE, 2))
        else:
            out.append(arrow(sx, LY - 3, hx, y + 28, ORANGE, 2))
    return out


def passes():
    out = []
    x = 680
    out.append(text(x - 46, 66, "逐行处理：每一轮一行", 15, INK, "bold", "start"))
    for k, (pa, rest) in enumerate((("0x1237c", "组 13，offset 60，4 字节"),
                                    ("0x12380", "组 14，offset 0，4 字节"))):
        y = 108 + k * 62
        out.append(text(x - 12, y + 5, f"第 {k + 1} 轮", 15, INK, "bold", "end"))
        shapes, _, x1 = fn(x, y, f"cache_access({pa})", 14)
        out += shapes
        out.append(text(x, y + 30, rest, 14, MUTED, "normal", "start"))
    out.append(text(x - 46, 244, "两行各计一次命中或缺失", 15, INK, "bold", "start"))
    return out


def build():
    out = lines()
    shapes, _, x1 = fn(LX, 40, "cache_read", 15)
    out += shapes
    out.append(mono(x1 + 6, 45, "(0x1237c, dst, 8)", 15, INK))
    out += buffer(64, "dst", False)
    shapes, _, x1 = fn(LX, 262, "cache_write", 15)
    out += shapes
    out.append(mono(x1 + 6, 267, "(0x1237c, src, 8)", 15, INK))
    out += buffer(222, "src", True)
    out.append(text(LX + 128 * BW + GAP, 267, "两行都置 dirty = 1", 14, RED, "bold", "end"))
    out += passes()
    return out


if __name__ == "__main__":
    save("cache-span", W, H, build())
