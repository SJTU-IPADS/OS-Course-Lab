#!/usr/bin/env python3
"""The 16 instructions of the scalar int4 loop body, and the cycles they need.

gcc -O2 -fno-tree-vectorize compiles one iteration of dot_q4 (one byte of
weights, two multiply-adds) to 16 instructions: 4 multiply-add, 3 loads,
6 that unpack the two 4-bit weights, 3 of loop control. A Willow Cove core
issues at most 5 per cycle; cmp and jne fuse into one, so the 15 issue slots
take 3 cycles, i.e. at most 2/3 multiply-add per cycle.
Run it to refresh ../assets/insn-mix.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK,
                    MUTED, ORANGE, mono, rect, save, text)

W, H = 1120, 380

KIND = {"mac": (FILL_ORANGE, ORANGE, "乘加", "imul × 2、add × 2"),
        "load": (FILL_BLUE, BLUE, "读取", "movzbl 读权重、movsbl × 2 读激活值"),
        "unpack": (FILL_GREY, MUTED, "拆出 4 位权重", "mov、shr、and、movzbl、sub × 2"),
        "loop": (FILL_GREEN, GREEN, "循环控制", "add、cmp、jne")}

# the loop body in program order, as objdump prints it
BODY = [("movzbl", "load"), ("movsbl", "load"), ("mov", "unpack"), ("shr", "unpack"),
        ("and", "unpack"), ("movzbl", "unpack"), ("sub", "unpack"), ("sub", "unpack"),
        ("imul", "mac"), ("movsbl", "load"), ("add", "loop"), ("imul", "mac"),
        ("add", "mac"), ("add", "mac"), ("cmp", "loop"), ("jne", "loop")]


def cell(x, y, w, h, name, kind, size=15):
    fill, stroke = KIND[kind][:2]
    return [rect(x, y, w, h, fill, stroke, rx=2, width=1.6),
            mono(x + w / 2, y + h / 2 + size * 0.36, name, size, INK, "bold",
                 anchor="middle")]


def build():
    out = [text(20, 28, "标量循环体：16 条指令，每轮处理 1 字节权重（2 次乘加）", 17, INK, "bold",
                anchor="start")]
    cw = 1080 / len(BODY)
    for k, (name, kind) in enumerate(BODY):
        out += cell(20 + k * cw, 42, cw, 46, name, kind, 14)
    # legend
    for k, key in enumerate(("mac", "load", "unpack", "loop")):
        fill, stroke, head, body = KIND[key]
        n = sum(1 for _, kd in BODY if kd == key)
        x = 20 + k * 270
        out.append(rect(x, 110, 20, 20, fill, stroke, rx=3, width=1.4))
        out.append(text(x + 30, 126, f"{head} {n} 条", 16, INK, "bold", anchor="start"))
        out.append(text(x + 30, 150, body, 13, MUTED, anchor="start"))
    # issue slots: 5 per cycle, cmp + jne fused into one
    slots = [(n, kd) for n, kd in BODY[:-2]] + [("cmp+jne", "loop")]
    out.append(text(20, 200, "每个周期最多发射 5 条（cmp 与 jne 合并为 1 条）", 17, INK, "bold",
                    anchor="start"))
    for c in range(3):
        y = 216 + c * 50
        out.append(text(20, y + 28, f"周期 {c + 1}", 16, MUTED, anchor="start"))
        for k in range(5):
            name, kind = slots[c * 5 + k]
            out += cell(100 + k * 100, y, 94, 40, name, kind)
    tx = 660
    out.append(text(tx, 250, "每轮循环至少 3 个周期", 20, INK, "bold", anchor="start"))
    out.append(text(tx, 290, "每周期最多 2 ÷ 3 ≈ 0.67 次乘加", 20, ORANGE, "bold",
                    anchor="start"))
    out.append(text(tx, 326, "循环体 57 字节，保存在核心的指令缓存中", 15, MUTED, anchor="start"))
    return out


if __name__ == "__main__":
    save("insn-mix", W, H, build())
