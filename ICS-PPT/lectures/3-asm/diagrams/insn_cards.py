#!/usr/bin/env python3
"""The lecture's instructions in five cards, mnemonics with a short meaning.

The grouping and the mnemonics are those of the two summary pages before the
figure. Every card has the height of the longest one, and its lines are
centred between the header and the bottom edge. A row of cards is centred on
the canvas, so the two cards of the second row sit on either side of the
vertical centre line. Run it to refresh ../assets/insn-cards.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, FILL_RED, GREEN, INK,
                    MUTED, ORANGE, RED, WHITE, mono, rect, save, text)

W = 1120
COLS, CWD, GAP = 3, 356, 14
HEAD, PITCH, CAP, PAD = 40, 24, 10, 16     # CAP: height of a capital letter

CARDS = [
    ("1  数据传送与扩展", BLUE, FILL_BLUE,
     [("movb movw movl movq", "数据传送"), ("movzbl movzwl", "零扩展"),
      ("movsbl movswl movslq cltq", "符号扩展"), ("pushq popq", "栈操作"),
      ("leal leaq", "地址计算")]),
    ("2  算术与逻辑运算", ORANGE, FILL_ORANGE,
     [("addl addq subl subq", "加减"), ("imull imulq", "乘法"),
      ("incl decl negl notl", "单操作数"), ("andl orl xorl", "位逻辑"),
      ("sall shll sarl shrl salq", "移位")]),
    ("3  状态比较与分支跳转", GREEN, FILL_GREEN,
     [("cmpl cmpq", "减法设标志"), ("testl testq", "与运算设标志"),
      ("jmp", "无条件跳转"), ("jl jle jg jge", "有符号"), ("jb jbe ja jae", "无符号"),
      ("je jne", "零值"), ("setX cmovX", "条件设置 / 传送")]),
    ("4  函数调用与运行时安全", RED, FILL_RED,
     [("call ret", "调用与返回"), ("%fs:40", "金丝雀值"),
      ("__stack_chk_fail@PLT", "检查失败")]),
    ("5  向量计算（AVX2 扩展）", BLUE, FILL_BLUE,
     [("vmovdqu vmovdqa", "加载 / 存储"), ("vpmulld vpaddd", "8 通道乘加"),
      ("vpxor", "清零")]),
]


# The longest card sets the height: header, padding, its lines, padding.
LINES = max(len(rows) for _, _, _, rows in CARDS)
CHT = HEAD + PAD + CAP + (LINES - 1) * PITCH + PAD
ROWS = -(-len(CARDS) // COLS)
H = 8 + ROWS * CHT + (ROWS - 1) * GAP + 2


def build():
    out = []
    for k, (title, stroke, fill, rows) in enumerate(CARDS):
        r = k // COLS
        n = len(CARDS[r * COLS:(r + 1) * COLS])
        x = (W - n * CWD - (n - 1) * GAP) / 2 + (k % COLS) * (CWD + GAP)
        y = 8 + r * (CHT + GAP)
        out.append(rect(x, y, CWD, CHT, WHITE, stroke, rx=8, width=1.8))
        out.append(rect(x, y, CWD, HEAD, fill, stroke, rx=8, width=1.8))
        out.append(text(x + 16, y + 27, title, 17, INK, "bold", anchor="start"))
        # Baseline of the first line: the block of lines, from the top of the
        # first capital to the last baseline, is centred under the header.
        first = y + (HEAD + CHT) / 2 - (len(rows) - 1) * PITCH / 2 + CAP / 2
        for j, (m, what) in enumerate(rows):
            yy = first + j * PITCH
            out.append(mono(x + 16, yy, m, 14, INK, "bold"))
            out.append(text(x + CWD - 14, yy, what, 13, MUTED, anchor="end"))
    return out


if __name__ == "__main__":
    save("insn-cards", W, H, build())
