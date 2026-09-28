#!/usr/bin/env python3
"""call and ret as the micro-operations they are, with the stack beside them.

The call site is A from the return-address figure: call at 0x4010, next
instruction at 0x4015, so 0x4015 is the value pushed and later popped. call is
above ret, tall and narrow, for the side column next to the text.
Run it to refresh ../assets/call-ret.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, arrow, mono, rect,
                    save, text)

W, H = 452, 648
PH = 294                            # panel height


def stack(x, y, top_label, top_fill, top_stroke, rsp_row):
    """Two stack slots; rsp_row is 0 (upper) or 1 (lower) for the %rsp arrow."""
    out = [rect(x, y, 200, 50, FILL_GREY, LINE, rx=2),
           text(x + 100, y + 31, "调用方的栈数据", 15, MUTED),
           rect(x, y + 50, 200, 50, top_fill, top_stroke, rx=2, width=1.8),
           mono(x + 100, y + 81, top_label, 17, INK, "bold", anchor="middle")]
    ry = y + 25 + rsp_row * 50
    out.append(arrow(x - 50, ry, x - 6, ry, INK, 2.2))
    out.append(mono(x - 54, ry + 6, "%rsp", 16, INK, "bold", anchor="end"))
    out.append(text(x + 210, y + 8, "高地址", 13, MUTED, anchor="start"))
    out.append(text(x + 210, y + 100, "低地址", 13, MUTED, anchor="start"))
    return out


def panel(y, title, color, fill, steps):
    out = [rect(16, y, 420, PH, WHITE, color, rx=10, width=1.8),
           rect(16, y, 420, 46, fill, "none", rx=10, width=0),
           mono(226, y + 31, title, 20, INK, "bold", anchor="middle")]
    for i, s in enumerate(steps):
        out.append(text(38, y + 82 + i * 32, s, 17, INK, anchor="start"))
    return out


def build():
    out = panel(20, "call dot_product", BLUE, FILL_BLUE,
                ["① %rsp ← %rsp − 8", "② 把返回地址 0x4015 写到 %rsp 所指处",
                 "③ %rip ← dot_product 的入口"])
    out += stack(120, 20 + 172, "0x4015", FILL_ORANGE, ORANGE, 1)
    out += panel(40 + PH, "ret", GREEN, FILL_GREEN,
                 ["① %rip ← %rsp 所指处的 8 字节（0x4015）", "② %rsp ← %rsp + 8",
                  "③ 从调用点的下一条指令继续执行"])
    out += stack(120, 40 + PH + 172, "0x4015（已弹出）", WHITE, LINE, 0)
    return out


if __name__ == "__main__":
    save("call-ret", W, H, build())
