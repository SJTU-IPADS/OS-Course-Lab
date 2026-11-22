#!/usr/bin/env python3
"""Four measurements on the real model, with what was measured drawn above them.

Three sessions send seven requests from an empty cache, A A B C B A C, at
most 8 tokens of answer each. Under each request: what its session has
cached afterwards, and how often the keys and values of earlier tokens are
computed again without a KV cache. A row is named by what it means. The
seconds and the counts are those of build/scratch-kvblocks/handoff/runs.md,
one run each.
Run it to refresh ../assets/kv-run-time.svg.
"""

from kv_kit import *

W, H = 1120, 446
# session, tokens of its text, tokens it has cached after the request, earlier rows computed
# again without a cache
SENT = [("A", 13, 20, 190), ("A", 35, 42, 671), ("B", 12, 19, 171), ("C", 14, 21, 210),
        ("B", 37, 44, 775), ("A", 60, 67, 1350), ("C", 36, 43, 693)]
NEW = 154
# name, what happened, seconds, recomputed tokens (None: no KV cache)
RUNS = [("182 块：全部会话都放得下", "没有失效", 181.9, 0),
        ("96 块，最小缓存优先", "失效 3 次，重新计算 21 个 token", 220.2, 21),
        ("96 块，最近最少使用", "失效 4 次，重新计算 63 个 token", 272.7, 63),
        ("不使用 KV Cache", "每一步重新计算之前 token 的键与值", 616.8, None)]

SX, SP, SW = 318, 72, 50                # the requests
ROW0, PITCH, BH = 126, 68, 32
TX, TW = 360, 310                       # time bars: left edge, the longest bar
KX, KS = 792, 1.35                      # token bars: left edge, px per token


def setting():
    out = [text(4, 22, "请求的先后", 17, INK, "bold", "start"),
           text(4, 51, "文本 + 回答的前 7 个 = 之后缓存的 token", 15, INK, anchor="start"),
           text(4, 75, "不使用 KV Cache 时重新计算的次数", 15, RED, anchor="start")]
    last = {}
    for i, (who, n_text, cached, again) in enumerate(SENT):
        x = SX + i * SP
        mid = x + SW / 2
        out.append(rect(x, 2, SW, 28, WHITE, OWNER[who], rx=5, width=2))
        out += tagged(f"sent/{i}/who", [mono(mid, 22, who, 18, OWNER[who], "bold", anchor="middle")])
        if i:
            out.append(arrow(x - SP + SW + 2, 16, x - 3, 16, MUTED, 1.6))
        out += tagged(f"sent/{i}/cached", [text(mid, 51, f"{n_text}+{cached - n_text}={cached}", 15, INK, "bold")])
        out += tagged(f"sent/{i}/again", [text(mid, 75, str(again), 16, RED, "bold")])
        last[who] = cached
    tail = SX + len(SENT) * SP + 4
    out += tagged("total/cached", [text(tail, 51, " + ".join(str(last[w]) for w in "ABC")
                                        + f" = {NEW} 个 token", 16, INK, "bold", "start")])
    out += tagged("total/again", [text(tail, 75, f"共 {sum(s[3] for s in SENT)} 次，比如 190 = 0+1+…+19", 15, RED, "bold", "start")])
    out.append(line(2, 85, W - 2, 85, LINE, 1.2))
    return out


def figure():
    scale = TW / max(run[2] for run in RUNS)
    out = setting()
    out += [text(TX, 110, "运行时间（各测一次）", 17, INK, "bold", "start"),
            text(KX, 110, "经过模型的 token", 17, INK, "bold", "start")]
    for i, (name, what, seconds, again) in enumerate(RUNS):
        y = ROW0 + i * PITCH
        out += tagged(f"run/{i}/name", [text(4, y + 14, name, 18, INK, "bold", "start")])
        out += tagged(f"run/{i}/what", [text(4, y + 38, what, 16, MUTED, anchor="start")])
        w = seconds * scale
        out += cell(TX, y, w, BH, "", "mid", rx=3)
        out += tagged(f"run/{i}/seconds", [text(TX + w + 8, y + BH / 2 + 6, f"{seconds:.1f} s", 18, INK,
                                                "bold", "start")])
        out += tagged(f"run/{i}/new", cell(KX, y, NEW * KS, BH, str(NEW), "new", size=16, rx=2))
        if again:
            out += tagged(f"run/{i}/again", cell(KX + NEW * KS, y, again * KS, BH, str(again), "waste",
                                                 size=15, rx=2))
        if again is None:
            out += tagged(f"run/{i}/rows", [text(KX, y + BH + 21, f"另有键与值的重新计算 "
                                                 f"{sum(s[3] for s in SENT)} 次", 16, RED, "bold", "start")])
    for x in (TX, KX):
        out.append(line(x, ROW0 - 8, x, ROW0 + 3 * PITCH + BH + 4, MUTED, 1.2))
    out.append(caption(W / 2, 409, "使用 KV Cache 时，运行时间随经过模型的 token 数增长"))
    out += centred_legend(W / 2, H - 9, [("mid", "运行时间"), ("new", "第一次计算的 token"),
                                         ("waste", "失效后重新计算的 token")], 16)
    return out


if __name__ == "__main__":
    save("kv-run-time", W, H, figure())
