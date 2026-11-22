"""The small example of the second subsection of part three, and its drawing.

kv_layout.py, kv_locate.py, kv_first.py and kv_next.py draw one example:
2 layers, rows of 10 floats (one key-value head, d = 10), a block of 40
floats or 160 bytes, a pool of 12 blocks, at most 2 answer tokens a turn.
Session A (id 0) takes t0 t1 t2 in its first turn and t5 in its second,
session B (id 1) takes b0 in between.

A block is one column of the pool, its four rows from the top at the
offsets 0, 10, 20, 30. Block numbers always carry 块 and token numbers t or
b, since the two differ from token 4 on.

A frame of the two animations has fixed areas, from top to bottom: the
steps of serve; who calls what with which arguments, what is computed and
returned; the tokens, each above its entry of the block table; the session
table; the pool with its bitmap; one sentence. A step states only what
changes. The block of a token is always looked up in the block table of the
state, and a change is drawn from the difference to the frame before.
"""

from kv_kit import *

N_LAYER, KV_DIM, FLOAT_BYTES = 2, 10, 4
N_BLOCKS = 12
SEGMENTS = [(layer, kind) for layer in range(N_LAYER) for kind in (0, 1)]
BLOCK_BYTES = len(SEGMENTS) * KV_DIM * FLOAT_BYTES
ENTRIES = 8                             # entries of a block table that are drawn
LETTER = {"A": "t", "B": "b"}


def offset(layer, kind):
    """The float at which a row starts in its block."""
    return (2 * layer + kind) * KV_DIM


def token_name(who, tok):
    return f"${LETTER[who]}_{tok}$"


def row_name(who, kind, tok):
    return f"${KV[kind]}_{tok}$" if who == "A" else f"${KV[kind]}_{{b{tok}}}$"


# ------------------------------------------------------------ the state

def empty_state():
    return {"clock": 0, "n_sessions": 0, "A": None, "B": None}


def entry_of(number, n_cached=0, last_use=0, blocks=()):
    return dict(id=number, n_cached=n_cached, capacity=len(blocks), last_use=last_use,
                block=list(blocks) + [None] * (ENTRIES - len(blocks)))


def turn2_start():
    """After the first turn of A."""
    return {"clock": 1, "n_sessions": 1, "A": entry_of(0, 4, 1, (0, 1, 2, 3)), "B": None}


def snapshot():
    """The moment of kv-layout and kv-locate: t4 computed, t5 not yet."""
    return {"clock": 3, "n_sessions": 2, "A": entry_of(0, 5, 3, (0, 1, 2, 3, 6, 7, 8)),
            "B": entry_of(1, 2, 2, (4, 5))}


def copy_state(state):
    out = dict(state)
    for who in "AB":
        if state[who]:
            out[who] = dict(state[who], block=list(state[who]["block"]))
    return out


def advance(state, step):
    """The state after a step: open, add, cached name what it changes."""
    now = copy_state(state)
    if "open" in step:
        who, number, last_use = step["open"]
        if not now[who]:
            now[who] = entry_of(number)
            now["n_sessions"] += 1
        now["clock"] = now[who]["last_use"] = last_use
    if "add" in step:
        who, blocks = step["add"]
        entry = now[who]
        for block in blocks:
            entry["block"][entry["capacity"]] = block
            entry["capacity"] += 1
    if "cached" in step:
        who, count = step["cached"]
        now[who]["n_cached"] = count
    return now


def owners(state):
    """block -> (session, token), read from the block tables."""
    out = {}
    for who in "AB":
        if state[who]:
            for tok, block in enumerate(state[who]["block"]):
                if block is not None:
                    out[block] = (who, tok)
    return out


def status(state, owner):
    if owner is None:
        return "free"
    who, tok = owner
    return "cache" if tok < state[who]["n_cached"] else "reserved"


# ------------------------------------------------------------ the pool

