#!/usr/bin/env python3
"""Turn 1 of session A, step by step: the first request fills four blocks.

3 message tokens, at most 2 answer tokens, an empty cache: kv_open makes
the entry, kv_reserve counts the tokens that go into the model as input
(t0 .. t3) and puts blocks 0 .. 3 into the block table, compute writes the
rows of token pos and reads those of tokens 0 .. pos. t4 is output only.
The example and the areas of a frame are in kv_example_kit.py.
Run it to refresh ../assets/kv-first-1.svg ... kv-first-16.svg.
"""

from kv_example_kit import *

REQUESTS = {"A": "A 第 1 轮：文本 `n_prompt`=3，最多生成 `max_answer`=2"}
MESSAGE = ((0, 3, "消息"),)
ANSWERED = ((0, 3, "消息"), (3, 1, "回答"))


def compute(tok):
    return ("given", f"compute(cache, A, t{tok}, state)")


INSIDE = [("fw", "embed"), ">", ("text", "每层"), ("fn", "kv_key"), ("fn", "kv_value"), ">",
          ("fw", "attention"), ">", ("fw", "feed_forward"), ">", ("text", "两层之后"),
          ("fn", "kv_append(A)")]


def whole(pos, **more):
    """One compute drawn as one frame."""
    marks = {**rows("r", "A", range(pos)), **rows("wr", "A", [pos])}
    return dict(stage=3, current=(pos,), marks=marks, cached=("A", pos + 1),
                reads=entries("A", range(pos + 1)),
                call=[("text", "框架调用"), compute(pos)],
                calc=INSIDE, labels=("调用", "依次", "结果"), **more)


