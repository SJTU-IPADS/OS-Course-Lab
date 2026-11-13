#!/usr/bin/env python3
"""Two decoding steps with a KV cache, laid out like kv_attention.py and kv_naive.py.

Each step computes one row of X, Q, K, V, A and O, for the new token only (orange). The
k and v of the earlier tokens are read from the KV cache (blue, inside the dashed box);
the new k and v are appended to it, so the cache grows by one row per step. Two notes per
frame give the reason from what the vectors mean: the query serves its own token once and
is not kept, the keys and values serve every later token and are kept.
Run it to refresh ../assets/kv-cache-1.svg and kv-cache-2.svg.
"""

from kv_matrix import *

ITEMS = [("new", "本步新算"), ("cache", "从 KV Cache 读出"), ("param", "模型参数")]
NOTE_X = MX + MW + 31                     # the note on the query, to the right of Q
CACHE_X = MX + MW + 119                   # the note on the cache, past its label


def notes(step, n):
    word = WORDS[n - 1]
    if step == 1:
        query = [f"查询 q{n}：「{word}」要查找的信息，只用于它自己的输出 o{n}",
                 f"算出 o{n} 之后不再使用，不保存"]
        cache = ["键与值：已有 token 提供给之后每个 token 的信息",
                 "不随新 token 改变，每一步都要读取，保存在缓存中"]
    else:
        query = [f"查询 q{n}：只用于「{word}」自己的输出 o{n}",
                 f"上一步的 q{n - 1} 没有保存，这一步也不需要它"]
        cache = [f"k{n - 1}、v{n - 1} 已在上一步存入缓存，这一步直接读出",
                 "缓存每步增加一行，已有的行不改变"]
    return (note(NOTE_X, QT + (n - 1) * RH - 22, query, ORANGE)
            + note(CACHE_X, VT + 72, cache, BLUE))


if __name__ == "__main__":
    for step, n in ((1, 5), (2, 6)):
        cap = (f"解码第 {step} 步：只为「{WORDS[n - 1]}」计算 q、k、v，"
               f"k{n}、v{n} 追加到 KV Cache")
        save(f"kv-cache-{step}", W, H, left=LEFT,
             body=figure("cache", n, step, cap, ITEMS) + notes(step, n))
