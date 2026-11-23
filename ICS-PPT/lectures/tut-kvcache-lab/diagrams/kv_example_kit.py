"""The small example of the second part, and its drawing.

kv_layout.py, kv_locate.py and kv_first.py draw one example: 2 layers, rows
of 10 floats (one key-value head, d = 10), a block of 40 floats or 160
bytes, a memory of 8 blocks, at most 2 answer tokens a turn. The first turn
of the conversation has the message t0 t1 t2.

The memory of the cache is one piece: block tok starts tok * 160 bytes after
base and holds token tok. A block is one column, its four rows from the top
the key and the value of layer 0, then of layer 1, 0, 40, 80 and 120 bytes
into the block (0, 10, 20 and 30 floats). An address is written in bytes
from base, as base+440. Block numbers always carry 块 and token numbers t.

A frame of the animation has fixed areas, from top to bottom: the steps of
serve; who calls what with which arguments, what is computed and returned;
the setting of the turn and the tokens, each above its block; struct kv_cache on the
left and the memory on the right; one sentence. A step states only what
changes, and a change is drawn from the difference to the frame before.
"""

from kv_kit import *

N_LAYER, KV_DIM, FLOAT_BYTES = 2, 10, 4
N_BLOCKS = 8
SEGMENTS = [(layer, kind) for layer in range(N_LAYER) for kind in (0, 1)]
BLOCK_FLOATS = len(SEGMENTS) * KV_DIM
BLOCK_BYTES = BLOCK_FLOATS * FLOAT_BYTES


def offset(layer, kind):
    """The float at which a row starts in its block."""
    return (2 * layer + kind) * KV_DIM


def row_bytes(layer, kind):
    """The byte at which a row starts in its block."""
    return offset(layer, kind) * FLOAT_BYTES


def address(layer, kind, tok):
    """Bytes from base to a row: the start of the block of the token, then the row."""
    return tok * BLOCK_BYTES + row_bytes(layer, kind)


def token_name(tok):
    return f"$t_{tok}$"


def row_name(kind, tok):
    return f"${KV[kind]}_{tok}$"


# ------------------------------------------------------------ the state

def empty_state():
    return {"n_cached": 0, "capacity": 0}


def snapshot():
    """The moment of kv-layout and kv-locate: t2 computed, t3 not yet."""
    return {"n_cached": 3, "capacity": 4}


def advance(state, step):
    """The state after a step: capacity and cached name what it changes."""
    now = dict(state)
    if "capacity" in step:
        now["capacity"] = step["capacity"]
    if "cached" in step:
        now["n_cached"] = step["cached"]
    return now


def status(state, block):
    if block < state["n_cached"]:
        return "cache"
    return "reserved" if block < state["capacity"] else "unused"


# ------------------------------------------------------------ the memory

