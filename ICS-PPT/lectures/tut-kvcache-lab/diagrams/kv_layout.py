#!/usr/bin/env python3
"""The structure of the KV cache in memory, on the small example.

The moment is the first turn, t0 t1 t2 computed and t3 not yet. struct
kv_cache on the left; on the right the memory, one piece from base, block
tok at base + tok * 160 bytes holding token tok, its blocks cached,
reserved or not used yet. One block is drawn larger, its four rows one
below the other as in the memory. No function appears here.
Run it to refresh ../assets/kv-layout.svg.
"""

from kv_example_kit import *

W, H = 1120, 384
MEMORY = Memory(466, 58, 24)
ZOOM_BLOCK, ZX, ZW, ZY, ZH = 2, 516, 45, 262, 21


def zoom():
    """One block: four rows of kv_dim floats, the order of memory left to right, then down."""
    x0, x1 = ZX, ZX + KV_DIM * ZW
    left, right = MEMORY.x(ZOOM_BLOCK), MEMORY.x(ZOOM_BLOCK) + MEMORY.CW
    out = [f'<path d="M {left} {MEMORY.bottom + 1} L {right} {MEMORY.bottom + 1} L {x1} {ZY} '
           f'L {x0} {ZY} z" fill="{FILL_BLUE}" stroke="{LINE}" stroke-width="1" opacity="0.6"/>']
    for row, (layer, kind) in enumerate(SEGMENTS):
        y = ZY + row * ZH
        out.append(text(x0 - 76, y + 16, f"第 {layer} 层 {KV[kind].upper()}", 15, INK, "bold", "end"))
        out.append(mono(x0 - 32, y + 16, f"+{row_bytes(layer, kind)}", 15, BLUE, "bold", anchor="end"))
        out.append(rich(x0 - 6, y + 16, row_name(kind, ZOOM_BLOCK), 17, anchor="end"))
        for i in range(KV_DIM):
            x = x0 + i * ZW
            out.append(box_of(x, y, ZW, ZH, "cache", 0.9, 0))
            out += tagged(f"float/{offset(layer, kind) + i}",
                          [mono(x + ZW / 2, y + 15.5, str(offset(layer, kind) + i), 14, INK,
                                anchor="middle")])
    out.append(rect(x0, ZY, x1 - x0, 4 * ZH, "none", INK, rx=0, width=2.2))
    out += vbrace(x1 + 6, ZY + 2, ZY + 4 * ZH - 2, INK, "", 15)
    tx = x1 + 22
    out.append(text(tx, ZY + 16, f"块 {ZOOM_BLOCK} 放大：", 16, INK, "bold", "start"))
    out.append(text(tx, ZY + 40, f"{BLOCK_FLOATS} 个 float", 16, INK, "bold", "start"))
    out += tagged("block_bytes", [text(tx, ZY + 64, f"= {BLOCK_BYTES} 字节", 16, INK, "bold", "start")])
    out.append(text(tx, ZY + 88, "块内只有 float", 16, INK, "bold", "start"))
    out.append(rich((x0 + x1) / 2, ZY + 4 * ZH + 26,
                    "一行 = `kv_dim` = 10 个 float = 40 字节，数字是块内第几个 float", 16, INK, "bold",
                    anchor="middle"))
    return out


def figure():
    state = snapshot()
    out = [rich(2, 18, f"KV Cache 的内存是一段：{N_BLOCKS} 块依次相接，"
                       f"块 tok 从 base 之后 tok × {BLOCK_BYTES} 字节处开始，存放 $t_{{tok}}$",
                17, INK, "bold"),
           rich(W - 4, 18, "此刻：$t_0$、$t_1$、$t_2$ 已算完，$t_3$ 还未算", 16, ORANGE, "bold", "end")]
    out += card(2, 30, 304, state)
    out += MEMORY.draw(state, focus=(ZOOM_BLOCK,))
    out += states_legend(4, 262)
    return out + zoom()


if __name__ == "__main__":
    save("kv-layout", W, H, figure())
