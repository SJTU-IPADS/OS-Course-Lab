#!/usr/bin/env python3
"""Two decoding steps with a KV cache, laid out like kv_attention.py and kv_naive.py.

Each step computes one row of X, Q, K, V, A and O, for the new token only (orange). The
k and v of the earlier tokens are read from the KV cache (blue, inside the dashed box);
the new k and v are appended to it, so the cache grows by one row per step.
Run it to refresh ../assets/kv-cache-1.svg and kv-cache-2.svg.
"""

from kv_matrix import *

ITEMS = [("new", "本步新算"), ("cache", "从 KV Cache 读出"), ("param", "模型参数")]

if __name__ == "__main__":
    for step, n in ((1, 5), (2, 6)):
        cap = (f"解码第 {step} 步：只为「{WORDS[n - 1]}」计算 q、k、v，"
               f"k{n}、v{n} 追加到 KV Cache")
        save(f"kv-cache-{step}", W, H, left=LEFT, body=figure("cache", n, step, cap, ITEMS))