class Memory:
    """The blocks side by side from base, each a column of one row a segment."""

    PITCH, CW = 82, 78

    def __init__(self, x0, head, row_h):
        self.x0, self.head, self.row_h = x0, head, row_h
        self.start_y = head + 18                # where each block starts, in bytes from base
        self.seg_y = head + 25
        self.band_y = self.seg_y + len(SEGMENTS) * row_h + 2
        self.bottom = self.band_y + 20

    def x(self, block):
        return self.x0 + block * self.PITCH

    def mid(self, block):
        return self.x(block) + self.CW / 2

    def right(self):
        return self.x(N_BLOCKS - 1) + self.CW

    def labels(self):
        edge = self.x0 - 6
        out = [text(edge, self.head, "KV Cache 的内存", 16, INK, "bold", "end"),
               text(edge, self.start_y, "距 base 的字节", 13, MUTED, "normal", "end"),
               text(edge, self.band_y + 15, "token", 15, MUTED, "bold", "end")]
        for row, (layer, kind) in enumerate(SEGMENTS):
            base = self.seg_y + (row + 0.5) * self.row_h + 5.5
            out.append(text(edge - 46, base, f"第 {layer} 层 {KV[kind].upper()}", 16, INK, "bold",
                            "end"))
            out.append(mono(edge, base, f"+{row_bytes(layer, kind)}", 15, BLUE, "bold", anchor="end"))
        return out

    def draw(self, state, before=None, marks=None, written=(), missing=(), focus=()):
        """marks: (block, layer, kind) -> r, w, wr or a; written: (block, layer) written
        earlier in the same compute; missing: blocks of a token that has no key yet."""
        marks, before = marks or {}, before or state
        out, rings = self.labels(), []
        size = 17 if self.row_h >= 22 else 15
        lift = 5.5 if self.row_h >= 22 else 4
        for block in range(N_BLOCKS):
            x, kind_of = self.x(block), status(state, block)
            modes = [marks.get((block,) + key, "") for key in SEGMENTS]
            tone = (ORANGE if "w" in modes or "wr" in modes else GREEN if "r" in modes
                    else INK if kind_of != "unused" or block in focus else MUTED)
            out += tagged(f"head/{block}", [text(x + self.CW / 2, self.head, f"块 {block}", 16, tone,
                                                  "bold")])
            out += tagged(f"start/{block}", [mono(x + self.CW / 2, self.start_y,
                                                   f"+{block * BLOCK_BYTES}", 15, MUTED,
                                                   anchor="middle")])
            for row, (layer, kind) in enumerate(SEGMENTS):
                y, mode = self.seg_y + row * self.row_h, modes[row]
                fresh = (block, layer) in written
                look = ("new" if mode in ("w", "wr") or fresh else "reuse" if mode == "r"
                        else kind_of)
                out.append(box_of(x, y, self.CW, self.row_h, look, 1.1, 0))
                word = {"r": "读", "w": "写", "wr": "写读", "a": "地址"}.get(mode, "")
                named = kind_of == "cache" or look == "new"
                base = y + self.row_h / 2 + lift
                shapes = []
                if named:
                    shapes.append(rich(x + self.CW / 2 - (17 if word else 0), base,
                                       row_name(kind, block), size, anchor="middle"))
                if word:
                    colour = {"r": GREEN, "a": INK}.get(mode, ORANGE)
                    shapes.append(text(x + self.CW / 2 + (17 if named else 0), base, word, 15,
                                       colour, "bold"))
                    rings.append(rect(x + 1.2, y + 1.2, self.CW - 2.4, self.row_h - 2.4, "none",
                                      colour, rx=1, width=2.3))
                out += tagged(f"row/{block}/{row_bytes(layer, kind)}", shapes)
            if block in missing:
                mid = self.seg_y + 2 * self.row_h
                out.append(text(x + self.CW / 2, mid - 5, "还没有", 16, INK, "bold"))
                out.append(rich(x + self.CW / 2, mid + 17, f"$k_{block}$、$v_{block}$", 16, INK,
                                "bold", anchor="middle"))
            edge = INK if block in focus or block in missing else MUTED
            out.append(rect(x, self.seg_y, self.CW, len(SEGMENTS) * self.row_h, "none", edge, rx=1,
                            width=2.8 if edge == INK else 1.6))
            changed = status(before, block) != kind_of
            if kind_of == "unused":
                shapes = [text(x + self.CW / 2, self.band_y + 15, "未使用", 15, MUTED)]
            else:
                cached = kind_of == "cache"
                out.append(rect(x, self.band_y, self.CW, 20, BLUE if cached else WHITE,
                                ORANGE if changed else BLUE, rx=3, width=2.6 if changed else 1.5))
                label = token_name(block) + ("" if cached else " 预留")
                shapes = [rich(x + self.CW / 2, self.band_y + 15, label, 15,
                               WHITE if cached else BLUE, "bold", anchor="middle")]
            out += tagged(f"own/{block}", shapes)
        return out + rings


# ------------------------------------------------------------ struct kv_cache

CARD_ROWS = [("n_layer", "层数"), ("kv_dim", "一行的 float 数"), ("base", "块 0 起点的地址"),
             ("n_blocks", "内存中的块数"), ("n_cached", "已缓存的 token 数"),
             ("capacity", "已预留的块数")]


