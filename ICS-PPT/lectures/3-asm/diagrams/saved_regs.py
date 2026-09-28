#!/usr/bin/env python3
"""The sixteen general registers split by who has to preserve them.

Nine caller-saved, six callee-saved; %rsp is the stack pointer and belongs to
neither group.
Run it to refresh ../assets/saved-regs.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN,
                    INK, LINE, MUTED, ORANGE, WHITE, mono, rect, save, text)

W, H = 1120, 270

CALLER = ["%rax", "%rcx", "%rdx", "%rsi", "%rdi", "%r8", "%r9", "%r10", "%r11"]
CALLEE = ["%rbx", "%rbp", "%r12", "%r13", "%r14", "%r15"]


def group(x, w, title, regs, cols, color, fill, lines):
    out = [rect(x, 20, w, 230, fill, color, rx=10, width=1.8),
           text(x + w / 2, 54, title, 19, color, "bold")]
    cw = (w - 40) / cols
    for i, r in enumerate(regs):
        cx = x + 20 + (i % cols) * cw
        cy = 74 + (i // cols) * 50
        out.append(rect(cx + 4, cy, cw - 8, 40, WHITE, color, rx=4, width=1.4))
        out.append(mono(cx + cw / 2, cy + 27, r, 18, INK, "bold", anchor="middle"))
    for i, s in enumerate(lines):
        out.append(text(x + w / 2, 198 + i * 28, s, 16, INK if i == 0 else MUTED,
                        "bold" if i == 0 else "normal"))
    return out


def build():
    out = group(20, 520, "调用者保存（Caller-saved）× 9", CALLER, 5, BLUE, FILL_BLUE,
                ["被调用者可以直接改写", "调用后还要用：调用者在调用前保存"])
    out += group(560, 380, "被调用者保存（Callee-saved）× 6", CALLEE, 3, GREEN,
                 FILL_GREEN, ["调用前后值必须相同", "要使用：入口保存、退出前恢复"])
    out.append(rect(960, 20, 140, 230, FILL_GREY, MUTED, rx=10, width=1.8))
    out.append(text(1030, 54, "栈指针", 19, INK, "bold"))
    out.append(rect(984, 74, 92, 40, WHITE, MUTED, rx=4, width=1.4))
    out.append(mono(1030, 101, "%rsp", 18, INK, "bold", anchor="middle"))
    out.append(text(1030, 198, "单独管理", 16, INK, "bold"))
    out.append(text(1030, 226, "见运行时栈", 16, MUTED))
    return out


if __name__ == "__main__":
    save("saved-regs", W, H, build())
