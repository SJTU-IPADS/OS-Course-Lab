#!/usr/bin/env python3
"""The memory write transaction `movq %rax, A` in three frames.

Same bus structure and colours as read_transaction.py (imported). %rax holds the
word y. 1: the CPU puts address A on the bus; main memory reads it and waits for
the data. 2: the CPU puts y on the bus. 3: main memory reads y from the bus and
stores it at address A.
Run it to refresh ../assets/write-transaction-1.svg .. write-transaction-3.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from read_transaction import W, H, caption, static, traffic
from svgkit import save

FRAMES = [
    (("y", True), ("?", False), "A", "① CPU 将地址 A 放到总线上，内存读出 A，等待数据"),
    (("y", True), ("?", False), "y", "② CPU 将数据字 y 放到总线上"),
    (("y", False), ("y", True), "y", "③ 内存从总线读出 y，存储在地址 A 处"),
]


def frame(k):
    rax, cell, label, cap = FRAMES[k - 1]
    out = static(rax, cell)
    out += traffic(label, True)
    return out + caption(cap)


if __name__ == "__main__":
    for k in (1, 2, 3):
        save(f"write-transaction-{k}", W, H, frame(k))
