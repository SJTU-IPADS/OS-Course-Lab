#!/usr/bin/env python3
"""Where one row lies in the pool of the small example, in two steps.

Session A at the moment of kv-layout, layer 1, V, token 4: (1) the block
table gives the block of token 4, block 6 of the pool, so the two numbers
differ; (2) the offset inside the block is 30 floats: 20 for the layer
before, 10 for the keys of this layer. kv_block_bytes is the length of the
block, 160 bytes. Only the numbers of the example are drawn.
Run it to refresh ../assets/kv-locate.svg.
"""

from kv_example_kit import *

W, H = 1120, 494
TARGET = ("A", 1, 1, 4)                 # session, layer, K or V, token
EX, EP, EY, EH = 176, 100, 70, 44       # the block table of A
PX, PP, PY, PH = 176, 78, 164, 40       # the pool, one box a block
ZX, ZW, ZY, ZH = 260, 52, 300, 35       # the block


def question():
    who, layer, kind, tok = TARGET
    out = [text(2, 24, "要找的一行", 18, INK, "bold", "start")]
    x = 108
    for label, hot in ((f"会话 {who}", False), (f"第 {layer} 层", False), ("V（值）", False),
                       (f"token {tok}", True)):
        fill, stroke = KINDS["new" if hot else "cache"]
        w = width_of(label, 17) + 24
        out.append(rect(x, 2, w, 31, fill, stroke, rx=4, width=1.8))
        out.append(text(x + 12, 24, label, 17, INK, "bold", "start"))
        x += w + 9
    out.append(arrow(x, 17, x + 34, 17, INK, 2))
    shapes, _, x1 = fn(x + 42, 17, f"kv_value(cache, {who}, {layer}, {tok})", 17)
    out += shapes
    out.append(rich(x1 + 10, 24, f"返回 $v_{tok}$ 的地址", 17, INK, "bold", room=W - x1 - 10))
    return out


def table_step(state):
    who, _, _, tok = TARGET
    entry = state[who]
    out = [text(2, 66, "① 查块表", 19, ORANGE, "bold", "start"),
           text(EX - 10, EY + 30, f"会话 {who} 的块表", 16, INK, "bold", "end")]
    for t in range(ENTRIES):
        x, block = EX + t * EP, entry["block"][t]
        w = W - 4 - x if t == ENTRIES - 1 else EP - 4
        kind = "new" if t == tok else status(state, None if block is None else (who, t))
        out.append(box_of(x, EY, w, EH, kind, 2.8 if t == tok else 1.3))
        label = f"block[{t}]" if t < ENTRIES - 1 else f"block[{t}..511]"
        out.append(mono(x + w / 2, EY + 17, label, 14, INK if t == tok else MUTED, "bold",
                        anchor="middle"))
        value = "NULL" if block is None else f"块 {block}"
        out += tagged(f"entry/{who}/{t}", [text(x + w / 2, EY + 38, value, 18,
                                               MUTED if block is None else INK, "bold")])
    block = entry["block"][tok]
    out.append(text(EX - 10, PY + 18, "内存池", 17, INK, "bold", "end"))
    out.append(text(EX - 10, PY + 37, f"{N_BLOCKS} 块", 15, MUTED, anchor="end"))
    now = owners(state)
    for b in range(N_BLOCKS):
        x, owner = PX + b * PP, now.get(b)
        out.append(box_of(x, PY, PP - 4, PH, "new" if b == block else status(state, owner),
                          2.8 if b == block else 1.3))
        out.append(text(x + (PP - 4) / 2, PY + 17, f"块 {b}", 16, INK if owner else MUTED, "bold"))
        label = f"{owner[0]} {token_name(*owner)}" if owner else "空闲"
        label += " 预留" if status(state, owner) == "reserved" else ""
        out += tagged(f"own/{b}", [rich(x + (PP - 4) / 2, PY + 35, label, 14,
                                        OWNER[owner[0]] if owner else MUTED, anchor="middle")])
    x0, x1 = EX + tok * EP + (EP - 4) / 2, PX + block * PP + (PP - 4) / 2
    out.append(arrow(x0, EY + EH + 2, x1, PY - 3, ORANGE, 2.8))
    out.append(text(max(x0, x1) + 26, 146, f"token 编号是 {tok}，块编号是 {block}", 18, ORANGE, "bold",
                    "start"))
    return out


