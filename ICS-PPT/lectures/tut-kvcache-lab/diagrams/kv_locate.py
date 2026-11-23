#!/usr/bin/env python3
"""Where one row lies in the memory of the small example, in two steps.

The moment of kv-layout, layer 1, V, token 2: (1) the block of token 2
starts 2 blocks after base, 2 * 160 = 320 bytes, and kv_block_bytes gives
the 160; (2) inside the block the row starts after 30 floats: 20 for the
layer before, 10 for the key of this layer, 30 * 4 = 120 bytes. The address
is base + 440. Only the numbers of the example are drawn.
Run it to refresh ../assets/kv-locate.svg.
"""

from kv_example_kit import *

W, H = 1048, 450
TARGET = (1, 1, 2)                      # layer, K or V, token
PX, PP, PY, PH = 176, 78, 98, 46        # the memory, one box a block
ZX, ZW, ZY, ZH = 300, 52, 262, 35       # the block


def question():
    layer, kind, tok = TARGET
    out = [text(2, 24, "要找的一行", 18, INK, "bold", "start")]
    x = 108
    for label in (f"第 {layer} 层", "V（值）", f"token {tok}"):
        w = width_of(label, 17) + 24
        out.append(rect(x, 2, w, 31, WHITE, MUTED, rx=4, width=1.6))
        out.append(text(x + 12, 24, label, 17, INK, "bold", "start"))
        x += w + 9
    out.append(arrow(x, 17, x + 34, 17, INK, 2))
    shapes, _, x1 = fn(x + 42, 17, f"kv_value(cache, {layer}, {tok})", 17)
    out += shapes
    out.append(rich(x1 + 10, 24, f"返回 $v_{tok}$ 的地址", 17, INK, "bold", room=W - x1 - 10))
    return out


def block_step(state):
    _, _, tok = TARGET
    before = tok * BLOCK_BYTES
    out = [text(2, 70, "① 找到块", 19, INK, "bold", "start"),
           text(PX - 10, PY + 20, "KV Cache 的内存", 17, INK, "bold", "end"),
           text(PX - 10, PY + 40, f"{N_BLOCKS} 块，依次相接", 15, MUTED, anchor="end"),
           text(PX - 10, PY + PH + 21, "距 base 的字节", 14, MUTED, anchor="end")]
    for b in range(N_BLOCKS):
        x, kind = PX + b * PP, status(state, b)
        hot = b == tok
        out.append(box_of(x, PY, PP - 4, PH, kind, 1.3))
        if hot:                         # the block found: a dark outline, as in kv-layout
            out.append(rect(x, PY, PP - 4, PH, "none", INK, rx=2, width=2.8))
        out.append(text(x + (PP - 4) / 2, PY + 19, f"块 {b}", 16, INK if kind != "unused" else MUTED,
                        "bold"))
        label = {"cache": token_name(b), "reserved": f"{token_name(b)} 预留"}.get(kind, "未使用")
        out += tagged(f"own/{b}", [rich(x + (PP - 4) / 2, PY + 38, label, 14,
                                        MUTED if kind == "unused" else INK, anchor="middle")])
        out += tagged(f"start/{b}", [mono(x + (PP - 4) / 2, PY + PH + 21, f"+{b * BLOCK_BYTES}", 15,
                                          INK if hot else MUTED, "bold" if hot else "normal",
                                          anchor="middle")])
    # from base to the start of the block, measured above the boxes
    x1, y = PX + tok * PP, PY - 14
    out.append(mono(PX - 6, y + 5, "base", 16, INK, "bold", anchor="end"))
    out.append(arrow(PX + 2, y, x1 - 2, y, INK, 2.4, both=True))
    out.append(line(PX, y - 8, PX, PY, INK, 1.6))
    out.append(line(x1, y - 8, x1, PY, INK, 1.6))
    out += tagged("blocks_before", [text(x1 + 8, y + 5, f"{tok} 块 × {BLOCK_BYTES} 字节 = {before} 字节",
                                         17, INK, "bold", "start")])
    # the length of one block
    bx = PX + N_BLOCKS * PP + 22
    shapes, _, _ = fn(bx, PY + 2, "kv_block_bytes(cache)", 16)
    out += shapes
    out.append(rich(bx + 2, PY + 34, "一块的长度：2 层 × 2 行", 17))
    out.append(rich(bx + 2, PY + 58, "× 10 个 float × 4 字节", 17))
    out += tagged("block_bytes", [text(bx + 2, PY + 84, f"= {BLOCK_BYTES} 字节", 19, INK, "bold",
                                       "start")])
    return out