def card(x, y, w, state, before=None, reads=(), row_h=27):
    """struct kv_cache: one row a field, with what it holds and its value now."""
    before = before or state
    values = {"n_layer": N_LAYER, "kv_dim": KV_DIM, "n_blocks": N_BLOCKS}
    top = y + 30
    out = [rect(x, y, w, 30 + len(CARD_ROWS) * row_h + 6, WHITE, MUTED, rx=6, width=1.4),
           mono(x + 10, y + 21, "struct kv_cache", 16, INK, "bold")]
    for i, (name, meaning) in enumerate(CARD_ROWS):
        ry = top + i * row_h
        base = ry + row_h / 2 + 6
        now, was = state.get(name, values.get(name)), before.get(name, values.get(name))
        changed, read = now != was, name in reads
        if changed or read:
            fill, stroke = KINDS["new" if changed else "reuse"]
            out.append(rect(x + 4, ry + 1, w - 8, row_h - 2, fill, stroke, rx=3, width=2))
        out.append(mono(x + 10, base, name, 16, INK, "bold"))
        out.append(text(x + 100, base, meaning, 14, MUTED, "normal", "start"))
        if now is not None:
            value = f"{was} → {now}" if changed else str(now)
            out += tagged(f"field/{name}", [mono(x + w - 10, base, value, 17,
                                                 ORANGE if changed else INK, "bold", anchor="end")])
    return out


def states_legend(x, y, size=16, gap=30):
    """The three kinds of block and the fields that divide them."""
    out = []
    for i, (kind, name, meaning) in enumerate((
            ("cache", "已缓存", "块 0 到 `n_cached` − 1"),
            ("reserved", "预留", "块 `n_cached` 到 `capacity` − 1"),
            ("unused", "未使用", "从块 `capacity` 起"))):
        top = y + i * gap
        out.append(box_of(x, top, 30, 21, kind, 1.5, 3))
        out.append(text(x + 40, top + 17, name, 17, INK, "bold", "start"))
        out.append(rich(x + 98, top + 17, meaning, size))
    return out


# ------------------------------------------------------------ a frame

W, H = 1120, 444
PANEL_Y, PANEL_H = 35, 94
ROWS = (52, 82, 112)
VX = 70                                 # the values of the panel
TOKEN_Y, TOKEN_H = 156, 27
FRAME_MEMORY = Memory(466, 214, 26)     # its last block ends at W - 2
CARD_X, CARD_Y, CARD_W = 2, 192, 304
LEGEND_Y = 394
CAPTION_Y = 434
STAGES = [("fw", "append_message"), ("fn", "kv_reserve"), ("given", "compute"),
          ("fw", "next_token"), ("end", "本轮结束")]


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


def address_row(row, spec, kind):
    """The addresses that kv_key or kv_value returns, each above the block it points into."""
    layer, toks = spec
    at = row_bytes(layer, kind)
    out, _, x1 = fn(VX, row, ("kv_key", "kv_value")[kind], 16, h=25)
    out.append(mono(x1 + 6, row + 5, f"base + {BLOCK_BYTES} × tok + {at}", 15, INK))
    edge = FRAME_MEMORY.x0 - 6
    out.append(mono(edge, row + 5, ("keys", "values")[kind], 16, INK, "bold", anchor="end"))
    for tok in toks:
        x = FRAME_MEMORY.x(tok)
        last = tok == toks[-1]
        out.append(box_of(x, row - 12.5, FRAME_MEMORY.CW, 25, "addr", 2.2 if last else 1.2))
        out += tagged(f"addr/{('keys', 'values')[kind]}/{tok}",
                      [text(x + FRAME_MEMORY.CW / 2, row + 5.5, f"base+{address(layer, kind, tok)}",
                            16, INK, "bold" if last else "normal")])
    return out


def panel(step):
    """Who calls what with which arguments; what is computed; what comes back."""
    out = [rect(1, PANEL_Y, W - 2, PANEL_H, WHITE, MUTED, rx=7, width=1.3),
           line(VX - 10, PANEL_Y + 6, VX - 10, PANEL_Y + PANEL_H - 6, LINE, 1)]
    addresses = "addresses" in step
    labels = step.get("labels", ("调用", "返回", "") if addresses else ("调用", "计算", "返回"))
    for label, row in zip(labels, ROWS):
        if label:
            out.append(text(12, row + 6, label, 17, MUTED, "bold", "start"))
    out += call_line(VX, ROWS[0], step["call"], W - 4, 17)
    if addresses:
        for kind in (0, 1):
            out += address_row(ROWS[1 + kind], step["addresses"], kind)
        return out
    for row, value, weight in zip(ROWS[1:], (step.get("calc", ""), step.get("ret", "")),
                                  ("bold", "normal")):
        if isinstance(value, list):
            out += call_line(VX, row, value, W - 4, 17)
        elif value:
            out.append(rich(VX, row + 6, value, 18, INK, weight, room=W - 6 - VX))
    return out


