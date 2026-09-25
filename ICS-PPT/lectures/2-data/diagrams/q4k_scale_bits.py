#!/usr/bin/env python3
"""Where one 6-bit Q4_K scale lives when 6 does not divide 8.

Twelve bytes hold sixteen 6-bit fields with no bit to spare, so half of them
straddle a byte boundary.  The figure follows one of those — sc[4] — from the
two bytes that hold its halves back to the value the decoder rebuilds.
Run it to refresh ../assets/q4k-scale-bits.svg.
"""

import pathlib

import svgkit as k

W, H = 1000, 300

GOLD, FILL_GOLD = "#b8860b", "#fbf2dc"
GREEN, FILL_GREEN = "#196b24", "#e7f2e4"

BX, BY, BW, BH = 80, 74, 70, 54           # 12 bytes of 70px = 840, x 80..920
BIT = BW / 8.0
JOIN_Y, JOIN_H, JOIN_W = 194, 46, 44      # the rebuilt 6-bit field
JOIN_X = (W - 6 * JOIN_W) / 2


def build():
    out = [k.text(W / 2, 30, "scales[0..11]：16 个 6 位字段，96 位占满 12 字节",
                  15.5, k.INK, "bold")]

    for i in range(12):
        x = BX + i * BW
        out.append(k.rect(x, BY, BW, BH, k.FILL_GREY, k.LINE, rx=3, width=1.2))
        out.append(k.text(x + BW / 2, BY - 10, f"Byte {i}", 11.5, k.MUTED))

    # Byte 0: the low six bits are one whole field, the high two are on loan
    out.append(k.rect(BX + 2 * BIT, BY, 6 * BIT, BH, FILL_GREEN, GREEN, rx=3, width=1.8))
    out.append(k.text(BX + 5 * BIT, BY + BH / 2 + 5, "sc[0]", 12.5, GREEN, "bold",
                      font=k.MONO))
    out.append(k.rect(BX, BY, 2 * BIT, BH, FILL_GOLD, GOLD, rx=3, width=1.8))

    # Byte 8: the low four bits are the other half of that same field
    b8 = BX + 8 * BW
    out.append(k.rect(b8 + 4 * BIT, BY, 4 * BIT, BH, FILL_GOLD, GOLD, rx=3, width=1.8))

    # what each highlighted run is
    out.append(k.text(BX + BIT, BY + BH + 20, "高 2 位", 12, GOLD, "bold"))
    out.append(k.text(BX + BIT, BY + BH + 36, "sc[4] 的 [5:4]", 11.5, GOLD))
    out.append(k.text(b8 + 6 * BIT, BY + BH + 20, "低 4 位", 12, GOLD, "bold"))
    out.append(k.text(b8 + 6 * BIT, BY + BH + 36, "sc[4] 的 [3:0]", 11.5, GOLD))
    out.append(k.line(BX + 5 * BIT, BY + BH, BX + 2.6 * BW, BY + BH + 14,
                      GREEN, 1.2, dash="4 3"))
    out.append(k.text(BX + 2.7 * BW, BY + BH + 19, "低 6 位整段就是 sc[0]", 12,
                      GREEN, "bold", anchor="start"))

    # the six cells the decoder puts back together
    for i in range(6):
        x = JOIN_X + i * JOIN_W
        out.append(k.rect(x, JOIN_Y, JOIN_W, JOIN_H, FILL_GOLD, GOLD, rx=3, width=1.6))
        out.append(k.text(x + JOIN_W / 2, JOIN_Y + JOIN_H / 2 + 6, f"b{5 - i}", 13,
                          k.INK, "bold", font=k.MONO))
    out.append(k.text(JOIN_X - 12, JOIN_Y + JOIN_H / 2 + 6, "sc[4]", 13.5, GOLD,
                      "bold", anchor="end", font=k.MONO))
    out += k.brace(JOIN_X, JOIN_X + 2 * JOIN_W, JOIN_Y + JOIN_H, GOLD,
                   "Byte 0 的高 2 位", 11.5)
    out += k.brace(JOIN_X + 2 * JOIN_W, JOIN_X + 6 * JOIN_W, JOIN_Y + JOIN_H, GOLD,
                   "Byte 8 的低 4 位", 11.5)

    # and the two splices that build it
    out.append(k.line(BX + BIT, BY + BH + 44, JOIN_X + JOIN_W, JOIN_Y - 6,
                      GOLD, 1.6, dash="5 4"))
    out.append(k.line(b8 + 6 * BIT, BY + BH + 44, JOIN_X + 4 * JOIN_W, JOIN_Y - 6,
                      GOLD, 1.6, dash="5 4"))

    out.append(k.text(W / 2, JOIN_Y + JOIN_H + 52,
                      "sc4 = (scales[8] & 0x0f) | ((scales[0] >> 6) << 4)",
                      14, k.INK, "bold", font=k.MONO))
    return k.svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "q4k-scale-bits.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
