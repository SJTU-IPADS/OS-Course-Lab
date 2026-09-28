#!/usr/bin/env python3
"""Zero extension against sign extension, bit by bit.

Left: movzbl takes the byte 0x9c and fills bits 31..8 with 0. Right: movslq
takes a 32-bit value and copies bit 31 into bits 63..32, once for a
non-negative value (0 copies 0) and once for a negative one (1 copies 1).
Long runs of equal bits are drawn as three cells, an ellipsis, three cells.
Run it to refresh ../assets/extension.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, arrow, brace, cells, mono,
                    path, rect, save, text)

W, H = 1120, 550
CH = 44                                 # cell height


def bits_row(x, y, groups, cw, ch=CH):
    """groups: [(bits string, fill, label)] drawn left to right."""
    out = []
    for s, fill, label in groups:
        cs, x2 = cells(x, y, list(s), cw, ch, fill, LINE, 20)
        out += cs
        if label:
            out.append(text((x + x2) / 2, y + ch + 24, label, 18, MUTED))
        x = x2
    return out, x


def zero_side():
    out = [rect(10, 10, 470, 530, WHITE, BLUE, rx=10, width=1.8),
           text(245, 52, "零扩展：movzbl", 24, INK, "bold"),
           text(245, 84, "高位全部填 0（unsigned）", 18, MUTED)]
    out.append(text(30, 170, "源：1 字节", 20, INK, "bold", anchor="start"))
    row, _ = bits_row(226, 140, [("10011100", FILL_BLUE, "0x9c")], 30)
    out += row
    out.append(arrow(346, 214, 346, 268, BLUE, 2.4))
    out.append(text(30, 300, "目的：4 字节", 20, INK, "bold", anchor="start"))
    # 32 bits shown as 3 zero bytes (condensed) + the source byte
    row, _ = bits_row(28, 316, [(list("000…000"), FILL_GREEN, ""),
                               ("10011100", FILL_BLUE, "")], 29)
    out += row
    out += brace(30, 229, 364, GREEN, "填充 24 个 0", 18)
    out += brace(234, 459, 364, BLUE, "源字节", 18)
    out.append(text(245, 470, "0x9c → 0x0000009c = 156", 21, INK, "bold"))
    out.append(text(245, 504, "数值按无符号解释保持不变", 18, MUTED))
    return out


def sign_side():
    x0 = 494
    out = [rect(x0, 10, 616, 530, WHITE, ORANGE, rx=10, width=1.8),
           text(x0 + 308, 52, "符号扩展：movslq", 24, INK, "bold"),
           text(x0 + 308, 84, "复制最高位（有符号补码）", 18, MUTED)]
    cases = [("5", "0", ["0", "…", "0", "1", "0", "1"], 160),
             ("-5", "1", ["1", "…", "1", "0", "1", "1"], 346)]
    for val, sign, low, y in cases:
        out.append(text(x0 + 18, y + 29, f"{val} ：", 21, INK, "bold", anchor="start"))
        cw = 38
        hx = x0 + 80
        # high 32 bits copied from the sign
        row, mid = bits_row(hx, y, [([sign] * 3 + ["…"] + [sign] * 3, FILL_ORANGE, "")],
                            cw)
        out += row
        row, end = bits_row(mid + 6, y, [(low[:1], FILL_BLUE, ""), (low[1:], WHITE, "")],
                            cw)
        out += row
        out += brace(hx + 2, mid - 2, y + CH + 4, ORANGE, f"高 32 位：复制 {sign}", 18)
        out += brace(mid + 8, end - 2, y + CH + 4, BLUE, "源：32 位", 18)
        sx = mid + 6 + cw / 2
        out.append(path(f"M {sx:.1f} {y - 2} C {sx:.1f} {y - 44}, "
                        f"{hx + 3.5 * cw:.1f} {y - 44}, {hx + 3.5 * cw:.1f} {y - 4}",
                        ORANGE, 2.2))
        out.append(text(sx + 14, y - 22, "第 31 位（符号位）", 17, ORANGE, "bold",
                        anchor="start"))
    out.append(text(x0 + 308, 504, "5 → 5，-5 → -5：补码数值不变", 21, INK, "bold"))
    return out


def build():
    return zero_side() + sign_side()


if __name__ == "__main__":
    save("extension", W, H, build())
