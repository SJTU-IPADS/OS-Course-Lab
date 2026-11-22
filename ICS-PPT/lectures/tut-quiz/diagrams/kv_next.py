#!/usr/bin/env python3
"""Turn 2 of session A, step by step: the next request finds its rows and goes on.

Session B takes one turn in between and gets blocks 4 and 5. Then kv_open
finds A by its id, kv_reserve adds only the three blocks it lacks (6, 7, 8)
and leaves the four entries it has, and compute collects the rows of tokens
0 .. pos through the block table, so t4 reads blocks 0, 1, 2, 3 and 6. t7 is
output only. The example and the areas of a frame are in kv_example_kit.py.
Run it to refresh ../assets/kv-next-1.svg ... kv-next-13.svg.
"""

from kv_example_kit import *

REQUESTS = {"A": "A 第 2 轮：文本 `n_prompt`=6，最多生成 `max_answer`=2",
            "B": "B 的一轮：文本 `n_prompt`=1，最多生成 `max_answer`=2"}
TEXT = ((0, 5, "第 1 轮"), (5, 1, "新消息"))
ANSWERED = TEXT + ((6, 1, "回答"),)
FINDS = entries("A", range(4))


def compute(tok):
    return ("given", f"compute(cache, A, t{tok}, state)")


INSIDE = [("fw", "embed"), ">", ("text", "每层"), ("fn", "kv_key"), ("fn", "kv_value"), ">",
          ("fw", "attention"), ">", ("fw", "feed_forward"), ">", ("text", "两层之后"),
          ("fn", "kv_append(A)")]


