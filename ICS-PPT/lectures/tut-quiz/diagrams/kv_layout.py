#!/usr/bin/env python3
"""The structure of the KV cache in memory, on the small example.

The moment is turn 2 of session A, t4 computed and t5 not yet. Three levels:
the session table with one entry a session, the block table inside an entry,
and the pool of equal blocks with one bit a block. The blocks of A are not
next to each other, a block holds floats only, and all three states show:
valid, reserved, free. One block is drawn larger, its four rows one below
the other as in the pool. No function appears here.
Run it to refresh ../assets/kv-layout.svg.
"""

from kv_example_kit import *
from svgkit import _marker

W, H = 1120, 504
FIELDS = [("id", 56), ("n_cached", 100), ("capacity", 100), ("last_use", 92)]
CARD_Y, CARD_H = 26, 136
CARDS = {"A": (2, 748, 7), "B": (758, 1118, 2)}    # left, right, entries before the NULL ones
FIELD_Y, FIELD_H, ENTRY_Y, ENTRY_H, EP = 54, 46, 110, 44, 88
POOL = Pool(215, 18.5)
ZOOM_BLOCK, ZX, ZW, ZY, ZH = 6, 434, 45, 388, 21


def card(state, index, who):
    x0, x1, shown = CARDS[who]
    out = [rect(x0, CARD_Y, x1 - x0, CARD_H, WHITE, OWNER[who], rx=6, width=2.2),
           mono(x0 + 10, CARD_Y + 21, f"session[{index}]", 16, INK, "bold"),
           text(x0 + 128, CARD_Y + 21, f"会话 {who}", 16, OWNER[who], "bold", "start")]
    entry, x = state[who], x0 + 8
    for name, w in FIELDS:
        out.append(rect(x, FIELD_Y, w - 4, FIELD_H, WHITE, MUTED, rx=3, width=1.2))
        out.append(mono(x + (w - 4) / 2, FIELD_Y + 18, name, 14, MUTED, "bold", anchor="middle"))
        out += tagged(f"field/{who}/{name}", [mono(x + (w - 4) / 2, FIELD_Y + 39, str(entry[name]),
                                                   19, INK, "bold", anchor="middle")])
        x += w
    if who == "A":
        out.append(rich(x + 8, FIELD_Y - 4, "`last_use`：越大表示越近使用", 16))
        out.append(rich(x + 8, FIELD_Y + 19, "`n_cached`：前 5 项已算完，是有效的", 16))
        out.append(rich(x + 8, FIELD_Y + 42, "`capacity`：前 7 项已分到块", 16))
    tops = {}
    for tok in range(shown + 1):
        x = x0 + 8 + tok * EP
        w = x1 - 8 - x if tok == shown else EP - 4
        block = entry["block"][tok]
        kind = status(state, None if block is None else (who, tok))
        out.append(box_of(x, ENTRY_Y, w, ENTRY_H, kind, 1.5))
        label = f"block[{tok}]" if block is not None else f"[{tok}..511]"
        out.append(mono(x + w / 2, ENTRY_Y + 17, label, 14, MUTED, "bold", anchor="middle"))
        if block is None:
            shapes = [mono(x + w / 2, ENTRY_Y + 38, "NULL", 15, MUTED, anchor="middle")]
        else:
            shapes = [text(x + w / 2, ENTRY_Y + 38, f"块 {block}", 18, INK, "bold")]
            tops[block] = (who, x + w / 2)
        out += tagged(f"entry/{who}/{tok}", shapes)
    return out, tops


def pointers(tops):
    out = []
    for block, (who, x) in tops.items():
        out.append(arrow(x, ENTRY_Y + ENTRY_H, POOL.mid(block), POOL.head - 18, OWNER[who], 1.9))
    return out