class Pool:
    """Twelve blocks, each a column of one row a segment, with the bitmap above."""

    X0, PITCH, CW = 134, 82, 78

    def __init__(self, head, row_h, bit_h=20):
        self.head, self.row_h, self.bit_h = head, row_h, bit_h
        self.bit_y = head + 5
        self.seg_y = self.bit_y + bit_h + 4
        self.band_y = self.seg_y + len(SEGMENTS) * row_h + 2
        self.bottom = self.band_y + 20

    def x(self, block):
        return self.X0 + block * self.PITCH

    def mid(self, block):
        return self.x(block) + self.CW / 2

    def labels(self):
        out = [text(128, self.bit_y + self.bit_h / 2 + 6, "位图", 16, INK, "bold", "end"),
               text(128, self.band_y + 15, "属于", 15, MUTED, "bold", "end")]
        for row, (layer, kind) in enumerate(SEGMENTS):
            base = self.seg_y + (row + 0.5) * self.row_h + 5
            out.append(text(88, base, f"第 {layer} 层 {KV[kind].upper()}", 16, INK, "bold", "end"))
            out.append(mono(128, base, f"+{offset(layer, kind)}", 15, BLUE, "bold", anchor="end"))
        return out

    def draw(self, state, before=None, marks=None, written=(), missing=(), focus=()):
        marks, before = marks or {}, before or state
        now_of, was_of = owners(state), owners(before)
        out, rings = self.labels(), []
        for block in range(N_BLOCKS):
            x, owner = self.x(block), now_of.get(block)
            kind_of = status(state, owner)
            modes = [marks.get(owner + key, "") if owner else "" for key in SEGMENTS]
            tone = (ORANGE if "w" in modes or "wr" in modes else GREEN if "r" in modes
                    else INK if owner or block in focus else MUTED)
            out += tagged(f"head/{block}", [text(x + self.CW / 2, self.head, f"块 {block}", 16,
                                                  tone, "bold")])
            bit, was = int(owner is not None), int(block in was_of)
            changed = bit != was
            fill, stroke = KINDS["new"] if changed else (WHITE, MUTED)
            out.append(rect(x, self.bit_y, self.CW, self.bit_h, fill, stroke, rx=2,
                            width=2 if changed else 1.1))
            out += tagged(f"bit/{block}",
                          [mono(x + self.CW / 2, self.bit_y + self.bit_h / 2 + 5.5,
                                f"{was}→{bit}" if changed else str(bit), 16,
                                ORANGE if changed else INK, "bold", anchor="middle")])
            for row, (layer, kind) in enumerate(SEGMENTS):
                y, mode = self.seg_y + row * self.row_h, modes[row]
                fresh = bool(owner) and owner + (layer,) in written
                look = ("new" if mode in ("w", "wr") or fresh else "reuse" if mode == "r"
                        else kind_of)
                out.append(box_of(x, y, self.CW, self.row_h, look, 1.1, 0))
                word = {"r": "读", "w": "写", "wr": "写读", "a": "地址"}.get(mode, "")
                named = bool(owner) and (kind_of == "cache" or look == "new")
                shapes = []
                size = 17 if self.row_h >= 22 else 15
                base = y + self.row_h / 2 + (5 if self.row_h >= 22 else 3.5)
                if named:
                    shapes.append(rich(x + self.CW / 2 - (17 if word else 0), base,
                                       row_name(owner[0], kind, owner[1]), size, anchor="middle"))
                if word:
                    shift = 17 if named else 0
                    shapes.append(text(x + self.CW / 2 + shift, base, word, 14,
                                       {"r": GREEN, "a": INK}.get(mode, ORANGE), "bold"))
                    colour = {"r": GREEN, "a": INK}.get(mode, ORANGE)
                    rings.append(rect(x + 1.2, y + 1.2, self.CW - 2.4, self.row_h - 2.4, "none",
                                      colour, rx=1, width=2.3))
                out += tagged(f"row/{block}/{offset(layer, kind)}", shapes)
            if block in missing:
                mid = self.seg_y + 2 * self.row_h
                tok = owner[1]
                out.append(text(x + self.CW / 2, mid - 5, "还没有", 16, INK, "bold"))
                out.append(rich(x + self.CW / 2, mid + 17, f"$k_{tok}$、$v_{tok}$", 16, INK, "bold",
                                anchor="middle"))
            edge = INK if block in focus or block in missing else MUTED
            out.append(rect(x, self.seg_y, self.CW, 4 * self.row_h, "none", edge, rx=1,
                            width=2.8 if edge == INK else 1.6))
            if owner:
                who, tok = owner
                valid = kind_of == "cache"
                colour = OWNER[who]
                out.append(rect(x, self.band_y, self.CW, 20, colour if valid else WHITE, colour,
                                rx=3, width=1.5))
                label = f"{who} {token_name(who, tok)}" + ("" if valid else " 预留")
                shapes = [rich(x + self.CW / 2, self.band_y + 15, label, 15,
                               WHITE if valid else colour, "bold", anchor="middle")]
            else:
                shapes = [text(x + self.CW / 2, self.band_y + 15, "空闲", 15, MUTED)]
            out += tagged(f"own/{block}", shapes)
        return out + rings


