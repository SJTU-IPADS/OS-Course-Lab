#!/usr/bin/env python3
"""The three fields of a physical address with --cache 64,4, and the
functions of cache.c that compute them.

The example is the one of src/lab2/cache.c and README.md: pa = 0x12345,
line number 0x48d, set index 13, tag 0x12, offset 5; the first byte of its
line is (0x12 * 64 + 13) * 64 = 0x12340. Bits 63 .. 20 of pa are zero and
are drawn as one cell; bits 19 .. 0 one cell each.
Run this script to refresh ../assets/cache-split.svg.
"""

from cache_kit import *

W, H = 1040, 300
X0, Y0, CW, CH = 64, 44, 34, 42
HIGH = 140                                  # the cell of bits 63 .. 20
PA = 0x12345


def bar():
    out = []
    bits = format(PA, "020b")
    fill, edge = FIELD["tag"]
    out.append(rect(X0, Y0, HIGH, CH, fill, edge, rx=0, width=1.2))
    out.append(mono(X0 + HIGH / 2, Y0 + CH / 2 + 6, "0 … 0", 18, TAG, "bold", anchor="middle"))
    x = X0 + HIGH
    xs = {}
    for k, b in enumerate(bits):
        bit = 19 - k
        name = "tag" if bit >= 12 else "idx" if bit >= 6 else "off"
        fill, edge = FIELD[name]
        out.append(rect(x, Y0, CW, CH, fill, edge, rx=0, width=1.2))
        out.append(mono(x + CW / 2, Y0 + CH / 2 + 7, b, 20, edge, "bold", anchor="middle"))
        xs[bit] = x
        x += CW
    # field outlines and bit numbers
    spans = {"tag": (X0, xs[11]), "idx": (xs[11], xs[5]), "off": (xs[5], x)}
    for name, (a, b) in spans.items():
        out.append(rect(a, Y0, b - a, CH, "none", FIELD[name][1], rx=2, width=2.6))
    for bit, bx in ((63, X0), (12, xs[12] + CW), (6, xs[6] + CW), (0, xs[0] + CW)):
        anchor = "start" if bit == 63 else "end"
        out.append(mono(bx + (4 if bit == 63 else -4), Y0 - 8, str(bit), 13, MUTED,
                        anchor=anchor))
    for bit, bx in ((11, xs[11]), (5, xs[5])):
        out.append(mono(bx + 4, Y0 - 8, str(bit), 13, MUTED))
    out.append(text(X0 - 14, Y0 + CH / 2 + 6, "pa", 18, INK, "bold", "end"))
    out.append(text(x + 14, Y0 + CH / 2 + 6, "= 0x12345", 18, INK, "bold", "start",
                    font=MONO))
    return out, spans


def outputs(spans):
    out = []
    t0, t1 = spans["tag"]
    s0, s1 = spans["idx"]
    o0, o1 = spans["off"]
    # the line number: tag and set together
    out += brace(t0 + 2, s1 - 2, Y0 + CH + 4, MUTED, "", depth=8)
    out.append(text((t0 + s1) / 2, Y0 + CH + 32, "行号 = pa / 64 = 0x48d", 15, INK, "bold"))
    y = Y0 + CH + 82
    for cx, name, value, rule, col in (((t0 + t1) / 2, "cache_tag(pa)", "= 0x12",
                                        "行号 / 64", TAG),
                                       ((s0 + s1) / 2 - 24, "cache_set_index(pa)", "= 13",
                                        "行号 % 64", IDX)):
        w = pill_width(name) + 8 + width_of(value, 17, MONO)
        x = cx - w / 2
        shapes, _, x1 = fn(x, y, name, 15)
        out += shapes
        out.append(mono(x1 + 8, y + 6, value, 17, col, "bold"))
        out.append(text(cx, y + 32, rule, 15, MUTED))
        out.append(arrow(cx, Y0 + CH + 40, cx, y - 16, col, 1.6))
    out.append(mono((o0 + o1) / 2, y + 6, "pa % 64 = 5", 17, OFF, "bold", anchor="middle"))
    out.append(text((o0 + o1) / 2, y + 32, "行内偏移", 15, MUTED))
    out.append(arrow((o0 + o1) / 2, Y0 + CH + 6, (o0 + o1) / 2, y - 14, OFF, 1.6))
    # back from tag and set to the first byte of the line
    y2 = y + 92
    name = "cache_line_address(0x12, 13)"
    rest = "= (0x12 × 64 + 13) × 64 = 0x12340"
    w = pill_width(name) + 10 + width_of(rest, 17, MONO)
    x = (t0 + o1) / 2 - w / 2
    shapes, x0, x1 = fn(x, y2, name, 15)
    out += shapes
    out.append(mono(x1 + 10, y2 + 6, rest, 17, INK, "bold"))
    out.append(text(x1 + 10 + width_of(rest, 17, MONO) / 2, y2 + 34,
                    "行的第一个字节：写回与读入一行都用这个地址", 15, MUTED))
    return out


def build():
    shapes, spans = bar()
    return shapes + outputs(spans)


if __name__ == "__main__":
    save("cache-split", W, H, build())
