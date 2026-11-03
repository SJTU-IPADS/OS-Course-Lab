#!/usr/bin/env python3
"""Copy Fail changes a shared file-cache copy while its disk file stays intact.

The figure is conceptual and contains no exploit operation or data. The red
cell denotes an unauthorised change, not a concrete instruction or byte value.
Source: https://xint.io/blog/copy-fail-linux-distributions (2026-04-29).
Run this script to refresh ../assets/copy-fail.svg.
"""

from svgkit import (
    BLUE, FILL_BLUE, FILL_GREEN, FILL_RED, GREEN, INK,
    MONO, MUTED, RED, WHITE, arrow, box, rect, save, text,
)


W, H = 1000, 240


def file_cells(x, y, changed):
    """Four symbolic portions of one file, with the changed portion outlined last."""
    out = []
    cell_w, cell_h = 46, 32
    for index in range(4):
        active = changed and index == 3
        fill = FILL_RED if active else (FILL_BLUE if changed else FILL_GREEN)
        stroke = RED if active else (BLUE if changed else GREEN)
        out.append(rect(x + index * cell_w, y, cell_w, cell_h,
                        fill, stroke, rx=0, width=1.3))
        out.append(text(x + (index + .5) * cell_w, y + 22,
                        "修改" if active else "···", 18,
                        RED if active else INK, "bold"))
    if changed:
        out.append(rect(x + 3 * cell_w, y, cell_w, cell_h,
                        "none", RED, rx=0, width=2.3))
    return out


def build():
    out = [
        rect(18, 68, 240, 146, FILL_GREEN, GREEN, width=1.8),
        text(138, 97, "磁盘", 22, INK, "bold"),
        text(138, 125, "/usr/bin/su", 19, INK, font=MONO),
        rect(380, 68, 280, 146, FILL_BLUE, BLUE, width=1.8),
        text(520, 97, "内存中的 page cache", 22, INK, "bold"),
        text(520, 125, "/usr/bin/su 的副本", 19, INK),
    ]
    out += file_cells(46, 142, changed=False)
    out += file_cells(428, 142, changed=True)
    out += [
        text(138, 199, "原始内容保持不变", 19, GREEN, "bold"),
        text(520, 199, "缓存内容已被修改", 19, RED, "bold"),
        arrow(272, 142, 366, 142, MUTED, 2),
        text(319, 125, "原先取入", 17, MUTED),
        arrow(674, 142, 778, 142, BLUE, 2),
        text(726, 125, "读取", 18, BLUE, "bold"),
        text(520, 27, "内核缺陷越权改写缓存", 21, RED, "bold"),
        arrow(520, 37, 520, 59, RED, 2),
    ]
    out += box(792, 98, 190, 89, "执行 su", WHITE, INK,
               size=23, sub="使用已修改的副本", sub_size=18)
    return out


if __name__ == "__main__":
    save("copy-fail", W, H, build())