# ------------------------------------------------------------ a frame

W, H = 1120, 500
PANEL_Y, PANEL_H = 35, 94
ROWS = (52, 82, 112)
VX = 70                                 # the values of the panel
EX, EP, EW = 382, 80, 78                # tokens and entries of the block table
TOKEN_Y, TOKEN_H = 156, 27
HEAD_Y, TABLE_Y, TABLE_H = 203, 209, 31
COLUMNS = [("会话", 62), ("id", 36), ("n_cached", 94), ("capacity", 94), ("last_use", 88)]
POOL_TITLE_Y = 290
FRAME_POOL = Pool(316, 23)
CAPTION_Y = 490
STAGES = [("fw", "append_message"), ("given", "kv_open"), ("fn", "kv_reserve"), ("given", "compute"),
          ("fw", "next_token"), ("end", "本轮结束")]


def entry_x(tok):
    return EX + tok * EP


def rail(active):
    """serve, one turn: the steps in their order, the current one ringed."""
    active = (active,) if isinstance(active, int) else active
    out = [mono(2, 20, "serve", 15, INK, "bold"), text(50, 20, "处理一轮", 15, INK, "bold", "start")]
    x = 118
    for i, (kind, name) in enumerate(STAGES):
        if i:
            out.append(arrow(x + 4, 15, x + 23, 15, MUTED, 1.6, both=name == "next_token"))
            x += 28
        if kind == "end":
            x0, x1 = x, x + width_of(name, 15) + 10
            shapes = [text((x0 + x1) / 2, 20, name, 15, INK, "bold")]
        else:
            shapes, x0, x1 = SHAPE[kind][0](x, 15, name, 15, h=24)
        out += shapes if i in active or kind in ("fn", "given") else faded(shapes)
        if i in active:
            out.append(rect(x0 - 3, 1, x1 - x0 + 6, 28, "none", ORANGE,
                            rx=14 if kind in ("fn", "given") else 5, width=2.8))
        x = x1
    keys = [("fn", "你实现"), ("given", "已给出"), ("frame", "框架")]
    left = W - 2 - legend_width(keys, 15, 12)
    if x + 8 > left:
        raise ValueError("rail too wide")
    return out + legend(left, 20, keys, 15, 12)


def address_row(row, state, spec, kind):
    """The addresses that kv_key or kv_value returns, each above its block table entry."""
    who, layer, toks = spec
    at = offset(layer, kind)
    out, _, x1 = fn(VX, row, ("kv_key", "kv_value")[kind], 15, h=24)
    out.append(mono(x1 + 6, row + 5, f"block[tok] + {at}", 14, INK))
    out.append(mono(EX - 6, row + 5, ("keys", "values")[kind], 15, INK, "bold", anchor="end"))
    for tok in toks:
        x = entry_x(tok)
        last = tok == toks[-1]
        out.append(box_of(x, row - 12.5, EW, 25, "addr", 2.2 if last else 1.2))
        out += tagged(f"addr/{('keys', 'values')[kind]}/{tok}",
                      [text(x + EW / 2, row + 5.5, f"块 {state[who]['block'][tok]} + {at}", 16, INK,
                            "bold" if last else "normal")])
    return out


def panel(step, state):
    """Who calls what with which arguments; what is computed; what comes back."""
    out = [rect(1, PANEL_Y, W - 2, PANEL_H, WHITE, MUTED, rx=7, width=1.3),
           line(VX - 10, PANEL_Y + 6, VX - 10, PANEL_Y + PANEL_H - 6, LINE, 1)]
    addresses = "addresses" in step
    labels = step.get("labels", ("调用", "返回", "") if addresses else ("调用", "计算", "返回"))
    for label, row in zip(labels, ROWS):
        if label:
            out.append(text(12, row + 6, label, 17, MUTED, "bold", "start"))
    out += call_line(VX, ROWS[0], step["call"], W - 4)
    if addresses:
        for kind in (0, 1):
            out += address_row(ROWS[1 + kind], state, step["addresses"], kind)
        return out
    for row, value, weight in zip(ROWS[1:], (step.get("calc", ""), step.get("ret", "")),
                                  ("bold", "normal")):
        if isinstance(value, list):
            out += call_line(VX, row, value, W - 4)
        elif value:
            out.append(rich(VX, row + 6, value, 18, INK, weight, room=W - 6 - VX))
    return out


