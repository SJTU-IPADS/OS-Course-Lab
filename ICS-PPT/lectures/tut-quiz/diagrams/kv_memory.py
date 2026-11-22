#!/usr/bin/env python3
"""The example of the third subsection, with everything it rests on, to scale.

The state before: A, B and C have cached 42, 19 and 21 tokens, one block
each, in a pool of 96 blocks, with their last_use. The request of B: 20
tokens of text and 17 of the new message, at most 8 of answer. The tokens
that serve as input are counted cell by cell: 44, so 25 blocks are missing
and 14 are free. The page ends with a question and this figure holds no
answer.
Run it to refresh ../assets/kv-memory.svg.
"""

from kv_run_kit import *

W, H = 1120, 336
SCALE = Scale(84, 9.6)                  # that of kv_reserve.py
BIT_Y, POOL_Y, POOL_H = 176, 200, 38
COLS = [("会话", "", 54), ("n_cached", "已缓存的 token", 132), ("last_use", "越大越近使用", 132),
        ("块", "在内存池中的编号", 164)]
TX, TY, RH = 2, 63, 26
QX, UNIT, CY, CH = 538, 12.8, 52, 24    # the tokens of the request


def state_table():
    out, x = [text(TX, 18, "现在的状态", 17, INK, "bold", "start")], TX
    for name, meaning, w in COLS:
        out.append(mono(x + w / 2, 40, name, 15, MUTED, "bold", anchor="middle") if name.isascii()
                   else text(x + w / 2, 40, name, 15, MUTED, "bold"))
        if meaning:
            out.append(text(x + w / 2, 57, meaning, 14, MUTED))
        x += w
    for i, (who, entry) in enumerate(START.items()):
        y, x = TY + i * RH, TX
        out.append(rect(TX, y, sum(c[2] for c in COLS), RH, WHITE, LINE, rx=2, width=1.1))
        out.append(mono(TX + COLS[0][2] / 2, y + 19, who, 17, OWNER[who], "bold", anchor="middle"))
        x += COLS[0][2]
        for key, value, (_, _, w) in zip(("n_cached", "last_use", "blocks"),
                                         (entry["n_cached"], entry["last_use"], ranges(entry)), COLS[1:]):
            out += tagged(f"field/{who}/{key}", [text(x + w / 2, y + 19, str(value), 17, INK, "bold")])
            x += w
    return out


def request():
    cached, n_text = START["B"]["n_cached"], B_TEXT + B_MESSAGE
    n_all, at = n_text + MAX_ANSWER, lambda k: QX + k * UNIT
    out = tagged("request", [text(QX, 18, f"B 的这次请求：文本 {B_TEXT} + {B_MESSAGE} = {n_text} 个 token，"
                                         f"最多回答 {MAX_ANSWER} 个", 17, INK, "bold", "start")])
    for first, end, label in ((0, B_TEXT, f"已有文本 {B_TEXT} 个"),
                              (B_TEXT, n_text, f"新消息 {B_MESSAGE} 个"),
                              (n_text, n_all, f"回答最多 {MAX_ANSWER} 个")):
        out += brace(at(first) + 1.5, at(end) - 1.5, CY - 3, MUTED, label, 15, 6, below=False)
    for k in range(n_all):
        kind = "cache" if k < cached else "new" if k < NEED else "output"
        out += tagged(f"token/{k}/{kind}", [box_of(at(k), CY, UNIT, CH, kind, 1, 0)])
    for edge in (B_TEXT, n_text):
        out.append(line(at(edge), CY - 2, at(edge), CY + CH + 2, INK, 2.2))
    y = CY + CH + 3
    out += tagged("have", brace(at(0) + 1.5, at(cached) - 1.5, y, BLUE, f"已缓存 {cached} 个，有 {cached} 块",
                                15, 6))
    out += tagged("lack", brace(at(cached) + 1.5, at(NEED) - 1.5, y, ORANGE, f"缺 {NEED - cached} 块", 15, 6))
    out.append(text(at(n_all), y + 25, "只输出", 15, MUTED, "bold", "end"))
    out += tagged("count", [text(QX, 126, f"会作为输入：文本 {n_text} 个与回答的前 {NEED - n_text} 个，共 {NEED} 个",
                                 17, INK, "bold", "start")])
    out.append(text(QX, 147, f"已有文本的第 {B_TEXT} 个上次只输出，还没有块", 15, MUTED, anchor="start"))
    return out


def figure():
    used, free = in_use(START), num_free(START)
    out = state_table() + request()
    out += SCALE.ticks((0, 42, 61, 82, 95), BIT_Y - 5)
    out.append(text(SCALE.x0 - 8, BIT_Y + 16, "位图", 16, INK, "bold", "end"))
    out.append(text(SCALE.x0 - 8, POOL_Y + 17, "内存池", 16, INK, "bold", "end"))
    out += tagged("blocks", [text(SCALE.x0 - 8, POOL_Y + 36, f"共 {N_BLOCKS} 块", 15, INK, "bold", "end")])
    out += SCALE.pool(START, BIT_Y, POOL_Y, POOL_H, free_names=False)
    y = POOL_Y + POOL_H + 29
    out += tagged("used", brace(SCALE.x0, SCALE.x(used) - 2, y, INK, f"在用 {used} 块", size=16))
    out += tagged("free", brace(SCALE.x(used) + 2, SCALE.x1, y, MUTED, f"空闲 {free} 块", size=16))
    out.append(text(SCALE.x0, H - 9, "池中一格是一块，保存一个 token 的键与值", 15, MUTED, anchor="start"))
    keys = [("cache", "已缓存"), ("free", "空闲"), ("new", "还没有块的 token"), ("output", "只输出")]
    return out + legend(SCALE.x1 - legend_width(keys, 15, 18), H - 10, keys, 15, 18)


if __name__ == "__main__":
    save("kv-memory", W, H, figure())
