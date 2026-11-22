#!/usr/bin/env python3
"""The two policies on the same example: one start, then three requests.

The start is that of kv-memory, drawn again: A 42, B 19, C 21 blocks, 14
free, A used least recently. Then B, A and C send one request each, at most
44, 67 and 43 blocks, also drawn. Each bar is the pool when the request is
finished, at the positions of the blocks. lru invalidates A, then C and B in
this order, then A again: 4 times, 63 tokens computed again. smallest
invalidates C, B and A: 3 times, 21 tokens computed again.
Run it to refresh ../assets/kv-policies.svg.
"""

from kv_run_kit import *

W, H = 1120, 370
UNIT, BAR_H = 5.45, 26
COL_X = (4, 592)                        # left edge of each column's bars
START_Y = 24                            # the bar of the start, the chips of the requests
HEAD_Y = 94                             # the centre of a column's head
ROW_Y = (137, 200, 263)                 # bar tops, one request a row

# policy -> one row a request, in the order of REQUESTS:
#           (session, blocks it lacks, blocks free, [(invalidated, blocks)],
#            [(first block, blocks, session, kind)])
RUNS = {
    "lru": [("B", 25, 14, [("A", 42)],
             [(0, 25, "B", "new"), (42, 19, "B", "cache"), (61, 21, "C", "cache")]),
            ("A", 67, 31, [("C", 21), ("B", 44)],
             [(0, 42, "A", "waste"), (42, 25, "A", "new")]),
            ("C", 43, 29, [("A", 67)],
             [(0, 21, "C", "waste"), (21, 22, "C", "new")])],
    "smallest": [("B", 25, 14, [("C", 21)],
                  [(0, 42, "A", "cache"), (42, 19, "B", "cache"), (61, 25, "B", "new")]),
                 ("A", 25, 10, [("B", 44)],
                  [(0, 42, "A", "cache"), (42, 25, "A", "new")]),
                 ("C", 43, 29, [("A", 67)],
                  [(0, 21, "C", "waste"), (21, 22, "C", "new")])],
}
HEAD = {"lru": "最近最少使用：取 last_use 最小", "smallest": "最小缓存优先：取 n_cached 最小"}
WORD = {"cache": "", "new": "新 ", "waste": "重算 "}


def start():
    scale = Scale(COL_X[0], UNIT)
    out = [text(scale.x0, 16, "相同的起点：各会话已缓存的 token，各占一块", 16, INK, "bold", "start"),
           rect(scale.x0, START_Y, N_BLOCKS * UNIT, BAR_H, *KINDS["free"], rx=0, width=1.2)]
    for who, entry in START.items():
        first, count = entry["runs"][0]
        x, w = scale.x(first), count * UNIT
        out.append(box_of(x, START_Y, w, BAR_H, "cache", 1.3, 0))
        out.append(rect(x + 1, START_Y + BAR_H + 2, w - 2, 5, OWNER[who], OWNER[who], rx=1, width=1))
        out += tagged(f"start/{who}", [text(x + w / 2, START_Y + 19, f"{who} {count}", 16, INK, "bold")])
    used = in_use(START)
    out += tagged("start/free", [text((scale.x(used) + scale.x1) / 2, START_Y + 19,
                                      f"空闲 {num_free(START)}", 15, MUTED, "bold")])
    out.append(line(scale.x1, START_Y - 3, scale.x1, START_Y + BAR_H + 6, INK, 2.6))
    order = sorted(START, key=lambda who: START[who]["last_use"])
    out += tagged("start/order", [text(scale.x0, 74, f"最近使用的先后（从久到近）：{'、'.join(order)}，"
                                       f"last_use 是 {'、'.join(str(START[w]['last_use']) for w in order)}",
                                       15, INK, anchor="start")])
    x0 = COL_X[1]
    out.append(text(x0, 16, f"随后依次到来三次请求，每次最多回答 {MAX_ANSWER} 个 token，实际都是 {MAX_ANSWER} 个",
                    16, INK, "bold", "start"))
    for i, (who, n_text, need) in enumerate(REQUESTS):
        x, w = x0 + i * 181, 160
        out.append(rect(x, START_Y - 1, w, BAR_H + 6, WHITE, OWNER[who], rx=5, width=2))
        out += tagged(f"request/{i}", [text(x + w / 2, START_Y + 20,
                                            f"{who}：{n_text} + {need - n_text} = {need} 块", 16,
                                            OWNER[who], "bold")])
        if i:
            out.append(arrow(x - 19, START_Y + 15, x - 3, START_Y + 15, MUTED, 1.7))
    out.append(text(x0, 74, "需要：文本 + 回答的前 7 个。发请求的会话记为最近使用，不会被选中", 15, INK,
                    anchor="start"))
    return out


