#!/usr/bin/env python3
"""Multi-head, grouped-query and multi-query attention: eight query heads in every panel,
8, 2 and 1 key/value heads below them; the size of the KV cache follows the number of
key/value heads. Run it to refresh ../assets/kv-gqa.svg."""

from kv_kit import *

W, H = 1000, 300
NQ, QW, QP = 8, 30, 38                   # query heads: width and pitch
PANELS = [("多头注意力 MHA", 8), ("分组查询注意力 GQA", 2), ("多查询注意力 MQA", 1)]


def panel(x0, title, n_kv):
    out = [text(x0 + (NQ * QP - 8) / 2, 28, title, 19, INK, "bold")]
    for i in range(NQ):
        out += cell(x0 + i * QP, 60, QW, 30, "", "new")
    out.append(text(x0 - 10, 80, "q", 16, ORANGE, "bold", "end"))
    group = NQ // n_kv
    for g in range(n_kv):
        gx = x0 + g * group * QP
        gw = group * QP - 8
        for j in range(group):
            qx = x0 + (g * group + j) * QP + QW / 2
            out.append(line(qx, 90, qx, 150, MUTED, 1.6))
        out += cell(gx, 150, gw, 34, "k v" if gw > 40 else "", "cache", size=15)
    out.append(text(x0 - 10, 172, "k v", 16, BLUE, "bold", "end"))
    out.append(text(x0 + (NQ * QP - 8) / 2, 218, f"{n_kv} 个 KV 头", 18, INK, "bold"))
    size = {8: "1", 2: "1/4", 1: "1/8"}[n_kv]
    out.append(text(x0 + (NQ * QP - 8) / 2, 252, f"KV Cache 为 MHA 的 {size}" if n_kv < 8
                    else "KV Cache：基准", 18, BLUE, "bold"))
    return out


def figure():
    out = []
    for k, (title, n_kv) in enumerate(PANELS):
        out += panel(40 + k * 320, title, n_kv)
    return out


if __name__ == "__main__":
    save("kv-gqa", W, H, figure())
