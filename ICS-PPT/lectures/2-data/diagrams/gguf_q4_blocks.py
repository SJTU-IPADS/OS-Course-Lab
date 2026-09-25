#!/usr/bin/env python3
"""The bytes a GGUF Q4_0 and Q4_1 block occupy, drawn to scale.

Two memory strips, one cell per byte, so the 18 / 20 byte totals on the slide
are something you can count.  One byte of the Q4_0 payload is blown up to show
the two 4-bit fields packed into it.  Run it to refresh
../assets/gguf-q4-blocks.svg.
"""

import pathlib

import svgkit as k

W, H = 900, 320

GOLD, FILL_GOLD = "#b8860b", "#fbf2dc"

X0, CELL, CELL_H = 46, 40, 40
TOP_Y, BOT_Y = 62, 238
ZOOM_X, ZOOM_Y, BIT_W, BIT_H = 214, 142, 47, 40
FOCUS = 2                                   # qs[0]: the third byte of the block


def strip(y, fields):
    """One memory strip.  `fields` is a list of (count, fill, stroke, label)."""
    out, x, i = [], X0, 0
    for count, fill, stroke, label in fields:
        for _ in range(count):
            width = 2.2 if (y == TOP_Y and i == FOCUS) else 1.3
            out.append(k.rect(x, y, CELL, CELL_H, fill, stroke, rx=3, width=width))
            out.append(k.text(x + CELL / 2, y + CELL_H / 2 + 4.5, label, 11.5,
                              k.MUTED, font=k.MONO))
            x += CELL
            i += 1
    return out, x


def build():
    out = [k.text(W / 2, 26, "GGUF 4 位量化块：一个块覆盖 32 个权重", 15.5, k.INK, "bold")]

    # Q4_0: 2 bytes of scale + 16 bytes of packed codes
    body, end = strip(TOP_Y, [(2, k.FILL_ORANGE, k.ORANGE, "d"),
                              (16, k.FILL_BLUE, k.BLUE, "qs")])
    out += body
    out.append(k.text(X0 - 8, TOP_Y + CELL_H / 2 + 5, "Q4_0", 13.5, k.INK, "bold",
                      anchor="end"))
    out.append(k.text(X0 + CELL, TOP_Y - 12, "2 字节", 12, k.ORANGE, "bold"))
    out.append(k.text(X0 + 10 * CELL, TOP_Y - 12, "16 字节 · qs[0]..qs[15]", 12,
                      k.BLUE, "bold"))
    out.append(k.text(end, TOP_Y + CELL_H + 18,
                      "合计 18 字节 · 4.50 位/权重", 12.5, k.INK, "bold", anchor="end"))

    # the packed byte, blown up into its two nibbles
    fx = X0 + FOCUS * CELL
    for a, b in ((fx, ZOOM_X), (fx + CELL, ZOOM_X + 8 * BIT_W)):
        out.append(k.line(a, TOP_Y + CELL_H, b, ZOOM_Y, k.LINE, 1.1, dash="4 4"))
    for i in range(8):
        high = i < 4
        x = ZOOM_X + i * BIT_W
        out.append(k.rect(x, ZOOM_Y, BIT_W, BIT_H,
                          k.FILL_BLUE if high else k.FILL_ORANGE,
                          k.BLUE if high else k.ORANGE, rx=3, width=1.5))
        out.append(k.text(x + BIT_W / 2, ZOOM_Y + BIT_H / 2 + 5.5, "0110"[i % 4],
                          15, k.INK, "bold", font=k.MONO))
        out.append(k.text(x + BIT_W / 2, ZOOM_Y - 8, f"b{7 - i}", 11, k.MUTED))
    out.append(k.text(ZOOM_X + 2 * BIT_W, ZOOM_Y + BIT_H + 20, "高 4 位 → w[16]",
                      12.5, k.BLUE, "bold"))
    out.append(k.text(ZOOM_X + 6 * BIT_W, ZOOM_Y + BIT_H + 20, "低 4 位 → w[0]",
                      12.5, k.ORANGE, "bold"))
    out.append(k.text(ZOOM_X + 4 * BIT_W, ZOOM_Y + BIT_H + 40,
                      "第 j 个权重与第 j+16 个权重交叉配对", 12, k.MUTED))

    # Q4_1: the same payload, with a block minimum in front of it
    body, end = strip(BOT_Y, [(2, k.FILL_ORANGE, k.ORANGE, "d"),
                              (2, FILL_GOLD, GOLD, "m"),
                              (16, k.FILL_BLUE, k.BLUE, "qs")])
    out += body
    out.append(k.text(X0 - 8, BOT_Y + CELL_H / 2 + 5, "Q4_1", 13.5, k.INK, "bold",
                      anchor="end"))
    out.append(k.text(X0 + 3 * CELL, BOT_Y - 12, "+2 字节 · 块内最小值 m", 12,
                      GOLD, "bold"))
    out.append(k.text(end, BOT_Y + CELL_H + 18,
                      "合计 20 字节 · 5.00 位/权重", 12.5, k.INK, "bold", anchor="end"))

    return k.svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "gguf-q4-blocks.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
