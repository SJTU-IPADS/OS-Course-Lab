#!/usr/bin/env python3
"""cmpl runs a subtraction and testl an AND; neither writes its result back.

In both panels the result is crossed out and only the flag lines reach
RFLAGS. testl sets ZF and SF from the AND and clears CF and OF.
Run it to refresh ../assets/cmp-test.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE,
                    MONO, MUTED, ORANGE, RED, WHITE, arrow, box, cells, line,
                    mono, rect, save, text)

W, H = 1120, 180


def panel(x0, insn, op, sym, flags):
    out = [rect(x0, 4, 540, 172, WHITE, LINE, rx=10, width=1.4),
           mono(x0 + 20, 30, insn, 19, INK, "bold")]
    out += box(x0 + 20, 46, 80, 42, "S1", FILL_BLUE, BLUE, 17, font=MONO)
    out += box(x0 + 20, 112, 80, 42, "S2", FILL_BLUE, BLUE, 17, font=MONO)
    out += box(x0 + 150, 62, 110, 76, sym, FILL_GREY, INK, 26, sub=op)
    out.append(arrow(x0 + 100, 67, x0 + 146, 86, INK, 2))
    out.append(arrow(x0 + 100, 133, x0 + 146, 114, INK, 2))
    # the result goes nowhere
    out.append(arrow(x0 + 260, 82, x0 + 316, 56, MUTED, 2, "5 4"))
    out.append(rect(x0 + 320, 34, 150, 40, FILL_GREY, LINE, rx=4, width=1.2))
    out.append(text(x0 + 395, 60, "运算结果", 15, MUTED))
    out.append(line(x0 + 330, 40, x0 + 460, 68, RED, 2.4))
    out.append(text(x0 + 480, 60, "丢弃", 15, RED, "bold", anchor="start"))
    # the flags go to RFLAGS
    out.append(arrow(x0 + 260, 118, x0 + 316, 128, ORANGE, 2.4))
    out += cells(x0 + 320, 108, [f for f, _ in flags], 50, 40,
                 [fill for _, fill in flags], ORANGE, 15)[0]
    out.append(text(x0 + 420, 166, "RFLAGS", 14, ORANGE, "bold"))
    return out


def build():
    cmp_flags = [(f, FILL_ORANGE) for f in ("ZF", "SF", "CF", "OF")]
    test_flags = [("ZF", FILL_ORANGE), ("SF", FILL_ORANGE), ("CF=0", WHITE),
                  ("OF=0", WHITE)]
    return (panel(10, "cmpl S2, S1", "S1 − S2", "−", cmp_flags)
            + panel(570, "testl S2, S1", "S1 & S2", "&", test_flags))


if __name__ == "__main__":
    save("cmp-test", W, H, build())
