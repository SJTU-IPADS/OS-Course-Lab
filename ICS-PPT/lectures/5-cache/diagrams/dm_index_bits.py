#!/usr/bin/env python3
"""Compare two index positions in a 4-bit byte address, for S=4, E=1, B=2.

There are 16 byte addresses and eight two-byte blocks. Middle indexing uses
address bits 2:1; high indexing uses bits 3:2. Highlighted addresses 0--7
occupy four blocks: four distinct sets with middle indexing, two pairs of
competing blocks with high indexing. Every group number is calculated.
Run this script to refresh ../assets/dm-index-bits.svg.
"""

from cachekit import *


W, H = 1000, 440
ADDRESS_BITS, SET_BITS, OFFSET_BITS = 4, 2, 1
SETS, BLOCK_BYTES = 1 << SET_BITS, 1 << OFFSET_BITS
ADDRESS_COUNT = 1 << ADDRESS_BITS
BLOCK_COUNT = ADDRESS_COUNT // BLOCK_BYTES
TAG_BITS = ADDRESS_BITS - SET_BITS - OFFSET_BITS
X0, BYTE_W, BYTE_H, BLOCK_GAP = 40, 54, 36, 8
BLOCK_W = BLOCK_BYTES * BYTE_W
BLOCK_STEP = BLOCK_W + BLOCK_GAP
BAR_X, BIT_W, BAR_H = 430, 120, 34
HIGHLIGHT = range(0, 8)
MIDDLE_FIELDS = [("tag", TAG_BITS), ("idx", SET_BITS), ("off", OFFSET_BITS)]
HIGH_FIELDS = [("idx", SET_BITS), ("tag", TAG_BITS), ("off", OFFSET_BITS)]


def middle_index(address):
    return (address >> OFFSET_BITS) & (SETS - 1)


def high_index(address):
    return address >> (ADDRESS_BITS - SET_BITS)


def block_sets(index_of):
    return [index_of(block * BLOCK_BYTES) for block in range(BLOCK_COUNT)]


def check_mapping():
    assert BLOCK_COUNT == 8
    assert sum(width for _, width in MIDDLE_FIELDS) == ADDRESS_BITS
    assert sum(width for _, width in HIGH_FIELDS) == ADDRESS_BITS
    assert [middle_index(address) for address in HIGHLIGHT] == [0, 0, 1, 1, 2, 2, 3, 3]
    assert [high_index(address) for address in HIGHLIGHT] == [0, 0, 0, 0, 1, 1, 1, 1]
    assert block_sets(middle_index)[:4] == [0, 1, 2, 3]
    assert block_sets(high_index)[:4] == [0, 0, 1, 1]
    for index_of in (middle_index, high_index):
        assert {index_of(address) for address in range(ADDRESS_COUNT)} == set(range(SETS))
        for block in range(BLOCK_COUNT):
            address = block * BLOCK_BYTES
            assert index_of(address) == index_of(address + 1)


def bar(y, fields):
    out = [text(BAR_X - 16, y - 8, "位号", 15, MUTED, anchor="end")]
    for position in range(ADDRESS_BITS):
        out.append(text(BAR_X + (position + 0.5) * BIT_W, y - 8,
                        str(ADDRESS_BITS - position - 1), 16, MUTED, font=MONO))
    x = BAR_X
    names = {"tag": "标记", "idx": "组索引", "off": "块偏移"}
    for kind, bits in fields:
        width = bits * BIT_W
        out.append(rect(x, y, width, BAR_H, FIELD_FILL[kind], FIELD_COLOR[kind],
                        rx=3, width=2))
        out.append(text(x + width / 2, y + BAR_H / 2 + 6,
                        f"{names[kind]} {bits} 位", 17, FIELD_COLOR[kind], "bold"))
        x += width
    return out


def row(top, title, fields, index_of, note):
    out = [text(X0, top + 28, title, 20, INK, "bold", "start"),
           text(X0, top + 82, "字节地址", 16, MUTED, anchor="start")]
    out += bar(top + 42, fields)
    y = top + 110
    for block, group in enumerate(block_sets(index_of)):
        x = X0 + block * BLOCK_STEP
        for offset in range(BLOCK_BYTES):
            address = block * BLOCK_BYTES + offset
            byte_x = x + offset * BYTE_W
            out.append(rect(byte_x, y, BYTE_W, BYTE_H, SET_FILLS[group], WHITE,
                            rx=0, width=1))
            out.append(text(byte_x + BYTE_W / 2, y - 8, str(address), 16, MUTED))
        out.append(rect(x, y, BLOCK_W, BYTE_H, "none", SET_STROKES[group],
                        rx=2, width=2.2))
        out.append(text(x + BLOCK_W / 2, y + BYTE_H + 25, f"组 {group}",
                        17, INK, "bold"))
    first_block = HIGHLIGHT.start // BLOCK_BYTES
    blocks = len(HIGHLIGHT) // BLOCK_BYTES
    highlight_x = X0 + first_block * BLOCK_STEP - 4
    highlight_w = blocks * BLOCK_W + (blocks - 1) * BLOCK_GAP + 8
    out.append(rect(highlight_x, y - 4, highlight_w, 78, "none", ORANGE,
                    rx=5, width=3.2))
    out.append(text(W / 2, top + 202, note, 18, ORANGE, "bold"))
    return out


def build():
    check_mapping()
    selected_blocks = len(HIGHLIGHT) // BLOCK_BYTES
    middle = block_sets(middle_index)[:selected_blocks]
    high = block_sets(high_index)[:selected_blocks]
    middle_groups = "、".join(str(group) for group in middle)
    unused_groups = "、".join(str(group) for group in sorted(set(range(SETS)) - set(high)))
    out = row(0, "用中间位作组索引", MIDDLE_FIELDS, middle_index,
              f"地址 0–7：4 个块分别进入组 {middle_groups}，可同时保留")
    out += row(220, "用高位作组索引", HIGH_FIELDS, high_index,
               f"地址 0–7：前两块在组 {high[0]} 互相替换，后两块在组 {high[2]} 互相替换，组 {unused_groups} 未使用")
    return out


if __name__ == "__main__":
    save("dm-index-bits", W, H, build())
