#!/usr/bin/env python3
"""One layer of one step: the attention of token pos reads the keys and values
of tokens 0 .. pos. Two figures on one layout, of where the rows of tokens
0 .. pos - 1 come from.

kv-reuse-1  without a KV cache: the input of every token at this layer is kept, and
            each step computes the keys and values of tokens 0 .. pos - 1
            again from them (red).
kv-reuse-2  the KV cache: rows 0 .. pos - 1 are read from the KV cache of the
            conversation (green). The functions that give these rows are
            left to the pages on the framework.
Run it to refresh ../assets/kv-reuse-1.svg and kv-reuse-2.svg.
"""

from kv_kit import *

W, H = 1000, 404
LABEL_X = 24
X0, PITCH, CW = 196, 94, 80            # token columns
COLS = ["0", "1", "2", "…", "pos−1", "pos"]
Y_TOK, Y_IN, Y_K, Y_V = 34, 92, 178, 226
RH = 34                                 # row height
BOX_X = 790                             # the attention box
CAPTION_Y, LEGEND_Y = 330, 380


def col_x(i):
    return X0 + i * PITCH


def row_label(y, s, sub=None):
    out = [text(LABEL_X, y + RH / 2 + 6, s, 16, INK, "bold", "start")]
    return out


def row(y, kinds, label=""):
    """One row of cells over the token columns; kinds[i] None leaves it blank."""
    out = []
    for i, kind in enumerate(kinds):
        x = col_x(i)
        if COLS[i] == "…":
            if kind:
                out.append(text(x + CW / 2, y + RH / 2 + 6, "…", 18, MUTED))
            continue
        if kind:
            out += cell(x, y, CW, RH, label, kind, size=14)
    return out


def tokens_row():
    out = [text(LABEL_X, Y_TOK + RH / 2 + 6, "token", 16, INK, "bold", "start")]
    for i, s in enumerate(COLS):
        x = col_x(i)
        if s == "…":
            out.append(text(x + CW / 2, Y_TOK + RH / 2 + 6, "…", 18, MUTED))
            continue
        tone = ORANGE if s == "pos" else INK
        if s == "pos":
            out.append(text(x + CW / 2, Y_TOK + 4, "新 token", 14, ORANGE, "bold"))
        out.append(mono(x + CW / 2, Y_TOK + RH / 2 + 6, s, 16, tone, "bold", anchor="middle"))
    return out


def attention_box():
    """q of token pos compared with every key, the values added by weight."""
    out = []
    top, bot = Y_K - 18, Y_V + RH + 30                # room for the third line of text
    out.append(rect(BOX_X, top, W - 24 - BOX_X, bot - top, WHITE, MUTED, rx=8, width=1.4))
    out.append(mono(BOX_X + 14, top + 26, "attention", 15, INK, "bold"))
    out += cell(BOX_X + 14, top + 40, 40, 28, "q", "new", size=15)
    out.append(text(BOX_X + 62, top + 60, "新 token 的查询", 14, INK, anchor="start"))
    out.append(text(BOX_X + 14, top + 94, "q 与每一行的键比较", 14, INK, anchor="start"))
    out.append(text(BOX_X + 14, top + 116, "各行的值按结果加权相加", 14, INK, anchor="start"))
    # The rows it reads: a bracket over both rows of every column.
    bx = col_x(len(COLS) - 1) + CW + 16
    out.append(path(f"M {bx - 6:.1f} {Y_K:.1f} L {bx:.1f} {Y_K:.1f} L {bx:.1f} {Y_V + RH:.1f} "
                    f"L {bx - 6:.1f} {Y_V + RH:.1f}", MUTED, 1.6, head=False))
    out.append(arrow(bx + 2, (Y_K + Y_V + RH) / 2, BOX_X - 4, (Y_K + Y_V + RH) / 2, MUTED, 1.8))
    return out


def key_value_arrows(cols, color):
    """From the input of a column in this layer down to its key and value."""
    out = []
    for i in cols:
        x = col_x(i) + CW / 2
        out.append(arrow(x, Y_IN + RH + 2, x, Y_K - 4, color, 1.8))
    return out


def base(earlier_kv, earlier_in):
    n = len(COLS)
    pos = n - 1
    kinds_kv = [earlier_kv] * pos + ["new"]
    kinds_in = [earlier_in] * pos + ["mid"]
    out = tokens_row()
    out.append(text(LABEL_X, Y_IN + RH / 2 + 6, "这一层的输入", 16, INK, "bold", "start"))
    out += row(Y_IN, kinds_in)
    out += row_label(Y_K, "键（keys）")
    out += row_label(Y_V, "值（values）")
    out += row(Y_K, kinds_kv)
    out += row(Y_V, kinds_kv)
    out += key_value_arrows([pos], ORANGE)
    out.append(text(col_x(pos) + CW / 2 + 8, (Y_IN + RH + Y_K) / 2 + 5, "计算键与值", 13, ORANGE,
                    "bold", "start"))
    out += attention_box()
    return out


def reuse_nocache():
    out = base("waste", "mid")
    n = len(COLS)
    out += key_value_arrows([i for i in range(n - 1) if COLS[i] != "…"], RED)
    out.append(text(col_x(0) - 6, (Y_IN + RH + Y_K) / 2 + 5, "计算键与值", 13, RED, "bold",
                    "end"))
    out.append(caption(W / 2, CAPTION_Y,
                       "不使用 KV Cache：保存每个 token 在每层的输入，每一步重新计算第 0 到 pos−1 行"))
    out += centred_legend(W / 2, LEGEND_Y, [("new", "这一步计算的数据"),
                                            ("waste", "重复的计算"), ("mid", "保存的输入向量")])
    return out


def reuse_cache():
    out = base("reuse", None)
    n = len(COLS)
    # The KV cache: the rows of this layer.
    fx0, fx1 = col_x(0) - 10, col_x(n - 1) + CW + 8
    out.append(rect(fx0, Y_K - 10, fx1 - fx0, Y_V + RH + 10 - (Y_K - 10), "none", BLUE, rx=8,
                    width=1.8, dash="6 4"))
    out.append(text(fx0, Y_V + RH + 32, "KV Cache：之前 token 在这一层的键与值", 14, BLUE,
                    "bold", "start"))
    out.append(caption(W / 2, CAPTION_Y,
                       "KV Cache：第 0 到 pos−1 行从 KV Cache 中读取，每一步只计算第 pos 行"))
    out += centred_legend(W / 2, LEGEND_Y, [("new", "这一步计算的数据"),
                                            ("reuse", "从缓存直接读取"), ("mid", "输入向量")])
    return out


if __name__ == "__main__":
    save("kv-reuse-1", W, H + 20, reuse_nocache())
    save("kv-reuse-2", W, H + 20, reuse_cache())