STEPS = [
    dict(stage=0, text=3, fresh=(0, 1, 2), groups=MESSAGE,
         call=[("fw", "serve"), ">", ("fw", "append_message(A, message)")],
         calc="消息转换为 3 个 token：$t_0$、$t_1$、$t_2$，所以 `n_prompt` = 3",
         labels=("调用", "计算", "此刻"),
         ret="会话表为空，内存池的 12 块都空闲，位图全为 0",
         say="请求到来：$t_0$、$t_1$、$t_2$ 都还没有键与值"),
    dict(stage=1, text=3, groups=MESSAGE, open=("A", 0, 1),
         call=[("fw", "serve"), ">", ("given", "kv_open(cache, 0)")],
         calc="表中没有 id = 0（A 的编号）：新建一项，`last_use` 记为 `clock` 加 1 之后的值 1，越大越近使用",
         ret="`&session[0]`，以下记为 A。新项的块表全为 NULL",
         say="`kv_open` 在会话表中新建 A 的一项，这一项还没有块"),
    dict(stage=2, text=3, shown=5, groups=((0, 4, "会作为输入：4 个"),),
         output=(4, "只输出，", "不占块"), reads=(("A", "capacity"),),
         call=[("text", "框架调用"), ("fn", "kv_reserve(cache, A, 3, 2)"),
               ">", ("fw", "kv_num_free()")],
         calc="会作为输入的是 $t_0$、$t_1$、$t_2$ 与回答的 $t_3$：共 4 个，各用 1 块。`capacity` = 0，缺 4 块",
         ret="`kv_num_free()` 返回 12：空闲块足够",
         say="数 token：$t_0$ 到 $t_3$ 会作为输入，需要 4 块。$t_4$ 只输出，不占块"),
    dict(stage=2, text=3, groups=MESSAGE, add=("A", (0, 1, 2, 3)),
         call=[("fn", "kv_reserve(cache, A, 3, 2)"), ">", ("fw", "kv_alloc()"), ("text", "4 次")],
         calc="`kv_alloc` 每次取编号最小的空闲块并置位：依次是块 0、1、2、3",
         ret="`kv_alloc` 返回块的地址，填入块表。`kv_reserve` 返回 1，表示放得下",
         say="4 个位变为 1，4 块填入 A 的块表，都是预留"),
    dict(stage=3, text=3, groups=MESSAGE, current=(0,), reads=(("A", "n_cached"),),
         call=[("text", "框架调用"), compute(0), ">",
               ("fw", "embed(t0, state)")],
         calc="pos = 调用时的 `n_cached` = 0：这个 token 用块表中的 `block[0]`",
         labels=("调用", "计算", "结果"),
         ret="`state` 是 $t_0$ 的向量，接下来依次经过第 0、1 层",
         say="框架从第 `n_cached` 个 token 起调用已给出的 `compute`"),
    dict(stage=3, text=3, groups=MESSAGE, current=(0,), reads=entries("A", [0]),
         addresses=("A", 0, [0]), marks=rows("a", "A", [0], [0]),
         call=[compute(0), ">", ("fn", "kv_key(cache, A, 0, 0)"),
               ("fn", "kv_value(cache, A, 0, 0)")],
         say="第 0 层：先查块表得到块 0，再加块内偏移，只计算地址"),
    dict(stage=3, text=3, groups=MESSAGE, current=(0,), marks=rows("wr", "A", [0], [0]),
         call=[compute(0), ">", ("fw", "attention(0, 0, state, keys, values)"), ("text", "再"),
               ("fw", "feed_forward(0, state)")],
         calc="先算出 $k_0$、$v_0$ 并写入两个地址，再读它们：只有一行，$a_0$ = 1，$o_0 = a_0 v_0$",
         labels=("调用", "计算", "结果"),
         ret="$o_0$ 加到 `state` 上，`feed_forward` 再更新 `state`",
         say="`attention` 把 $k_0$、$v_0$ 写入块 0，再读它们计算注意力"),
    dict(stage=3, text=3, groups=MESSAGE, current=(0,), reads=entries("A", [0]),
         addresses=("A", 1, [0]), marks=rows("wr", "A", [0], [1]), written={("A", 0, 0)},
         call=[("given", "compute"), ">", ("fn", "kv_key(cache, A, 1, 0)"),
               ("fn", "kv_value(cache, A, 1, 0)"), ("text", "再"), ("fw", "attention(1, 0, …)"), ("text", "再"),
               ("fw", "feed_forward")],
         say="第 1 层同样：仍是块 0，偏移换成 20 与 30，写入后再读"),
    dict(stage=3, text=3, groups=MESSAGE, cached=("A", 1),
         call=[compute(0), ">", ("fn", "kv_append(A)")],
         calc="两层的键与值都已写入块 0：`n_cached` 加 1",
         labels=("调用", "计算", "结果"),
         ret="块 0 变为有效。`compute` 返回时，`state` 是最后一层之后的向量",
         say="全部层完成后 `kv_append` 一次：$t_0$ 已缓存"),
    dict(stage=3, text=3, groups=MESSAGE, current=(1,), reads=entries("A", [0, 1]),
         addresses=("A", 0, [0, 1]),
         marks={**rows("a", "A", [0], [0]), **rows("w", "A", [1], [0])},
         call=[compute(1), ">", ("fn", "kv_key(cache, A, 0, tok)"),
               ("fn", "kv_value(cache, A, 0, tok)"), ("text", "再"), ("fw", "attention(0, 1, …)")],
         say="$t_1$ 的第 0 层：收集两项地址，`attention` 先把 $k_1$、$v_1$ 写入块 1"),
    dict(stage=3, text=3, groups=MESSAGE, current=(1,), written={("A", 1, 0)},
         marks=rows("r", "A", [0, 1], [0]),
         call=[compute(1), ">", ("fw", "attention(0, 1, state, keys, values)"), ">",
               ("fw", "attn_scores"), ("fw", "attn_values")],
         calc="$t_1$ 的查询 $q_1$：$q_1·k_0$、$q_1·k_1$ 除以 $√d$（d = 10），经 softmax 得到权重 $a_0$、$a_1$",
         labels=("调用", "计算", "结果"),
         ret="加权和 $o_1 = a_0 v_0 + a_1 v_1$，加到 `state` 上",
         say="读块 0、1：$q·k$ → softmax → 加权和。$k_0$、$v_0$ 取自块 0"),
    dict(stage=3, text=3, groups=MESSAGE, current=(1,), reads=entries("A", [0, 1]),
         addresses=("A", 1, [0, 1]), written={("A", 1, 0)}, cached=("A", 2),
         marks={**rows("r", "A", [0], [1]), **rows("wr", "A", [1], [1])},
         call=[("given", "compute"), ">", ("fn", "kv_key(cache, A, 1, tok)"), ("fn", "kv_value"), ("text", "再"),
               ("fw", "attention(1, 1, …)"), ("text", "再"), ("fw", "feed_forward"), ("text", "再"),
               ("fn", "kv_append(A)")],
         say="$t_1$ 的第 1 层同样，两层完成后 `n_cached` 变为 2"),
    whole(2, text=3, groups=MESSAGE,
          ret="`n_cached` = 3，`state` 用来预测下一个 token",
          say="$t_2$ 重复同样的过程：写块 2，读块 0、1、2"),
    dict(stage=4, text=4, fresh=(3,), groups=ANSWERED, output=(3, "是输出，", "还未输入"),
         missing=entries("A", [3]), noted=entries("A", [3]),
         call=[("fw", "serve"), ">", ("fw", "next_token(state)")],
         calc="$t_2$ 最后一层之后的 `state` 中分数最高的 token 是 $t_3$：它是模型的输出",
         ret="$t_3$。它作为输入再经过一次模型，才有 $k_3$、$v_3$",
         say="$t_3$ 刚被输出：它作为输入再经过一次模型，才有 $k_3$、$v_3$"),
    whole(3, text=4, groups=ANSWERED,
          ret="`n_cached` = 4，预留的 4 块都已有效",
          say="$t_3$ 作为输入经过 `compute`：$k_3$、$v_3$ 写入块 3"),
    dict(stage=(4, 5), text=5, fresh=(4,), groups=((0, 3, "消息"), (3, 2, "回答")),
         output=(4, "只输出，", "未输入"), noted=entries("A", [4]),
         call=[("fw", "serve"), ">", ("fw", "next_token(state)")],
         calc="输出 $t_4$ 之后回答已有 2 个 token，达到 `max_answer`：本轮结束",
         ret="$t_4$。池中没有 $k_4$、$v_4$，`block[4]` 是 NULL",
         say="本轮结束：$t_4$ 只输出，它下一轮作为输入时才算出 $k_4$、$v_4$"),
]


def frames():
    return animate(empty_state(), STEPS, REQUESTS)


if __name__ == "__main__":
    for number, (body, _, _) in enumerate(frames(), 1):
        save(f"kv-first-{number}", W, H, body)
