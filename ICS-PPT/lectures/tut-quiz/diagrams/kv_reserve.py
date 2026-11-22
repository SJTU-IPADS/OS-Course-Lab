#!/usr/bin/env python3
"""kv_reserve for the request of B under lru, in six frames.

The example of kv-memory. The arguments 37 and 8 are explained on the
slide, the 44 tokens in the panel of every frame, and last_use 5 of B is
marked as that of this request, set before kv_reserve is called.
(1) B will have at most 44 tokens cached: it keeps its 19 blocks and lacks
25. (2) 14 are free, and at the end A, B and C would need 107 blocks of the
96: the limit is really exceeded. (3) Reserved blocks of the other sessions
come first: here there are none. (4) kv_victim picks A. (5) kv_invalidate
gives back its 42 blocks, 56 are free, enough. (6) 25 blocks, 0 to 24, go
into entries 19 .. 43 of B, whose blocks 42 .. 60 stay. The pool has the
scale of kv-memory.svg.
Run it to refresh ../assets/kv-reserve-1..6.svg.
"""

from kv_run_kit import *

W, H = 1120, 370
SCALE = Scale(84, 9.6)                  # 107 blocks still fit on the canvas
RULE = ["算出缺几块", "空闲不够：收回预留", "仍不够：失效", "只分配缺的块"]
TX, HEAD_Y, SUB_Y, TY, RH = 2, 46, 61, 65, 25
COLS = [("会话", "", 54), ("n_cached", "已缓存的 token", 106), ("capacity", "已分到的块", 92),
        ("last_use", "越大越近使用", 140), ("块表指向的块", "", 146)]
PX, PY, PH = 550, 35, 103
ROWS = (52, 84, 106, 128)
BIT_Y, POOL_Y, POOL_H = 161, 184, 32
NEED_Y, NEED_H = 250, 29
CAPTION_Y, LEGEND_Y = 326, 356

OPENED = dict(START, B=dict(START["B"], last_use=5))
INVALID = dict(OPENED, A=session(0, 2))
AFTER_ALLOC = dict(INVALID, B=session(19, 5, (42, 19), (0, 25)))
MISSING = NEED - OPENED["B"]["capacity"]
FINAL = [("B", NEED), ("A", 42), ("C", 21)]


def rail(active):
    out = [mono(2, 20, "kv_reserve", 16, INK, "bold")]
    x = 110
    for i, name in enumerate(RULE):
        if i:
            out.append(arrow(x + 4, 15, x + 34, 15, MUTED, 1.7))
            x += 40
        label = f"{'①②③④'[i]} {name}"
        w = width_of(label, 17) + 22
        on = i == active
        out.append(rect(x, 1, w, 28, FILL_ORANGE if on else WHITE, ORANGE if on else LINE, rx=6,
                        width=2.6 if on else 1.2))
        out.append(text(x + w / 2, 21, label, 17, INK if on else MUTED, "bold"))
        x += w
    return out


def table(sessions, before, reads=()):
    out, x = [], TX
    for name, meaning, w in COLS:
        out.append(mono(x + w / 2, HEAD_Y, name, 15, MUTED, "bold", anchor="middle")
                   if name.isascii() else text(x + w / 2, HEAD_Y, name, 15, MUTED, "bold"))
        if meaning:
            out.append(text(x + w / 2, SUB_Y, meaning, 13, MUTED))
        x += w
    edges = []
    for i, (who, entry) in enumerate(sessions.items()):
        y = TY + i * RH
        out.append(rect(TX, y, sum(c[2] for c in COLS), RH, WHITE, LINE, rx=2, width=1.1))
        out.append(mono(TX + 27, y + 18.5, who, 17, OWNER[who], "bold", anchor="middle"))
        x = TX + COLS[0][2]
        for name, _, w in COLS[1:]:
            if name == "块表指向的块":
                key, value, was = "blocks", ranges(entry), ranges(before[who])
                label = f"{was} → 无" if value == "无" and was != value else value
            else:
                key, value, was = name, entry[name], before[who][name]
                label = f"{was} → {value}" if was != value else str(value)
                if (who, name) == ("B", "last_use"):
                    label += "（这次请求）"
            changed = was != value
            if changed or (who, key) in reads:
                fill, stroke = KINDS["new" if changed else "reuse"]
                edges.append(rect(x + 1.5, y + 2, w - 3, RH - 4, fill, stroke, rx=2, width=2))
            edges += tagged(f"field/{who}/{key}", [text(x + w / 2, y + 18.5, label, 17,
                                                       ORANGE if changed else INK, "bold")])
            x += w
    return out + edges


