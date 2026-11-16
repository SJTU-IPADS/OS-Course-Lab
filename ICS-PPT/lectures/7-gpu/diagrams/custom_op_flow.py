#!/usr/bin/env python3
"""The four steps from a CUDA kernel to a Python call of a custom operator.

custom-op-flow.svg: left to right, ① the kernel and its C++ function in one
.cu file, ② the declaration and the per-key registration in the same file,
③ compiling and loading from Python (the load runs the registration),
④ the call through torch.ops. Blue boxes are C++ and CUDA source, the grey
box is the build, the green box is Python. A bracket under each side names it.
Run it to refresh ../assets/custom-op-flow.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_GREY, GREEN, INK, MUTED, arrow, brace,
                    mono, rect, save, text)

W, H = 1120, 212
BW, BH, GAP = 245, 130, 38
X0 = 13
TOP = 8

STEPS = [
    ("① 核函数与封装", "myadd.cu", ("核函数 add_kernel", "与 C++ 函数 myadd_cuda"),
     FILL_BLUE, BLUE),
    ("② 声明与注册", "TORCH_LIBRARY", ("声明 mylib::myadd，", "为 CUDA 登记 myadd_cuda"),
     FILL_BLUE, BLUE),
    ("③ 编译与加载", "cpp_extension.load", ("nvcc 编译成动态库，", "加载时执行注册"),
     FILL_GREY, MUTED),
    ("④ Python 调用", "torch.ops.mylib.myadd", ("Dispatcher 按 key", "选出 myadd_cuda"),
     FILL_GREEN, GREEN),
]


def build():
    out = []
    for k, (title, code, desc, fill, stroke) in enumerate(STEPS):
        x = X0 + k * (BW + GAP)
        out.append(rect(x, TOP, BW, BH, fill, stroke, rx=6, width=1.6))
        out.append(text(x + BW / 2, TOP + 30, title, 18, INK, "bold"))
        out.append(mono(x + BW / 2, TOP + 62, code, 14, INK, "bold", "middle"))
        for j, line in enumerate(desc):
            out.append(text(x + BW / 2, TOP + 92 + j * 22, line, 15, MUTED))
        if k:
            out.append(arrow(x - GAP + 2, TOP + BH / 2, x - 2, TOP + BH / 2, INK, 2))
    x0 = X0
    out += brace(x0, x0 + 2 * BW + GAP, TOP + BH + 10, BLUE, "同一个 .cu 文件")
    x1 = X0 + 2 * (BW + GAP)
    out += brace(x1, x1 + 2 * BW + GAP, TOP + BH + 10, GREEN, "Python 脚本")
    return out


if __name__ == "__main__":
    save("custom-op-flow", W, H, build(), left=0)
