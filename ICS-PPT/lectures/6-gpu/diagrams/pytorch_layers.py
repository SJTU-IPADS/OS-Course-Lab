#!/usr/bin/env python3
"""The layers a PyTorch operator call passes through, from Python to the GPU.

pytorch-layers.svg: one band per layer, top to bottom. Python API and Python
binding (torch/csrc), ATen (tensors and the C++ function of every operator),
the dispatcher (purple, the selecting layer), the kernels registered per
dispatch key, the libraries and hand-written CUDA kernels those kernels call,
the CUDA runtime and driver, the GPU. Orange marks what the call of
`x @ w` on CUDA tensors that require gradients runs through: the Autograd and
CUDA kernels, then cuBLAS. Autocast, CPU and the other keys are present in the
table and not selected. The directory names are in the speaker notes.
Run it to refresh ../assets/pytorch-layers.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, FILL_PURPLE, INK, MUTED, ORANGE,
                    PURPLE, WHITE, arrow, box, elbow, rect, save, text)

W, H = 620, 508
X, BW = 10, 600
GAP = 16


def band(y, h, label, sub, fill, stroke):
    """A full-width band: the layer's name on the left, what it does on the right."""
    out = [rect(X, y, BW, h, fill, stroke, rx=6, width=1.6),
           text(X + 16, y + 26, label, 17, INK, "bold", anchor="start")]
    if sub:
        out.append(text(X + BW - 14, y + 26, sub, 14, MUTED, anchor="end"))
    return out


def chips(y, h, items, selected):
    """Small boxes inside a tall band, under its label; `selected` are orange."""
    out = []
    n = len(items)
    pad, gap = 14, 8
    w = (BW - 2 * pad - (n - 1) * gap) / n
    centres = {}
    for k, name in enumerate(items):
        x = X + pad + k * (w + gap)
        hit = name in selected
        out += box(x, y + h - 38, w, 30, name, FILL_ORANGE if hit else WHITE,
                   ORANGE if hit else MUTED, size=15, weight="bold" if hit else "normal", rx=4)
        centres[name] = (x + w / 2, y + h - 38, y + h - 8)
    return out, centres


def build():
    out = []
    short, tall = 40, 70
    kinds = [
        ("Python API", "torch.matmul、Tensor.__matmul__", FILL_BLUE, BLUE, short),
        ("Python 绑定", "解析参数，释放 GIL，调用 C++", FILL_GREY, MUTED, short),
        ("ATen", "at::Tensor、at::matmul、at::mm", FILL_GREY, MUTED, short),
        ("Dispatcher", "按 dispatch key 选择核函数", FILL_PURPLE, PURPLE, short),
        ("各 key 的核函数", "", FILL_GREY, MUTED, tall),
        ("库与 CUDA 核函数", "", FILL_GREY, MUTED, tall),
        ("CUDA 运行时与驱动", "启动核函数，管理显存", FILL_GREY, MUTED, short),
        ("GPU", "SM 与显存", WHITE, INK, short),
    ]
    y = 8
    tops = []
    for label, sub, fill, stroke, h in kinds:
        out += band(y, h, label, sub, fill, stroke)
        tops.append((y, h))
        y += h + GAP
    # arrows between consecutive bands
    for k in range(len(kinds) - 1):
        y0 = tops[k][0] + tops[k][1]
        out.append(arrow(X + BW / 2, y0 + 1, X + BW / 2, y0 + GAP - 1, INK, 1.8))
    # chips in the two tall bands
    y5, h5 = tops[4]
    c5, ctr5 = chips(y5, h5, ["Autocast", "Autograd", "CPU", "CUDA", "…"], {"Autograd", "CUDA"})
    y6, h6 = tops[5]
    c6, ctr6 = chips(y6, h6, ["cuDNN", "自写 .cu 核函数", "cuBLAS"], {"cuBLAS"})
    out += c5 + c6
    # CUDA -> cuBLAS: the kernel of the CUDA key calls the library
    cx, _, cy1 = ctr5["CUDA"]
    bx, by0, _ = ctr6["cuBLAS"]
    mid = y5 + h5 + GAP / 2
    out.append(elbow([(cx, cy1 + 1), (cx, mid), (bx, mid), (bx, by0 - 1)], ORANGE, 2))
    return out


if __name__ == "__main__":
    save("pytorch-layers", W, H, build())