def offset_step(state):
    who, layer, kind, tok = TARGET
    block = state[who]["block"][tok]
    x0, x1 = ZX, ZX + KV_DIM * ZW
    left = PX + block * PP
    out = [f'<path d="M {left} {PY + PH + 1} L {left + PP - 4} {PY + PH + 1} L {x1} {ZY} '
           f'L {x0} {ZY} z" fill="{FILL_ORANGE}" stroke="{LINE}" stroke-width="1" opacity="0.55"/>',
           text(2, 262, "② 加块内偏移", 19, ORANGE, "bold", "start")]
    for row, (lay, knd) in enumerate(SEGMENTS):
        y, hot = ZY + row * ZH, (lay, knd) == (layer, kind)
        out.append(mono(x0 - 8, y + 23, f"+{offset(lay, knd)}", 16, ORANGE if hot else BLUE, "bold",
                        anchor="end"))
        out.append(rich(x0 - 52, y + 23, row_name(who, knd, tok), 18, anchor="end"))
        out.append(text(x0 - 82, y + 23, f"第 {lay} 层 {KV[knd].upper()}", 16, INK,
                        "bold", "end"))
        for i in range(KV_DIM):
            out.append(box_of(x0 + i * ZW, y, ZW, ZH, "new" if hot else "cache", 0.9, 0))
            out += tagged(f"float/{offset(lay, knd) + i}",
                          [mono(x0 + (i + 0.5) * ZW, y + 23, str(offset(lay, knd) + i), 16, INK,
                                "bold" if hot else "normal", anchor="middle")])
    out.append(rect(x0, ZY, x1 - x0, 4 * ZH, "none", INK, rx=0, width=2.2))
    at = offset(layer, kind)
    out.append(rect(x0, ZY + (2 * layer + kind) * ZH, x1 - x0, ZH, "none", ORANGE, rx=0, width=3))
    # the two parts of the offset, as distances from the top of the block
    ax, before = 78, offset(layer, 0)
    ys = ZY, ZY + 2 * layer * ZH, ZY + (2 * layer + kind) * ZH
    for y in ys:
        out.append(line(ax - 8, y, x0 - 172, y, MUTED, 1, "3 3"))
    out.append(arrow(ax, ys[0] + 1, ax, ys[1] - 1, BLUE, 2.4))
    out.append(arrow(ax, ys[1] + 1, ax, ys[2] - 1, ORANGE, 2.4))
    out.append(text(ax - 12, ys[0] + 24, "前 1 层", 16, BLUE, "bold", "end"))
    out += tagged("offset/layers", [mono(ax - 12, ys[0] + 48, f"+{before}", 17, BLUE, "bold",
                                         anchor="end")])
    out.append(text(ax - 12, ys[1] + 16, "键", 16, ORANGE, "bold", "end"))
    out += tagged("offset/keys", [mono(ax - 12, ys[1] + 34, f"+{at - before}", 17, ORANGE, "bold",
                                       anchor="end")])
    # the length of the block
    bx = x1 + 8
    out.append(path(f"M {bx} {ZY + 1} L {bx + 9} {ZY + 1} L {bx + 9} {ZY + 4 * ZH - 1} "
                    f"L {bx} {ZY + 4 * ZH - 1}", INK, 1.8, head=False))
    shapes, _, _ = fn(bx + 22, ZY + 30, "kv_block_bytes(cache)", 16)
    out += shapes
    out.append(rich(bx + 24, ZY + 70, "一块的长度：2 层 × 2 行", 17))
    out.append(rich(bx + 24, ZY + 96, "× 10 个 float × 4 字节", 17))
    out += tagged("block_bytes", [text(bx + 24, ZY + 125, f"= {BLOCK_BYTES} 字节", 19, INK, "bold",
                                       "start")])
    out += tagged("result", [rich(W / 2, H - 14,
                                  f"地址 = `block[{tok}]` + {before}（前 1 层）+ {at - before}（这一层的键）"
                                  f"= `block[{tok}]` + {at}，以 float 计",
                                  20, INK, "bold", anchor="middle")])
    return out


def figure():
    state = snapshot()
    return question() + table_step(state) + offset_step(state)


if __name__ == "__main__":
    save("kv-locate", W, H, figure())