STEPS = [
    dict(stage=(1, 2, 3, 4), who="B", text=3, output=(2, "只输出，", "不占块"),
         groups=((0, 1, "消息"), (1, 2, "回答")),
         open=("B", 1, 2), add=("B", (4, 5)), cached=("B", 2), marks=rows("wr", "B", [0, 1]),
         call=[("fw", "serve"), ">", ("given", "kv_open(cache, 1)"), ("text", "再"),
               ("fn", "kv_reserve(cache, B, 1, 2)"), ">", ("fw", "kv_alloc()"), ("text", "2 次")],
         labels=("调用", "返回", "随后"),
         calc="`&session[1]`，以下记为 B。会作为输入的是 $b_0$、$b_1$，分到块 4、5",
         ret=[("given", "compute(cache, B, b0, state)"), ("text", "再"), ("given", "compute(cache, B, b1, state)")],
         say="两轮之间 B 运行一轮，占用块 4、5。A 的块与块表不变"),
    dict(stage=0, text=6, fresh=(5,), groups=TEXT,
         call=[("fw", "serve"), ">", ("fw", "append_message(A, message)")],
         calc="新消息 $t_5$ 接在 $t_0$ 到 $t_4$ 之后，所以 `n_prompt` = 6",
         labels=("调用", "计算", "此刻"),
         ret="$t_0$ 到 $t_3$ 已缓存，$t_4$ 上一轮只输出，还没有 $k_4$、$v_4$",
         say="A 的下一次请求到来：怎样找到上一轮的 K、V？"),
    dict(stage=1, text=6, groups=TEXT, open=("A", 0, 3),
         reads=(("A", "id"),), noted=(("A", "n_cached"),) + FINDS, focus=FINDS,
         call=[("fw", "serve"), ">", ("given", "kv_open(cache, 0)")],
         calc="表中已有 id = 0：就是 `session[0]`，`last_use` 记为 `clock` 加 1 之后的值 3，越大越近使用",
         ret="原来的 A：`n_cached` = 4，`block[0..3]` 仍指向块 0、1、2、3",
         say="按 id 找到原来的一项，由 `n_cached` 与块表找到上次的 K、V"),
    dict(stage=2, text=6, shown=8, groups=((0, 7, "会作为输入：7 个"),),
         output=(7, "只输出，", "不占块"), reads=(("A", "capacity"),),
         call=[("text", "框架调用"), ("fn", "kv_reserve(cache, A, 6, 2)"),
               ">", ("fw", "kv_num_free()")],
         calc="会作为输入的是 $t_0$ 到 $t_5$ 与回答的 $t_6$：共 7 个。已有 4 块继续使用，只缺 3 块",
         ret="`kv_num_free()` 返回 6：空闲块足够",
         say="数 token：共需 7 块，已有 4 块，只缺 3 块。$t_7$ 只输出，不占块"),
    dict(stage=2, text=6, groups=TEXT, add=("A", (6, 7, 8)),
         call=[("fn", "kv_reserve(cache, A, 6, 2)"), ">", ("fw", "kv_alloc()"), ("text", "3 次")],
         calc="块 4、5 属于 B，编号最小的空闲块是 6、7、8：填入 `block[4]` 到 `block[6]`",
         ret="`kv_reserve` 返回 1，表示放得下。原有 4 项与块中的内容都不变",
         say="只置 3 个位、填 3 项：$t_4$ 分到块 6，A 的块不连续"),
    dict(stage=3, text=6, groups=TEXT, current=(4,), reads=(("A", "n_cached"),),
         call=[("text", "框架调用"), compute(4), ">",
               ("fw", "embed(t4, state)")],
         calc="pos = 调用时的 `n_cached` = 4：从 $t_4$ 继续，$t_0$ 到 $t_3$ 的键与值已在池中",
         labels=("调用", "计算", "结果"),
         ret="`state` 是 $t_4$ 的向量，它的键与值将写入 `block[4]` 指向的块 6",
         say="上一轮只输出的 $t_4$ 现在作为输入，开始计算 $k_4$、$v_4$"),
    dict(stage=3, text=6, groups=TEXT, current=(4,), reads=entries("A", range(5)),
         addresses=("A", 0, list(range(5))), marks=rows("a", "A", range(5), [0]),
         call=[compute(4), ">", ("fn", "kv_key(cache, A, 0, tok)"),
               ("fn", "kv_value(cache, A, 0, tok)"), ("text", "tok 取 0 到 4")],
         say="按块表收集 5 项地址：块 0、1、2、3、6，各加 0 与 10"),
    dict(stage=3, text=6, groups=TEXT, current=(4,),
         marks={**rows("r", "A", range(4), [0]), **rows("wr", "A", [4], [0])},
         call=[compute(4), ">", ("fw", "attention(0, 4, state, keys, values)"), ("text", "再"),
               ("fw", "feed_forward(0, state)")],
         calc="写入 $k_4$、$v_4$，再读 5 项：$[a_0, …, a_4] = softmax([q_4·k_0, …, q_4·k_4] / √d)$",
         labels=("调用", "计算", "结果"),
         ret="$o_4 = a_0 v_0 + … + a_4 v_4$ 加到 `state` 上。$q_4$ 是 $t_4$ 的查询，d = 10",
         say="只写最后一项（块 6），读全部 5 项：前 4 项来自上一轮"),
    dict(stage=3, text=6, groups=TEXT, current=(4,), reads=entries("A", range(5)),
         addresses=("A", 1, list(range(5))), written={("A", 4, 0)},
         marks={**rows("r", "A", range(4), [1]), **rows("wr", "A", [4], [1])},
         call=[("given", "compute"), ">", ("fn", "kv_key(cache, A, 1, tok)"),
               ("fn", "kv_value(cache, A, 1, tok)"), ("text", "再"), ("fw", "attention(1, 4, …)"), ("text", "再"),
               ("fw", "feed_forward")],
         say="第 1 层：还是这 5 块，各加 20 与 30，写最后一项，读全部 5 项"),
    dict(stage=3, text=6, groups=TEXT, cached=("A", 5),
         call=[compute(4), ">", ("fn", "kv_append(A)")],
         calc="两层都已写入块 6：`n_cached` 加 1",
         labels=("调用", "计算", "结果"),
         ret="块 6 变为有效，块 7、8 仍是预留",
         say="$t_4$ 已缓存：A 的有效块是 0、1、2、3、6"),
    dict(stage=3, text=6, groups=TEXT, current=(5,), cached=("A", 6),
         reads=entries("A", range(6)),
         marks={**rows("r", "A", range(5)), **rows("wr", "A", [5])},
         call=[("text", "框架调用"), compute(5)],
         calc=INSIDE, labels=("调用", "依次", "结果"),
         ret="`n_cached` = 6，消息已全部计算",
         say="$t_5$ 重复同样的过程：写块 7，读块 0、1、2、3、6、7"),
    dict(stage=(3, 4), text=7, groups=ANSWERED, current=(6,), cached=("A", 7),
         reads=entries("A", range(7)),
         marks={**rows("r", "A", range(6)), **rows("wr", "A", [6])},
         call=[("fw", "serve"), ">", ("fw", "next_token(state)"), ("text", "输出 $t_6$，框架再调用"),
               compute(6)],
         calc=INSIDE, labels=("调用", "依次", "结果"),
         ret="`n_cached` = 7，预留的块都已有效",
         say="输出的 $t_6$ 再作为输入经过 `compute`：写块 8，读 A 的 7 块"),
    dict(stage=(4, 5), text=8, fresh=(7,), groups=ANSWERED[:2] + ((6, 2, "回答"),),
         output=(7, "只输出，", "未输入"), noted=entries("A", [7]),
         call=[("fw", "serve"), ">", ("fw", "next_token(state)")],
         calc="输出 $t_7$ 之后回答已有 2 个 token，达到 `max_answer`：本轮结束",
         ret="$t_7$。池中没有 $k_7$、$v_7$，它下一轮作为输入时才算出",
         say="本轮结束：$t_7$ 只输出。下次仍按 id、`n_cached` 与块表找到这 7 块"),
]


def frames():
    return animate(turn2_start(), STEPS, REQUESTS)


if __name__ == "__main__":
    for number, (body, _, _) in enumerate(frames(), 1):
        save(f"kv-next-{number}", W, H, body)