def column(x0, policy):
    out = []
    scale = Scale(x0, UNIT)
    shapes, _, p1 = fn(x0, HEAD_Y, "kv_victim", 14, h=22)
    out += shapes
    out.append(mono(p1 + 8, HEAD_Y + 6, policy, 16, INK, "bold"))
    out.append(text(p1 + 16 + width_of(policy, 16, MONO), HEAD_Y + 6, HEAD[policy], 16, INK, anchor="start"))
    again = []
    count = 0
    for i, ((who, lacks, free, gone, parts), y) in enumerate(zip(RUNS[policy], ROW_Y)):
        out += tagged(f"{policy}/{i}/need",
                      [text(x0, y - 8, f"{who} 的请求：缺 {lacks} 块，空闲 {free}", 15, INK, "bold", "start")])
        names = [f"{name}（{blocks} 块）" for name, blocks in gone]
        label = f"先 {names[0]}后 {names[1]}失效" if len(gone) == 2 else f"{names[0]}失效"
        out += tagged(f"{policy}/{i}/gone", [text(scale.x1, y - 8, label, 15, RED, "bold", "end")])
        out.append(rect(x0, y, N_BLOCKS * UNIT, BAR_H, *KINDS["free"], rx=0, width=1.2))
        for first, blocks, owner, kind in parts:
            x, w = scale.x(first), blocks * UNIT
            out.append(box_of(x, y, w, BAR_H, kind, 1.3, 0))
            out.append(rect(x + 1, y + BAR_H + 2, w - 2, 5, OWNER[owner], OWNER[owner], rx=1, width=1))
            out += tagged(f"{policy}/{i}/part/{first}/{blocks}/{owner}/{kind}",
                          [text(x + w / 2, y + 19, f"{owner} {WORD[kind]}{blocks}", 16, INK, "bold")])
            if kind == "waste":
                again.append(blocks)
        count += len(gone)
        out.append(line(scale.x1, y - 3, scale.x1, y + BAR_H + 6, INK, 2.6))
    ty = ROW_Y[-1] + BAR_H + 32
    total = " + ".join(map(str, again)) + f" = {sum(again)}" if len(again) > 1 else str(sum(again))
    out += tagged(f"{policy}/recomputed", [text(x0, ty, f"重新计算 {total} 个 token", 18, RED, "bold",
                                                "start")])
    out += tagged(f"{policy}/invalidations", [text(x0 + 285, ty, f"失效 {count} 次", 18, INK, "bold",
                                                   "start")])
    out.append(text(scale.x1, ty, f"右端是上限 {N_BLOCKS} 块", 15, MUTED, anchor="end"))
    return out


def figure():
    out = start() + [line(2, 80, W - 2, 80, LINE, 1.2)]
    out += column(COL_X[0], "lru") + column(COL_X[1], "smallest")
    out.append(line(W / 2, 84, W / 2, ROW_Y[-1] + BAR_H + 40, LINE, 1.2, "4 4"))
    out += centred_legend(W / 2, H - 11, [("cache", "已缓存，直接读取"), ("new", "这次请求的新 token"),
                                          ("waste", "失效后重新计算"), ("free", "空闲")], 16)
    return out


if __name__ == "__main__":
    save("kv-policies", W, H, figure())
