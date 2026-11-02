#!/usr/bin/env python3
"""Compare free-line use in equal-capacity direct-mapped and associative caches.

Read byte addresses 0, 8, 0 and 8, with m=4, B=2 and four cache lines. Repeated
reads miss in the direct-mapped cache as blocks 0 and 4 replace each other in
set 0. They hit in the fully associative cache, which keeps both blocks.
Run this script to refresh ../assets/cache-line-use.svg.
"""

from cachekit import (BLUE, FILL_BLUE, FILL_ORANGE, GREEN, INK, LINE, MUTED,
                      ORANGE, RED, WHITE, line, rect, save, text)


W, H = 1000, 310
ADDRESS_BITS, BLOCK_BYTES, LINE_COUNT = 4, 2, 4
ADDRESSES = (0, 8, 0, 8)
ROW_Y, ROW_H, ROW_STEP, ROW_W = 82, 30, 40, 360
LEFT_X, RIGHT_X = 84, 584


def simulate():
    direct = [None] * LINE_COUNT
    associative = [None] * LINE_COUNT
    replacements = []
    direct_hits, associative_hits = [], []
    for address in ADDRESSES:
        assert 0 <= address < 1 << ADDRESS_BITS
        block = address // BLOCK_BYTES
        group = block % LINE_COUNT
        old = direct[group]
        direct_hits.append(old == block)
        associative_hits.append(block in associative)
        if old is not None and old != block:
            replacements.append((old, block))
        direct[group] = block
        if block not in associative:
            associative[associative.index(None)] = block
    assert len(direct) == len(associative) == LINE_COUNT
    assert LINE_COUNT * BLOCK_BYTES == 8
    assert direct == [4, None, None, None]
    assert associative == [0, 4, None, None]
    assert replacements == [(0, 4), (4, 0), (0, 4)]
    assert direct_hits == [False, False, False, False]
    assert associative_hits == [False, False, True, True]
    return direct, associative, replacements


def cache_row(x, y, index, block, newest):
    current = block is not None and block == newest
    fill = FILL_ORANGE if current else FILL_BLUE if block is not None else WHITE
    edge = ORANGE if current else INK if block is not None else LINE
    out = [rect(x, y, ROW_W, ROW_H, fill, edge, rx=2, width=2 if current else 1.4),
           line(x + 56, y, x + 56, y + ROW_H, edge, 1.2),
           text(x + 28, y + 21, f"行 {index}", 16, MUTED)]
    if block is None:
        label = "空闲"
    else:
        first = block * BLOCK_BYTES
        label = f"块 {block}：地址 {first}–{first + BLOCK_BYTES - 1}"
    out.append(text(x + 56 + (ROW_W - 56) / 2, y + 21, label, 17,
                    ORANGE if current else INK if block is not None else MUTED,
                    "bold" if block is not None else "normal"))
    return out


def build():
    direct, associative, replacements = simulate()
    newest = ADDRESSES[-1] // BLOCK_BYTES
    out = [text(LEFT_X + ROW_W / 2, 26, "直接映射：S = 4，E = 1", 20, INK, "bold"),
           text(RIGHT_X + ROW_W / 2, 26, "全相联：S = 1，E = 4", 20, INK, "bold"),
           text(W / 2, 58, "依次读取地址 0、8、0、8（B = 2 字节）", 18, INK)]
    for index, block in enumerate(direct):
        y = ROW_Y + index * ROW_STEP
        out += cache_row(LEFT_X, y, index, block, newest)
        out.append(text(LEFT_X - 12, y + 21, f"组 {index}", 17, BLUE,
                        "bold", "end"))

    bottom = ROW_Y + (LINE_COUNT - 1) * ROW_STEP + ROW_H
    out.append(rect(RIGHT_X - 8, ROW_Y - 8, ROW_W + 16,
                    bottom - ROW_Y + 16, "none", LINE, rx=7, width=1.4, dash="5 3"))
    for index, block in enumerate(associative):
        out += cache_row(RIGHT_X, ROW_Y + index * ROW_STEP, index, block, newest)
    out.append(text(RIGHT_X - 20, (ROW_Y + bottom) / 2 + 6, "组 0", 17,
                    BLUE, "bold", "end"))

    old, new = replacements[-1]
    kept = "、".join(str(block) for block in associative if block is not None)
    out.append(text(LEFT_X + ROW_W / 2, 268, f"块 {new} 替换块 {old}", 17, RED, "bold"))
    out.append(text(RIGHT_X + ROW_W / 2, 268, f"块 {kept} 同时保留", 17, GREEN, "bold"))
    out.append(text(LEFT_X + ROW_W / 2, 296, "发生冲突时仍有空闲行", 18, RED, "bold"))
    out.append(text(RIGHT_X + ROW_W / 2, 296, "任意空闲行都能保存新块", 18, GREEN, "bold"))
    return out


if __name__ == "__main__":
    save("cache-line-use", W, H, build())
