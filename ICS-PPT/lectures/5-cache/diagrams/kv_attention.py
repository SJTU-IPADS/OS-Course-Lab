#!/usr/bin/env python3
"""The attention of one head in the prefill, drawn as matrices in three frames:
X times W_Q, W_K, W_V gives Q, K, V; Q K^T under the causal mask gives A; A V gives O,
whose last row produces the next token. Run it to refresh ../assets/kv-attention-1..3.svg.
"""

from kv_matrix import *

N = 4
ITEMS = [("param", "模型参数"), ("mid", "一行对应一个 token"), ("empty", "被掩码去掉")]
CAPTIONS = [
    "① X 的每一行是一个 token 的向量，乘以三个权重矩阵得到 Q、K、V",
    "② A = softmax(mask(Q Kᵀ) / √d)：因果掩码使第 i 行只保留前 i 列",
    f"③ O = A V：O 的最后一行用于生成下一个 token「{WORDS[N]}」",
]

if __name__ == "__main__":
    for stage in (1, 2, 3):
        save(f"kv-attention-{stage}", W, H, left=LEFT, body=figure("prefill", N, stage, CAPTIONS[stage - 1], ITEMS))