def token_row(step, state, setting):
    """The text of the conversation: each token above its block."""
    out = [rich(2, TOKEN_Y + 19, setting, 15, INK, "bold", room=FRAME_MEMORY.x0 - 12)]
    n_text, n_shown = step["text"], step.get("shown", step["text"])
    cw = FRAME_MEMORY.CW
    for first, count, name in step.get("groups", ()):
        x0, x1 = FRAME_MEMORY.x(first) + 3, FRAME_MEMORY.x(first + count - 1) + cw - 3
        half = rich_width(name, 15) / 2 + 5
        mid = (x0 + x1) / 2
        out.append(path(f"M {x0} 153 L {x0} 147 L {max(x0, mid - half):.1f} 147", MUTED, 1.4,
                        head=False))
        out.append(path(f"M {x1} 153 L {x1} 147 L {min(x1, mid + half):.1f} 147", MUTED, 1.4,
                        head=False))
        out.append(rich(mid, 151, name, 15, INK, "bold", anchor="middle"))
    out_only = step.get("output")
    for tok in range(n_shown):
        x, fill, stroke, dash, width = FRAME_MEMORY.x(tok), WHITE, MUTED, None, 1.3
        if tok in step.get("current", ()):
            (fill, stroke), width = KINDS["new"], 2.6
        elif tok < state["n_cached"]:
            fill, stroke = KINDS["cache"]
        if tok in step.get("fresh", ()):
            stroke, width = ORANGE, 2.6
        if tok >= n_text:
            dash = "5 4"
        if out_only and tok == out_only[0]:
            fill, stroke, width, dash = FILL_GREY, INK, 2.2, "5 4"
        out.append(rect(x, TOKEN_Y, cw, TOKEN_H, fill, stroke, rx=3, width=width, dash=dash))
        out += tagged(f"token/{tok}", [rich(x + cw / 2, TOKEN_Y + 19, token_name(tok), 18, INK,
                                            "bold", anchor="middle")])
    if out_only:
        tok, label = out_only
        x = FRAME_MEMORY.x(tok) + cw + 8
        out.append(line(x - 3, TOKEN_Y - 2, x - 3, TOKEN_Y + TOKEN_H + 3, INK, 2.4))
        out.append(text(x + 1, TOKEN_Y + 19, label, 16, INK, "bold", "start"))
    return out


def memory_legend():
    keys = [("cache", "已缓存"), ("reserved", "预留"), ("unused", "未使用"), ("new", "写入或改变"),
            ("reuse", "读取"), ("addr", "本步定位")]
    left = W - 2 - legend_width(keys, 15, 14)
    out = [text(CARD_X + CARD_W + 26, LEGEND_Y + 1, f"{N_BLOCKS} 块，每块 {BLOCK_BYTES} 字节", 16, INK,
                "bold", "start")]
    if CARD_X + CARD_W + 26 + width_of(f"{N_BLOCKS} 块，每块 {BLOCK_BYTES} 字节", 16) + 12 > left:
        raise ValueError("legend too wide")
    return out + legend(left, LEGEND_Y, keys, 15, 14)


def caption_line(number, total, sentence):
    lead = f"第 {number}/{total} 步"
    lead_w, body_w = width_of(lead, 19) + 16, rich_width(sentence, 20)
    if lead_w + body_w > W - 6:
        raise ValueError(f"caption too wide: {sentence}")
    x = (W - lead_w - body_w) / 2
    return [text(x, CAPTION_Y, lead, 19, ORANGE, "bold", "start"),
            rich(x + lead_w, CAPTION_Y, sentence, 20, INK, "bold")]


def animate(start, steps, setting):
    """The shapes of every frame, with the state after its step and the step."""
    before = dict(start)
    for number, step in enumerate(steps, 1):
        state = advance(before, step)
        out = rail(step["stage"]) + panel(step) + token_row(step, state, setting)
        out += card(CARD_X, CARD_Y, CARD_W, state, before, step.get("reads", ()))
        out += FRAME_MEMORY.draw(state, before, step.get("marks"), step.get("written", ()),
                                 step.get("missing", ()), step.get("focus", ()))
        out += memory_legend() + caption_line(number, len(steps), step["say"])
        yield out, state, step
        before = state


def rows(mode, toks, layers=range(N_LAYER)):
    """The marks of the key and the value row of some tokens in some layers."""
    return {(tok, layer, kind): mode for tok in toks for layer in layers for kind in (0, 1)}
