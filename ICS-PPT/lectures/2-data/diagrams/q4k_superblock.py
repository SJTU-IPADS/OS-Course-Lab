#!/usr/bin/env python3
"""How a Q4_K superblock spends its 144 bytes.

Three levels: the superblock, the three fields it splits into, and the eight
sub-blocks the twelve bytes of secondary-quantized parameters steer.  The
bottom line says what the dashed lines to the sub-blocks mean: each sub-block's
step and offset are d and dmin times its two 6-bit integers.  Drawn flat, to
sit above the text of kquants-superblock-cont.  Run it to refresh
../assets/q4k-superblock.svg.
"""

import pathlib

import svgkit as k

W, H = 1000, 248

GOLD, FILL_GOLD = "#b8860b", "#fbf2dc"
GREEN, FILL_GREEN = "#196b24", "#e7f2e4"

TOP_X, TOP_Y, TOP_W, TOP_H = 70, 4, 860, 44
MID_Y, MID_H = 80, 58
SUB_Y, SUB_H = 172, 44
SUB_N, SUB_W, SUB_GAP = 8, 96, 12


def field(x, w, fill, stroke, head, body):
    return [k.rect(x, MID_Y, w, MID_H, fill, stroke, rx=4, width=1.6),
            k.text(x + w / 2, MID_Y + 24, head, 13.5, stroke, "bold"),
            k.text(x + w / 2, MID_Y + 43, body, 12, k.MUTED)]


def build():
    out = [k.rect(TOP_X, TOP_Y, TOP_W, TOP_H, FILL_GOLD, GOLD, rx=5, width=2.0),
           k.text(TOP_X + TOP_W / 2, TOP_Y + TOP_H / 2 + 6,
                  "Q4_K 超块：256 个权重 · 144 字节", 16, GOLD, "bold")]

    # the three fields, each box sized to hold its own label rather than to
    # its share of the 144 bytes — the counts are written on the boxes
    fields = [(400, k.FILL_BLUE, k.BLUE, "qs · 128 字节", "256 个 4 位权重编码"),
              (240, FILL_GREEN, GREEN, "scales · 12 字节", "8 个 sc 与 8 个 m（各 6 位）"),
              (200, k.FILL_ORANGE, k.ORANGE, "d, dmin · 4 字节", "sc 与 m 的量化步长（FP16）")]
    x = TOP_X
    for w, fill, stroke, head, body in fields:
        out += field(x, w, fill, stroke, head, body)
        out.append(k.line(TOP_X + TOP_W / 2, TOP_Y + TOP_H, x + w / 2, MID_Y,
                          k.LINE, 1.2, dash="4 4"))
        if fill is FILL_GREEN:
            scales_mid = x + w / 2
        x += w + 10

    # the eight sub-blocks the twelve bytes of parameters steer
    span = SUB_N * SUB_W + (SUB_N - 1) * SUB_GAP
    sx = (W - span) / 2
    for i in range(SUB_N):
        x = sx + i * (SUB_W + SUB_GAP)
        out.append(k.rect(x, SUB_Y, SUB_W, SUB_H, FILL_GREEN, GREEN, rx=4, width=1.4))
        out.append(k.text(x + SUB_W / 2, SUB_Y + 20, f"子块 {i}", 12.5, GREEN, "bold"))
        out.append(k.text(x + SUB_W / 2, SUB_Y + 36, "32 个权重", 11.5, k.MUTED))
        out.append(k.line(scales_mid, MID_Y + MID_H, x + SUB_W / 2, SUB_Y,
                          k.LINE, 1.0, dash="3 4"))

    out.append(k.text(W / 2, SUB_Y + SUB_H + 24,
                      "每个子块各有一个 sc 与一个 m：子块的步长 = d × sc，偏移 = dmin × m",
                      13.5, k.INK))

    return k.svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "q4k-superblock.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
