#!/usr/bin/env python3
"""A decoding step without a KV cache, in two frames, laid out like kv_attention.py.

The new token is appended to X as row N+1. Frame 1: every matrix is computed again from
X; the rows of the N old tokens (red) are the same as in the previous step, the new row
is orange. Frame 2: the next token needs only the last row of O, hence the last row of A,
the q of the new token and every k and v; the other rows of Q, A and O are greyed out.
Run it to refresh ../assets/kv-naive-1.svg and kv-naive-2.svg.
"""

from kv_matrix import *

N = 5                                     # rows: the 4 prompt tokens and the new one
ITEMS = [("new", "新 token 的一行"), ("waste", "重新计算，与上一步相同"),
         ("gone", "生成下一个 token 时不需要")]
CAPTIONS = [
    f"解码第 1 步：X 增加「{WORDS[N - 1]}」一行，Q、K、V、A、O 全部重新计算",
    "生成下一个 token 只用到 q5 与全部的 k、v，k1–k4、v1–v4 与上一步相同",
]

if __name__ == "__main__":
    for stage in (1, 2):
        items = ITEMS if stage == 2 else ITEMS[:2]
        save(f"kv-naive-{stage}", W, H, left=LEFT, body=figure("naive", N, stage, CAPTIONS[stage - 1], items))
