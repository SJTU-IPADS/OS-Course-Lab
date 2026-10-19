#!/usr/bin/env python3
"""The helpers a case calls, between the bytes of the instruction and the
machine state (frame/x86.c, declared in include/mcpu.h).

Run it to refresh ../assets/cpu_helpers.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK,
                    MUTED, ORANGE, WHITE, arrow, rect, save, text)
from cpu_kit import code, note

W, H = 1120, 330


def group(x, y, w, h, title, rows, fill, stroke):
    out = [rect(x, y, w, h, fill, stroke, rx=8, width=1.4),
           text(x + 12, y + 22, title, 14, stroke, "bold", anchor="start")]
    for k, (fn, what) in enumerate(rows):
        yy = y + 46 + k * 24
        out.append(code(x + 12, yy, fn, 14, INK, "bold"))
        out.append(note(x + 12 + len(fn) * 8.6 + 8, yy, what, 13, INK))
    return out


def build():
    out = []
    # the instruction bytes
    out += group(20, 14, 360, 150, "指令的其余字节（R[RIP] 之后）",
                 [("fetch8/16/32/64", "读 1、2、4、8 字节，R[RIP] 前进"),
                  ("fetch_imm", "按 x.size 读立即数，符号扩展"),
                  ("modrm", "读 ModRM、SIB、位移，填 x.reg x.sub x.rm"),
                  ("ea", "内存操作数的地址；rip 相对时加 R[RIP]")],
                 WHITE, MUTED)
    # the case
    out.append(rect(410, 50, 300, 126, FILL_ORANGE, ORANGE, rx=8, width=1.6))
    out.append(text(560, 80, "case 的代码", 17, INK, "bold"))
    out.append(text(560, 106, "读字节 → 计算 → 写回", 14, INK))
    out.append(note(560, 132, "读 x.op x.size x.rex x.reg", 12, anchor="middle"))
    out.append(note(560, 150, "x.sub x.rm x.v x.vlen", 12, anchor="middle"))
    out.append(arrow(380, 90, 406, 100, MUTED, 2))
    out.append(arrow(714, 100, 736, 90, BLUE, 2, both=True))
    # the machine state
    out += group(740, 14, 360, 150, "寄存器与标志位",
                 [("get / set", "按 x.size 读写寄存器 n"),
                  ("R[...]", "直接读写：R[RSP]、R[RIP]、R[RFLAGS]"),
                  ("x86_alu", "运算编号 0..7 → alu.c，设置标志位"),
                  ("cond", "标志位是否满足条件码（你写）")],
                 FILL_BLUE, BLUE)
    out += group(20, 184, 360, 128, "rm 操作数与内存",
                 [("rm_get / rm_set", "rm 是寄存器时用 get/set"),
                  ("", "是内存时在 ea 处读写 x.size 字节"),
                  ("load / store", "地址 a 处的 n 字节；失败记缺页")],
                 FILL_GREEN, GREEN)
    out += group(740, 184, 360, 128, "向量寄存器 Y[]",
                 [("rm_vget", "读向量 rm 的 n 字节到 u256"),
                  ("rm_vset / vset", "写 n 字节；写寄存器时高位清零"),
                  ("Y[x.v]", "VEX 的第一个源操作数，直接读")],
                 FILL_GREEN, GREEN)
    out.append(arrow(380, 240, 420, 180, GREEN, 2, both=True))
    out.append(arrow(700, 180, 736, 240, GREEN, 2, both=True))
    out.append(note(560, 300, "声明都在 include/mcpu.h；第一个参数都是 &x", 13, anchor="middle"))
    return out


if __name__ == "__main__":
    save("cpu_helpers", W, H, build())
