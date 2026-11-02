#!/usr/bin/env python3
"""Original three-vertex diagram of the RUM design dimensions.

Terminology follows Designing Access Methods: The RUM Conjecture,
EDBT 2016: https://www.openproceedings.org/2016/conf/edbt/paper-12.pdf
The geometry is drawn independently and does not reproduce a paper figure.
The labels identify design overheads without asserting a universal theorem.

Run this script to refresh ../assets/rum-tradeoff.svg.
"""

from svgkit import (
    BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE,
    GREEN, INK, LINE, MUTED, ORANGE, circle, path, save, text,
)


W, H = 380, 280
READ = (190, 53)
UPDATE = (45, 209)
MEMORY = (335, 209)


def build():
    out = [path("M 190 53 L 45 209 L 335 209 Z", LINE, 2,
                fill=FILL_GREY, head=False)]
    for (x, y), fill, stroke in [
        (READ, FILL_BLUE, BLUE),
        (UPDATE, FILL_ORANGE, ORANGE),
        (MEMORY, FILL_GREEN, GREEN),
    ]:
        out.append(circle(x, y, 6, fill, stroke, width=2))

    out.append(text(190, 27, "R：读开销", 18, BLUE, "bold"))
    out.append(text(86, 238, "U：更新开销", 18, ORANGE, "bold"))
    out.append(text(294, 238, "M：空间开销", 18, GREEN, "bold"))
    out.append(text(190, 155, "RUM Conjecture", 18, INK, "bold"))
    out.append(text(190, 270, "数据访问方法的设计猜想", 16, MUTED))
    return out


if __name__ == "__main__":
    save("rum-tradeoff", W, H, build())