def offset_step():
    layer, kind, tok = TARGET
    x0, x1 = ZX, ZX + KV_DIM * ZW
    left = PX + tok * PP
    out = [f'<path d="M {left} {PY + PH + 1} L {left + PP - 4} {PY + PH + 1} L {x1} {ZY} '
           f'L {x0} {ZY} z" fill="{FILL_BLUE}" stroke="{LINE}" stroke-width="1" opacity="0.6"/>',
           text(2, 230, "② 加块内偏移", 19, INK, "bold", "start")]
    for row, (lay, knd) in enumerate(SEGMENTS):
        y, hot = ZY + row * ZH, (lay, knd) == (layer, kind)
        out.append(mono(x0 - 8, y + 23, f"+{row_bytes(lay, knd)}", 16, INK if hot else BLUE, "bold",
                        anchor="end"))
        out.append(rich(x0 - 58, y + 23, row_name(knd, tok), 18, anchor="end"))
        out.append(text(x0 - 88, y + 23, f"第 {lay} 层 {KV[knd].upper()}", 16, INK, "bold", "end"))
        for i in range(KV_DIM):
            out.append(box_of(x0 + i * ZW, y, ZW, ZH, "cache", 0.9, 0))
            out += tagged(f"float/{offset(lay, knd) + i}",
                          [mono(x0 + (i + 0.5) * ZW, y + 23, str(offset(lay, knd) + i), 16, INK,
                                "bold" if hot else "normal", anchor="middle")])
    out.append(rect(x0, ZY, x1 - x0, 4 * ZH, "none", INK, rx=0, width=2.2))
    at = offset(layer, kind)
    out.append(rect(x0, ZY + (2 * layer + kind) * ZH, x1 - x0, ZH, "none", INK, rx=0, width=3.2))
    # the two parts of the offset, as distances from the top of the block
    ax, before = 112, offset(layer, 0)
    ys = ZY, ZY + 2 * layer * ZH, ZY + (2 * layer + kind) * ZH
    for y in ys:
        out.append(line(ax - 8, y, x0 - 168, y, MUTED, 1, "3 3"))
    out.append(arrow(ax, ys[0] + 1, ax, ys[1] - 1, INK, 2.4))
    out.append(arrow(ax, ys[1] + 1, ax, ys[2] - 1, INK, 2.4))
    out.append(text(ax - 12, ys[0] + 24, "前 1 层", 16, INK, "bold", "end"))
    out += tagged("offset/layers", [text(ax - 12, ys[0] + 46, f"{before} 个 float", 15, INK, "bold",
                                         "end")])
    out.append(text(ax - 12, ys[1] + 16, "键", 16, INK, "bold", "end"))
    out += tagged("offset/keys", [text(ax - 12, ys[1] + 34, f"{at - before} 个 float", 15, INK, "bold",
                                       "end")])
    # the offset in floats, then in bytes
    tx = x1 + 30
    out.append(text(tx, ZY + 30, "块内偏移：", 17, INK, "bold", "start"))
    out.append(text(tx, ZY + 60, f"{before} + {at - before} = {at} 个 float", 17, INK, "normal", "start"))
    out += tagged("offset/bytes", [text(tx, ZY + 92, f"= {at} × 4 = {row_bytes(layer, kind)} 字节", 19,
                                        INK, "bold", "start")])
    out.append(text(tx, ZY + 124, "数字是块内第几个 float", 15, MUTED, "normal", "start"))
    return out


def result():
    layer, kind, tok = TARGET
    total = address(layer, kind, tok)
    return tagged("result", [rich(W / 2, H - 12,
                                  f"地址 = base + {tok * BLOCK_BYTES}（前 {tok} 块）"
                                  f"+ {row_bytes(layer, kind)}（块内 {offset(layer, kind)} 个 float）"
                                  f"= base + {total}，以字节计",
                                  20, INK, "bold", anchor="middle")])


def figure():
    state = snapshot()
    funnel, *zoomed = offset_step()         # under the memory, so that its labels stay on top
    return question() + [funnel] + block_step(state) + zoomed + result()


if __name__ == "__main__":
    save("kv-locate", W, H, figure())