def token_row(step, state, who, request):
    """The text of the session: each token above its entry of the block table."""
    out = [rich(2, 151, request, 14, INK, "bold", room=EX - 4)]
    entry = state[who]
    n_cached = entry["n_cached"] if entry else 0
    n_text, n_shown = step["text"], step.get("shown", step["text"])
    for first, count, name in step.get("groups", ()):
        x0, x1 = entry_x(first) + 3, entry_x(first + count - 1) + EW - 3
        half = rich_width(name, 15) / 2 + 5
        mid = (x0 + x1) / 2
        out.append(path(f"M {x0} 153 L {x0} 147 L {max(x0, mid - half):.1f} 147", MUTED, 1.4,
                        head=False))
        out.append(path(f"M {x1} 153 L {x1} 147 L {min(x1, mid + half):.1f} 147", MUTED, 1.4,
                        head=False))
        out.append(rich(mid, 151, name, 15, INK, "bold", anchor="middle"))
    out_only = step.get("output")
    for tok in range(n_shown):
        x, fill, stroke, dash, width = entry_x(tok), WHITE, MUTED, None, 1.3
        if tok in step.get("current", ()):
            (fill, stroke), width = KINDS["new"], 2.6
        elif tok < n_cached:
            fill, stroke = KINDS["cache"]
        if tok in step.get("fresh", ()):
            stroke, width = ORANGE, 2.6
        if tok >= n_text:
            dash = "5 4"
        if out_only and tok == out_only[0]:
            fill, stroke, width, dash = FILL_GREY, INK, 2.2, "5 4"
        out.append(rect(x, TOKEN_Y, EW, TOKEN_H, fill, stroke, rx=3, width=width, dash=dash))
        out += tagged(f"token/{tok}", [rich(x + EW / 2, TOKEN_Y + 19, token_name(who, tok), 18,
                                            INK, "bold", anchor="middle")])
    if out_only:
        tok, first, second = out_only
        x = entry_x(tok) + EW + 8
        out.append(line(x - 3, TOKEN_Y - 2, x - 3, TOKEN_Y + TOKEN_H + 3, INK, 2.4))
        out.append(text(x + 1, TOKEN_Y + 9, first, 16, INK, "bold", "start"))
        out.append(text(x + 1, TOKEN_Y + 26, second, 16, INK, "bold", "start"))
    return out


def counters(state, before):
    out = []
    for name, x, w in (("clock", 2, 150), ("n_sessions", 160, 206)):
        changed = state[name] != before[name]
        value = f"{before[name]} → {state[name]}" if changed else str(state[name])
        fill, stroke = KINDS["new"] if changed else (WHITE, LINE)
        out.append(rect(x, TOKEN_Y, w, TOKEN_H, fill, stroke, rx=3, width=1.8 if changed else 1.1))
        out.append(mono(x + 7, TOKEN_Y + 19, name, 15, MUTED))
        out += tagged(f"cache/{name}", [mono(x + w - 7, TOKEN_Y + 19, value, 16,
                                             ORANGE if changed else INK, "bold", anchor="end")])
    return out