def panel(step):
    out = [rect(PX, PY, W - 2 - PX, PH, WHITE, MUTED, rx=7, width=1.3)]
    out += call_line(PX + 10, ROWS[0], step["call"], W - 6)
    out += tagged("count", [rich(PX + 12, ROWS[1], COUNTED, 15, MUTED, "bold", room=W - 6 - PX - 12)])
    for row, value, weight in zip(ROWS[2:], step["say2"], ("bold", "normal")):
        out.append(rich(PX + 12, row, value, 16, INK, weight, room=W - 6 - PX - 12))
    return out


def need_bar(parts):
    """The blocks needed when the request is finished, on the scale of the pool.

    The blocks beyond the limit are tinted inside the session they belong to,
    so that the widths and the labels add up to the same total."""
    out = [text(SCALE.x0 - 8, NEED_Y + 11, "请求结束", 15, INK, "bold", "end"),
           text(SCALE.x0 - 8, NEED_Y + 28, "时需要", 15, INK, "bold", "end")]
    at, labels = 0, []
    for who, count in parts:
        pieces = ([(19, "cache", "已有 19 块"), (MISSING, "new", f"缺 {MISSING} 块")] if who == "B"
                  else [(count, "cache", f"{who}：{count} 块")])
        first = at
        for n, kind, label in pieces:
            x, w = SCALE.x(at), n * SCALE.unit
            out.append(box_of(x, NEED_Y, w, NEED_H, kind, 1.2, 0))
            labels += tagged(f"need/{who}/{n}", [text(x + w / 2, NEED_Y + 20, label, 16, INK, "bold")])
            at += n
        out.append(rect(SCALE.x(first), NEED_Y, (at - first) * SCALE.unit, NEED_H, "none",
                        OWNER[who], rx=0, width=2.4))
    under = NEED_Y + NEED_H
    if at > N_BLOCKS:
        x, end = SCALE.x1, SCALE.x(at)
        out.append(rect(x, NEED_Y + 1.4, end - x - 1.4, NEED_H - 2.8, FILL_RED, "none", rx=0))
        out.append(line(x + 3, under + 4, end, under + 4, RED, 2.2))
        out += tagged("over", [text((x + end) / 2, under + 20, f"超出 {at - N_BLOCKS} 块", 15, RED,
                                    "bold")])
    else:
        out.append(line(SCALE.x1, NEED_Y, SCALE.x1, under, INK, 3))
        out += tagged("total", [text(SCALE.x(at) + 8, NEED_Y + 20, f"共 {at} 块", 16, INK, "bold",
                                     "start")])
    out.append(line(SCALE.x1, POOL_Y, SCALE.x1, NEED_Y, INK, 3))
    out.append(line(SCALE.x1, under, SCALE.x1, under + 5, INK, 3))
    out.append(text(SCALE.x1 - 5, under + 20, f"上限 {N_BLOCKS} 块", 15, INK, "bold", "end"))
    return out + labels


def caption_line(number, sentence):
    lead = f"第 {number}/{len(STEPS)} 步"
    lead_w = width_of(lead, 18) + 14
    x = (W - lead_w - width_of(sentence, 19)) / 2
    return [text(x, CAPTION_Y, lead, 18, ORANGE, "bold", "start"),
            text(x + lead_w, CAPTION_Y, sentence, 19, INK, "bold", "start")]


