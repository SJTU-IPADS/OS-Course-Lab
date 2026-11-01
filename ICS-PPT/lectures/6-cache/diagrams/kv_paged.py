#!/usr/bin/env python3
"""Allocating the KV cache in blocks of four tokens, one frame per event. The pool has twelve
physical blocks; every request has a block table from its logical block numbers to physical
block numbers. The allocation (free block with the lowest number first) is simulated below, and
so are the counts under the table. Run it to refresh ../assets/kv-paged-1.svg ... kv-paged-6.svg."""

from kv_kit import *
from cachekit import SET_FILLS, SET_STROKES

W, H = 1040, 404
BLOCK, POOL = 4, 12
PER_ROW = 6
TW, TH = 24, 30                          # a token cell
PX, PY, PPITCH, RPITCH = 372, 56, 112, 118
TX = 20                                  # the block tables

# (event text, what happens): ("arrive", name, tokens) ("grow", name) ("finish", name)
EVENTS = [
    ("请求 A、B 到达：A 有 5 个 token，需要 2 块，B 有 3 个，需要 1 块", [("arrive", "A", 5), ("arrive", "B", 3)]),
    ("B 生成 1 个 token，放进最后一块的空位，不分配新块", [("grow", "B")]),
    ("B 再生成 1 个 token，最后一块已满，分配新块", [("grow", "B")]),
    ("请求 C 到达：6 个 token，需要 2 块", [("arrive", "C", 6)]),
    ("请求 A 结束，释放它的块", [("finish", "A")]),
    ("请求 D 到达：9 个 token，需要 3 块，取到不连续的空闲块", [("arrive", "D", 9)]),
]
NAMES = "ABCD"


def simulate():
    """One state per event: tokens, tables, the block just allocated, the token just added."""
    tokens, tables, free = {}, {}, list(range(POOL))
    states = []
    for _, acts in EVENTS:
        fresh, added = [], None
        for act in acts:
            if act[0] == "arrive":
                _, name, n = act
                tokens[name] = n
                need = -(-n // BLOCK)
                tables[name] = [free.pop(0) for _ in range(need)]
                fresh += tables[name]
            elif act[0] == "grow":
                name = act[1]
                tokens[name] += 1
                added = (name, tokens[name] - 1)
                if (tokens[name] - 1) // BLOCK == len(tables[name]):
                    block = free.pop(0)
                    tables[name].append(block)
                    fresh.append(block)
            else:
                name = act[1]
                free += tables.pop(name)
                free.sort()
                tokens.pop(name)
        states.append(dict(tokens=dict(tokens), tables={k: list(v) for k, v in tables.items()},
                           fresh=fresh, added=added, free=list(free)))
    return states


def owner(state):
    own = {}
    for name, table in state["tables"].items():
        for logical, block in enumerate(table):
            own[block] = (name, logical)
    return own


def pool(state):
    out = [text(PX + (PER_ROW * PPITCH - 8) / 2, 28, "物理块（显存中的 12 块）", 17, INK, "bold")]
    own = owner(state)
    for b in range(POOL):
        x = PX + (b % PER_ROW) * PPITCH
        y = PY + (b // PER_ROW) * RPITCH
        out.append(text(x, y + 14, f"P{b}", 15, MUTED, "bold", "start"))
        if b in own:
            name, logical = own[b]
            shade = NAMES.index(name)
            n_in = min(BLOCK, state["tokens"][name] - logical * BLOCK)
            fresh = b in state["fresh"]
            out.append(rect(x - 3, y + 20, BLOCK * TW + 6, TH + 6, "none",
                            ORANGE if fresh else "none", rx=5, width=3))
            for k in range(BLOCK):
                cx = x + k * TW
                if k < n_in:
                    just = state["added"] == (name, logical * BLOCK + k)
                    if just:
                        out += cell(cx, y + 23, TW, TH, name, "new", size=14)
                    else:
                        out.append(rect(cx, y + 23, TW, TH, SET_FILLS[shade], SET_STROKES[shade],
                                        rx=3, width=1.4))
                        out.append(mono(cx + TW / 2, y + 23 + TH / 2 + 5, name, 14, INK, "bold",
                                        "middle"))
                else:
                    out += cell(cx, y + 23, TW, TH, "", "empty")
        else:
            for k in range(BLOCK):
                out += cell(x + k * TW, y + 23, TW, TH, "", "empty")
    return out


def tables(state):
    out = [text(TX + 160, 28, "块表：逻辑块号 → 物理块号", 17, INK, "bold")]
    for k, name in enumerate(NAMES):
        y = PY + k * 58
        if name not in state["tables"]:
            out.append(text(TX, y + 22, f"请求 {name}", 16, LINE, "bold", "start"))
            continue
        out.append(text(TX, y + 22, f"请求 {name}", 16, INK, "bold", "start"))
        for logical, block in enumerate(state["tables"][name]):
            fresh = block in state["fresh"]
            out += cell(TX + 74 + logical * 82, y, 74, 32, f"{logical} → P{block}",
                        "new" if fresh else "cache", size=14)
    return out


def frame(n):
    state = simulate()[n - 1]
    out = tables(state) + pool(state)
    used = sum(len(t) for t in state["tables"].values())
    idle = sum(len(t) * BLOCK - state["tokens"][name] for name, t in state["tables"].items())
    out.append(text(TX, 330, f"占用 {used} 块，空闲 {POOL - used} 块", 17, INK, "bold", "start"))
    out.append(text(TX, 356, f"块内未填的位置 {idle} 个，每个请求至多 {BLOCK - 1} 个", 16, MUTED,
                    "normal", "start"))
    out.append(text(W / 2 + 120, 310, EVENTS[n - 1][0], 19, INK, "bold"))
    out += legend(400, 392, [("new", "本步新分配或新写入"), ("cache", "已占用"), ("empty", "空闲")])
    return out


if __name__ == "__main__":
    for n in range(1, len(EVENTS) + 1):
        save(f"kv-paged-{n}", W, H, frame(n))
