#!/usr/bin/env python3
"""Eight lane sums folded in half three times into one int in %ecx.

Level 1 adds the high 128 bits (vextracti128) to the low 128; levels 2 and 3
shift the xmm register right by 8 and 4 bytes (vpsrldq) and add again.
Run it to refresh ../assets/reduction-tree.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, line, mono, rect, save, text)

W, H = 1120, 380
X0, CW = 300, 100
RH = 40
LEVELS = [(20, [f"s{k}" for k in range(8)], "%ymm1", FILL_GREEN, GREEN),
          (110, [f"t{k}" for k in range(4)], "%xmm0", FILL_BLUE, BLUE),
          (200, ["u0", "u1"], "%xmm0", FILL_BLUE, BLUE),
          (290, ["v"], "%xmm0", FILL_ORANGE, ORANGE)]
STEPS = ["vextracti128 + vpaddd：t_k = s_k + s_(k+4)",
         "vpsrldq $8 + vpaddd：u_k = t_k + t_(k+2)",
         "vpsrldq $4 + vpaddd：v = u0 + u1"]


def cell_x(k):
    """Lane k, lane 0 rightmost, as in the register."""
    return X0 + (7 - k) * CW


def build():
    out = []
    for lvl, (y, names, reg, fill, stroke) in enumerate(LEVELS):
        out.append(mono(X0 - 20, y + 26, reg, 17, INK, "bold", anchor="end"))
        for k, n in enumerate(names):
            x = cell_x(k)
            out.append(rect(x + 6, y, CW - 12, RH, fill, stroke, rx=3, width=1.6))
            out.append(mono(x + CW / 2, y + 26, n, 17, INK, "bold", anchor="middle"))
        if lvl < 3:
            half = len(names) // 2
            ny = LEVELS[lvl + 1][0]
            for k in range(half):
                tx = cell_x(k) + CW / 2
                out.append(line(cell_x(k) + CW / 2, y + RH, tx, ny, stroke, 1.4))
                out.append(line(cell_x(k + half) + CW / 2, y + RH, tx + 2, ny, ORANGE, 1.4,
                                "5 3"))
            out.append(text(20, y + RH + 30, STEPS[lvl], 14, MUTED, anchor="start"))
    x = cell_x(0) + CW / 2
    out.append(line(x, 290 + RH, x, 350, ORANGE, 1.6))
    out.append(mono(x - 60, 370, "vmovd → %ecx", 17, ORANGE, "bold", anchor="middle"))
    return out


if __name__ == "__main__":
    save("reduction-tree", W, H, build())