RESERVE = ("fn", "kv_reserve(cache, B, 37, 8)")
COUNTED = "会作为输入：文本 37 个与回答前 7 个，共 44 个。第 8 个只输出"
STEPS = [
    dict(rule=0, now=OPENED, before=OPENED, need=FINAL[:1], reads=(("B", "capacity"),),
         call=[RESERVE, ("text", "数出需要的块")],
         say2=("已有的 19 块继续使用，只需再分配 25 块",
               "调用之前，B 因这次请求记为最近使用：`last_use` = 4 + 1 = 5"),
         say="B 的这次请求最多缓存 44 个 token：已有 19 块留在原处，只缺 25 块"),
    dict(rule=0, now=OPENED, before=OPENED, need=FINAL, read_bits=True,
         call=[RESERVE, ("text", "比较需要的块与空闲的块")],
         say2=("B 的 44 块在 96 块之内，它单独放得下", "空闲 14 块，比缺的 25 块少"),
         say="最终需要 A 42 + B 44 + C 21 = 107 > 96 块：真正超出了上限"),
    dict(rule=1, now=OPENED, before=OPENED, need=FINAL,
         reads=(("A", "n_cached"), ("A", "capacity"), ("C", "n_cached"), ("C", "capacity")),
         call=[RESERVE, ("text", "检查 A、C 预留而未用的块")],
         say2=("A、C 的 `capacity` 都等于 `n_cached`：可收回 0 块", "回答提前结束时，才会留下预留而未用的块"),
         say="先收回其他会话预留而未用的块：这次是 0 块，仍缺空闲块"),
    dict(rule=2, now=OPENED, before=OPENED, need=FINAL,
         reads=(("A", "last_use"), ("C", "last_use")),
         call=[RESERVE, ">", ("fn", "kv_victim(cache, B)")],
         say2=("候选是有块且不是 B 的会话：A 与 C", "最近最少使用（lru）：`last_use` 最小的是 A（2，C 是 4）"),
         say="仍不够，才选出一个会话失效：kv_victim 按 lru 返回 A"),
    dict(rule=2, now=INVALID, before=OPENED, need=[("B", NEED), ("C", 21)],
         freed=range(42),
         call=[("fn", "kv_reserve"), ">", ("fn", "kv_invalidate(A)"), ">", ("fw", "kv_free()"), ("text", "42 次")],
         say2=("释放块 0–41，A 的块表项都置 NULL", "空闲 14 → 56 块，不少于 25，停止失效"),
         say="kv_invalidate 释放 A 的 42 块：B 44 + C 21 = 65，已在上限之内"),
    dict(rule=3, now=AFTER_ALLOC, before=INVALID, need=[("B", NEED), ("C", 21)],
         call=[RESERVE, ">", ("fw", "kv_alloc()"), ("text", "25 次")],
         say2=("块 0–24 填入 B 的块表项 `block[19..43]`", "返回 1，表示放得下。此时在用 65 块"),
         say="只分配 25 块：B 原有的块 42–60 与其中的内容都不变"),
]


def frames():
    for number, step in enumerate(STEPS, 1):
        now = step["now"]
        out = rail(step["rule"]) + table(now, step["before"], step.get("reads", ())) + panel(step)
        out += SCALE.ticks((0, 24, 42, 61, 82, 95), BIT_Y - 5)
        out.append(text(SCALE.x0 - 8, BIT_Y + 16, "位图", 16, INK, "bold", "end"))
        out.append(text(SCALE.x0 - 8, POOL_Y + POOL_H / 2 + 6, "内存池", 16, INK, "bold", "end"))
        out += SCALE.pool(now, BIT_Y, POOL_Y, POOL_H, step["before"], step.get("freed", ()),
                          step.get("read_bits", False))
        out.append(text(SCALE.x1 + 8, BIT_Y + 16, "空闲", 15, MUTED, "bold", "start"))
        out += tagged("free", [text(SCALE.x1 + 8, POOL_Y + POOL_H / 2 + 7, f"{num_free(now)} 块", 18,
                                    INK, "bold", "start")])
        out += need_bar(step["need"])
        out += caption_line(number, step["say"])
        out += centred_legend(W / 2, LEGEND_Y,
                              [("cache", "有效"), ("reserved", "预留"), ("free", "空闲"),
                               ("new", "改变或缺少"), ("reuse", "读取"), ("freed", "失效释放"),
                               ("fn", "你实现"), ("frame", "框架")], 15, 16)
        yield out, now, step


if __name__ == "__main__":
    for number, (body, _, _) in enumerate(frames(), 1):
        save(f"kv-reserve-{number}", W, H, body)