def table(state, before, reads=(), noted=()):
    """session[], and in each entry its block table."""
    out, edges = [], []
    x = 2
    for name, w in COLUMNS:
        out.append(text(x + w / 2, HEAD_Y, name, 15, MUTED, "bold") if name == "会话"
                   else mono(x + w / 2, HEAD_Y, name, 15, MUTED, "bold", anchor="middle"))
        x += w
    for tok in range(ENTRIES):
        out.append(mono(entry_x(tok) + EW / 2, HEAD_Y, f"block[{tok}]", 14, INK, "bold",
                        anchor="middle"))
    tail = entry_x(ENTRIES)
    out.append(mono(tail + (W - 1 - tail) / 2, HEAD_Y, "[8..511]", 14, MUTED, anchor="middle"))
    for index, who in enumerate("AB"):
        y = TABLE_Y + index * TABLE_H
        base = y + 21
        entry, old = state[who], before[who]
        if not entry:
            out.append(rect(2, y, W - 4, TABLE_H, WHITE, LINE, rx=2, width=1.1, dash="5 4"))
            out.append(mono(8, base, f"[{index}]", 15, MUTED))
            out += tagged(f"unused/{who}", [text(W / 2, base, "未使用", 15, MUTED)])
            continue
        out.append(rect(2, y, EX - 8, TABLE_H, WHITE, MUTED, rx=2, width=1.3))
        out.append(mono(8, base, f"[{index}] {who}", 16, OWNER[who], "bold"))
        x = 2 + COLUMNS[0][1]
        for name, w in COLUMNS[1:]:
            now, was = entry[name], old[name] if old else None
            changed = now != was
            value = f"{was} → {now}" if changed and old else str(now)
            read = (who, name) in reads
            if changed or read:
                fill, stroke = KINDS["new" if changed else "reuse"]
                edges.append(rect(x + 1.5, y + 2, w - 3, TABLE_H - 4, fill, stroke, rx=2, width=2))
            if (who, name) in noted:
                edges.append(rect(x + 1.5, y + 2, w - 3, TABLE_H - 4, "none", INK, rx=2, width=2.6))
            edges += tagged(f"field/{who}/{name}",
                            [mono(x + w / 2, base, value, 17, ORANGE if changed else INK, "bold",
                                  anchor="middle")])
            x += w
        for tok in range(ENTRIES):
            now, was = entry["block"][tok], old["block"][tok] if old else None
            x = entry_x(tok)
            changed = now != was
            kind = ("new" if changed else "reuse" if (who, tok) in reads
                    else status(state, None if now is None else (who, tok)))
            out.append(box_of(x, y + 1.5, EW, TABLE_H - 3, kind, 1.9 if changed else 1.2))
            if changed:
                shapes = [mono(x + 4, base, "NULL→", 13, MUTED),
                          text(x + EW - 3, base, f"块 {now}", 16, ORANGE, "bold", "end")]
            elif now is None:
                shapes = [mono(x + EW / 2, base, "NULL", 14, MUTED, anchor="middle")]
            else:
                shapes = [text(x + EW / 2, base, f"块 {now}", 17, INK, "bold")]
            out += tagged(f"entry/{who}/{tok}", shapes)
            if (who, tok) in noted:
                edges.append(rect(x, y + 1, EW, TABLE_H - 2, "none", INK, rx=2, width=2.6))
        out.append(box_of(tail, y + 1.5, W - 2 - tail, TABLE_H - 3, "free", 1.2))
        out.append(mono(tail + (W - 2 - tail) / 2, base, "NULL", 14, MUTED, anchor="middle"))
    return out + edges


def pool_head():
    out = [text(2, POOL_TITLE_Y, f"内存池：{N_BLOCKS} 块，每块 {BLOCK_BYTES} 字节", 17, INK, "bold",
                "start")]
    keys = [("cache", "有效"), ("reserved", "预留"), ("free", "空闲"), ("new", "写入或改变"),
            ("reuse", "读取"), ("addr", "本步定位")]
    left = W - 2 - legend_width(keys, 15, 14)
    out.append(rich(250, POOL_TITLE_Y, "`n_cached`：已算完，`capacity`：已分到块", 14, MUTED,
                    room=left - 258))
    return out + legend(left, POOL_TITLE_Y, keys, 15, 14)


def caption_line(number, total, sentence):
    lead = f"第 {number}/{total} 步"
    lead_w, body_w = width_of(lead, 19) + 16, rich_width(sentence, 20)
    if lead_w + body_w > W - 6:
        raise ValueError(f"caption too wide: {sentence}")
    x = (W - lead_w - body_w) / 2
    return [text(x, CAPTION_Y, lead, 19, ORANGE, "bold", "start"),
            rich(x + lead_w, CAPTION_Y, sentence, 20, INK, "bold")]


def animate(start, steps, requests):
    """The shapes of every frame, with the state after its step and the step."""
    before = copy_state(start)
    for number, step in enumerate(steps, 1):
        state = advance(before, step)
        who = step.get("who", "A")
        out = rail(step["stage"]) + panel(step, state)
        out += token_row(step, state, who, requests[who]) + counters(state, before)
        out += table(state, before, step.get("reads", ()), step.get("noted", ()))
        out += pool_head()
        missing = [state[name]["block"][tok] for name, tok in step.get("missing", ())]
        focus = [state[name]["block"][tok] for name, tok in step.get("focus", ())]
        out += FRAME_POOL.draw(state, before, step.get("marks"), step.get("written", ()), missing,
                               focus)
        out += caption_line(number, len(steps), step["say"])
        yield out, state, step
        before = state


def rows(mode, who, toks, layers=range(N_LAYER)):
    """The marks of the key and the value row of some tokens in some layers."""
    return {(who, tok, layer, kind): mode for tok in toks for layer in layers for kind in (0, 1)}


def entries(who, toks):
    return tuple((who, tok) for tok in toks)