def zoom(state):
    """One block: four rows of kv_dim floats, the order of memory left to right, then down."""
    who, tok = owners(state)[ZOOM_BLOCK]
    x0, x1 = ZX, ZX + KV_DIM * ZW
    left, right = POOL.x(ZOOM_BLOCK), POOL.x(ZOOM_BLOCK) + POOL.CW
    out = [f'<path d="M {left} {POOL.bottom + 1} L {right} {POOL.bottom + 1} L {x1} {ZY} '
           f'L {x0} {ZY} z" fill="{FILL_BLUE}" stroke="{LINE}" stroke-width="1" opacity="0.6"/>']
    for row, (layer, kind) in enumerate(SEGMENTS):
        y = ZY + row * ZH
        out.append(text(x0 - 70, y + 16, f"第 {layer} 层 {KV[kind].upper()}", 15, INK, "bold", "end"))
        out.append(mono(x0 - 30, y + 16, f"+{offset(layer, kind)}", 15, BLUE, "bold", anchor="end"))
        out.append(rich(x0 - 6, y + 16, row_name(who, kind, tok), 17, anchor="end"))
        for i in range(KV_DIM):
            x = x0 + i * ZW
            out.append(box_of(x, y, ZW, ZH, "cache", 0.9, 0))
            out += tagged(f"float/{offset(layer, kind) + i}",
                          [mono(x + ZW / 2, y + 15.5, str(offset(layer, kind) + i), 14, INK,
                                anchor="middle")])
    out.append(rect(x0, ZY, x1 - x0, 4 * ZH, "none", INK, rx=0, width=2.2))
    out += vbrace(x1 + 6, ZY + 2, ZY + 4 * ZH - 2, INK, "", 15)
    tx = x1 + 22
    out.append(text(tx, ZY + 24, "块 6 放大：40 个 float", 16, INK, "bold", "start"))
    out.append(text(tx, ZY + 48, "= 160 字节", 16, INK, "bold", "start"))
    out.append(text(tx, ZY + 72, "块内只有 float", 16, INK, "bold", "start"))
    out.append(rich((x0 + x1) / 2, ZY + 4 * ZH + 24,
                    "一行 = `kv_dim` = 10 个 float，4 行依次相接，数字是块内第几个 float", 16, INK, "bold",
                    anchor="middle"))
    return out


def states_legend():
    out = []
    for i, (kind, name, meaning) in enumerate((("cache", "有效", "已算完，位是 1"),
                                               ("reserved", "预留", "已分到，位是 1"),
                                               ("free", "空闲", "未分配，位是 0"))):
        y = 388 + i * 30
        out.append(box_of(4, y, 30, 21, kind, 1.5, 3))
        out.append(text(44, y + 17, name, 17, INK, "bold", "start"))
        out.append(text(86, y + 17, meaning, 16, INK, anchor="start"))
    out.append(text(4, 496, "位图：每块 1 位", 16, INK, "bold", "start"))
    return out


def figure():
    state = snapshot()
    out = [text(2, 18, "会话表", 18, INK, "bold", "start"),
           mono(62, 18, "kv_cache.session[]", 17, INK, "bold"),
           text(270, 18, "每个会话一项，各有一张块表：每个 token 一项，指向一块", 17, INK,
                anchor="start"),
           rich(W - 4, 18, "此刻：A 算完了 $t_0$ 到 $t_4$，$t_5$ 还未算", 16, ORANGE, "bold", "end")]
    tops = {}
    for index, who in enumerate("AB"):
        shapes, found = card(state, index, who)
        out += shapes
        tops.update(found)
    out += pointers(tops)
    out.append(text(2, POOL.head - 25, "内存池", 18, INK, "bold", "start"))
    out.append(text(2, POOL.head - 5, f"{N_BLOCKS} 块，大小相等", 15, MUTED, anchor="start"))
    out += POOL.draw(state, focus=(ZOOM_BLOCK,))
    return out + zoom(state) + states_legend()


if __name__ == "__main__":
    save("kv-layout", W, H, figure(), [_marker(PURPLE)])
