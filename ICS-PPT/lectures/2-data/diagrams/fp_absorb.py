#!/usr/bin/env python3
"""The figure part-5.md asks for on its page 2: 2^24 + 1.0 in the adder.

Two operand registers, each an exponent and a 24-bit significand (the hidden
1 and the 23 stored fraction bits).  Aligning to the larger exponent shifts
1.0's significand right by 24 places, which carries its only 1 out of the
window into the band the adder drops; the sum is the larger operand again.
Run it to refresh ../assets/fp-absorb.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from svgkit import (BLUE, FILL_BLUE, FILL_GREY, INK, LINE, MUTED, ORANGE, brace,
                    cells, line, rect, svg, text)

W, H = 900, 226
RED, FILL_RED = "#c0392b", "#fbe3e0"

LABEL_X = 146                # labels end here
EXP_X, EXP_W = 156, 58       # exponent box
MAN_X, CW, CH = 224, 17, 24  # significand cells
BITS, DROP = 24, 4           # window width, dropped cells drawn
WIN_END = MAN_X + BITS * CW
DROP_END = WIN_END + DROP * CW
NOTE_X = DROP_END + 16

ROW1, ROW2, ROW3, ROW4 = 44, 80, 142, 188


def register(y, label, exp, bits, dropped=(), label_color=INK):
    out = [text(LABEL_X, y + CH / 2 + 5, label, 13.5, label_color, "bold", "end"),
           rect(EXP_X, y, EXP_W, CH, FILL_GREY, MUTED),
           text(EXP_X + EXP_W / 2, y + CH / 2 + 5, exp, 13, INK)]
    shapes, _ = cells(MAN_X, y, bits, CW, CH, FILL_BLUE, BLUE, 11.5)
    out += shapes
    for i, v in dropped:
        x = WIN_END + i * CW
        out.append(rect(x, y, CW, CH, FILL_RED, RED))
        out.append(text(x + CW / 2, y + CH / 2 + 4, v, 11.5, RED, "bold"))
    return out


def build():
    one = ["1"] + ["0"] * (BITS - 1)
    zero = ["0"] * BITS
    out = [
        # the band the adder drops, behind every row
        rect(WIN_END, 30, DROP * CW, H - 36, FILL_RED, RED, rx=4, width=1.2, dash="4 3"),
        text(WIN_END + DROP * CW / 2, 22, "舍弃区", 12.5, RED, "bold"),
        text(EXP_X + EXP_W / 2, 22, "阶码", 12.5, MUTED),
        text(MAN_X + CW / 2, 22, "隐含 1", 11.5, MUTED),
    ]
    out += brace(MAN_X + CW, WIN_END, 30, BLUE, "23 位尾数", 12.5, depth=5, below=False)
    out[-1] = text((MAN_X + CW + WIN_END) / 2, 22, "23 位尾数", 12.5, BLUE, "bold")

    out += register(ROW1, "大数 2²⁴", "24", one)
    out += register(ROW2, "小数 1.0", "0", one)
    out += register(ROW3, "对阶后的小数", "24", zero, dropped=[(0, "1")], label_color=ORANGE)

    # 1.0's leading 1 travels 24 places right, out of the window
    x0, y0 = MAN_X + CW / 2, ROW2 + CH
    x1, y1 = WIN_END + CW / 2, ROW3
    out.append(f'<path d="M {x0:.1f} {y0 + 2:.1f} C {x0:.1f} {y0 + 30:.1f}, '
               f'{x1:.1f} {y1 - 30:.1f}, {x1:.1f} {y1 - 7:.1f}" fill="none" '
               f'stroke="{ORANGE}" stroke-width="2"/>')
    out.append(f'<polygon points="{x1:.1f},{y1 - 1:.1f} {x1 - 5:.1f},{y1 - 9:.1f} '
               f'{x1 + 5:.1f},{y1 - 9:.1f}" fill="{ORANGE}"/>')
    out.append(text(NOTE_X, ROW2 + CH + 22, "对阶：尾数右移 24 位", 13, ORANGE, "bold", "start"))
    out.append(text(NOTE_X, ROW3 + CH / 2 + 5, "保护位：精确中点", 13, RED, "start"))

    rule = ROW4 - 8
    out.append(line(EXP_X, rule, WIN_END, rule, INK, 1.4))
    out += register(ROW4, "相加的结果", "24", one)
    out.append(text(NOTE_X, ROW4 + CH / 2 + 5, "2²⁴ ⊕ 1.0 = 2²⁴", 13, INK, "bold", "start"))
    return svg(W, H, out)


if __name__ == "__main__":
    path = pathlib.Path(__file__).resolve().parent.parent / "assets" / "fp-absorb.svg"
    path.write_text(build(), encoding="utf-8")
    print(path)
