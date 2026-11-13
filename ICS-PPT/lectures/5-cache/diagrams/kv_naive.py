#!/usr/bin/env python3
"""A decoding step without a KV cache, in three frames, laid out like kv_attention.py.

The new token is appended to X as row N+1. Frame 1: every matrix is computed again from
X; the rows of the N old tokens (red) are the same as in the previous step, the new row
is orange. Frame 2: the next token needs only the last row of O, hence the last row of A
and the query of the new token; the other rows of Q, A and O are greyed out, with a note
on what the old queries are for. Frame 3: the new query reads every key and value, and a
note says why the old keys and values are the same as in the previous step.
Run it to refresh ../assets/kv-naive-1.svg to kv-naive-3.svg.
"""

from kv_matrix import *

N = 5                                     # rows: the 4 prompt tokens and the new one
NOTE_X = MX + MW + 31                     # annotations start to the right of Q, K and V
ITEMS = [("new", "新 token 的一行"), ("waste", "重新计算，与上一步相同"),
         ("gone", "生成下一个 token 时不需要")]
CAPTIONS = [
    f"解码第 1 步：X 增加「{WORDS[N - 1]}」一行，Q、K、V、A、O 全部重新计算",
    f"生成下一个 token 只用新 token 的输出 o{N}，已有 token 的查询不需要",
    f"q{N} 要读取全部的键与值，已有 token 的键与值与上一步相同",
]
OLD_Q = [f"q1–q{N - 1}：已有 token 的查询，各自只用于自己的输出 o1–o{N - 1}",
         f"生成下一个 token 只用 o{N}，这一步不需要它们"]
OLD_KV = [f"k1–k{N - 1}、v1–v{N - 1}：已有 token 的键与值，q{N} 要读取它们",
          f"它们只由 x1–x{N - 1} 算出，与上一步的结果相同"]


def frame(stage):
    items = ITEMS if stage > 1 else ITEMS[:2]
    out = figure("naive", N, stage, CAPTIONS[stage - 1], items)
    if stage > 1:
        out += note(NOTE_X, QT + 10, OLD_Q, LINE)
    if stage > 2:
        out += note(NOTE_X, VT + 68, OLD_KV, RED)
    return out


if __name__ == "__main__":
    for stage in (1, 2, 3):
        save(f"kv-naive-{stage}", W, H, left=LEFT, body=frame(stage))
