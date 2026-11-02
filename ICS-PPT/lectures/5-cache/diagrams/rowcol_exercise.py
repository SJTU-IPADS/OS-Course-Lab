#!/usr/bin/env python3
"""Static address, row-access and column-access figures for the cache exercise.

All model numbers come from examples/rowcol_cache_model.json. Geometry and
colours use the lecture's existing SVG helpers. Run this script to refresh
the three exercise-rowcol-*.svg assets.
"""

import json
from collections import Counter
from pathlib import Path

from cachekit import (BLUE, FIELD_COLOR, FIELD_FILL, FILL_BLUE, FILL_GREEN,
                      FILL_ORANGE, FILL_RED, GREEN, INK, LINE, MONO, MUTED, ORANGE, RED,
                      WHITE, arrow, brace, line, rect, save, text)


MODEL = json.loads((Path(__file__).resolve().parent.parent / "examples"
                    / "rowcol_cache_model.json").read_text())
CACHE, MATRIX = MODEL["cache"], MODEL["matrix"]
ADDRESS_BITS = CACHE["address_bits"]
BLOCK_BYTES, SETS, WAYS = CACHE["block_bytes"], CACHE["sets"], CACHE["ways"]
ELEMENT_BYTES = MATRIX["element_bytes"]
ROWS, COLUMNS = MATRIX.get("n", MATRIX.get("rows")), MATRIX.get("n", MATRIX.get("columns"))
ROW_STRIDE = COLUMNS * ELEMENT_BYTES
INTS_PER_BLOCK = BLOCK_BYTES // ELEMENT_BYTES
OFFSET_BITS = BLOCK_BYTES.bit_length() - 1
INDEX_BITS = SETS.bit_length() - 1
TAG_BITS = ADDRESS_BITS - OFFSET_BITS - INDEX_BITS
BASE_ADDRESS = MATRIX["base_address"]
EXAMPLE_COLUMN = 0
COLUMN_GROUPS = Counter(
    ((BASE_ADDRESS + row * ROW_STRIDE + EXAMPLE_COLUMN * ELEMENT_BYTES) // BLOCK_BYTES) % SETS
    for row in range(ROWS)
)
W = 1000

assert BLOCK_BYTES == 1 << OFFSET_BITS
assert SETS == 1 << INDEX_BITS
assert INTS_PER_BLOCK * ELEMENT_BYTES == BLOCK_BYTES
assert ROW_STRIDE % BLOCK_BYTES == 0
assert BASE_ADDRESS % BLOCK_BYTES == 0
assert len(COLUMN_GROUPS) == 2
assert len(set(COLUMN_GROUPS.values())) == 1


def address():
    left, width, top, height = 40, 920, 64, 70
    fields = [("tag", TAG_BITS, "标记"), ("idx", INDEX_BITS, "组索引"),
              ("off", OFFSET_BITS, "块偏移")]
    out = brace(left, left + width, 34, INK, f"{ADDRESS_BITS} 位字节地址",
                20, below=False)
    x, high = left, ADDRESS_BITS - 1
    for kind, bits, label in fields:
        field_width = width * bits / ADDRESS_BITS
        low = high - bits + 1
        middle = x + field_width / 2
        out.append(text(middle, top - 10, f"{high}..{low}", 17, MUTED, font=MONO))
        out.append(rect(x, top, field_width, height, FIELD_FILL[kind],
                        FIELD_COLOR[kind], rx=3, width=2))
        out.append(text(middle, top + 28, label, 19, FIELD_COLOR[kind], "bold"))
        out.append(text(middle, top + 56, f"{bits} 位", 18, FIELD_COLOR[kind], "bold"))
        x, high = x + field_width, low - 1
    return out


def row_access():
    left, top, width, height = 40, 54, 920, 64
    cell_width = width / INTS_PER_BLOCK
    out = [text(left - 18, top - 14, "j", 17, MUTED, anchor="end", font=MONO)]
    for index in range(INTS_PER_BLOCK):
        x = left + index * cell_width
        first = index == 0
        fill, color = (FILL_RED, RED) if first else (FILL_GREEN, GREEN)
        out.append(text(x + cell_width / 2, top - 14, str(index), 17, MUTED, font=MONO))
        out.append(rect(x, top, cell_width, height, fill, WHITE, rx=0, width=1))
        out.append(text(x + cell_width / 2, top + height / 2 + 8,
                        "M" if first else "H", 24, color, "bold", font=MONO))
    out.append(rect(left - 2, top - 2, width + 4, height + 4, "none", INK,
                    rx=4, width=2))
    out.append(text(W / 2, 155,
                    f"同一块 {BLOCK_BYTES} 字节，{INTS_PER_BLOCK} 次读取："
                    f"1 次 miss（M），{INTS_PER_BLOCK - 1} 次 hit（H）",
                    18, INK, "bold"))
    return out


def column_access():
    source_x, source_w, source_h = 42, 232, 36
    group_x, group_w, group_h = 624, 320, 64
    selected = sorted(COLUMN_GROUPS)
    group_tops = {group: 60 + index * 100 for index, group in enumerate(selected)}
    out = [text(source_x, 24, f"列 j={EXAMPLE_COLUMN}", 19, INK, "bold", "start"),
           text(450, 56, f"{ROWS} 个不同的块", 19, INK, "bold"),
           text(group_x + group_w / 2, 24,
                f"选中的 {len(selected)} 个组（缓存共 {SETS} 组）", 19, INK, "bold")]
    for row, y in ((0, 44), (1, 88), (ROWS - 1, 158)):
        out.append(rect(source_x, y, source_w, source_h, FILL_GREEN, GREEN,
                        rx=3, width=1.6))
        out.append(text(source_x + source_w / 2, y + source_h / 2 + 7,
                        f"W[{row}][{EXAMPLE_COLUMN}]", 19, INK, "bold", font=MONO))
        group = ((BASE_ADDRESS + row * ROW_STRIDE) // BLOCK_BYTES) % SETS
        out.append(arrow(source_x + source_w, y + source_h / 2,
                         group_x - 6, group_tops[group] + group_h / 2, GREEN, 2))
    out.append(text(source_x + source_w / 2, 146, "⋮", 24, MUTED))
    for group in selected:
        group_y = group_tops[group]
        out.append(text(group_x + group_w / 2, group_y - 10,
                        f"组 {group}：{COLUMN_GROUPS[group]} 个块", 17, BLUE, "bold"))
        out.append(rect(group_x, group_y, group_w, group_h, "none", BLUE,
                        rx=5, width=1.8))
        slot_pitch = (group_h - 12) / WAYS
        for index in range(WAYS):
            y = group_y + 6 + index * slot_pitch
            out.append(rect(group_x + 12, y, group_w - 24, slot_pitch * 0.64,
                            FILL_BLUE, LINE, rx=0, width=0.8))
    out.append(text(source_x + source_w / 2, 224,
                    f"行跨度 {ROW_STRIDE // 1024} KiB", 18, MUTED))
    out.append(text(450, 238, "相邻两行交替进入两组", 18, BLUE, "bold"))
    out.append(text(group_x + group_w / 2, 250,
                    f"每组 {WAYS} 行，最多保留 {WAYS} 个块", 19, ORANGE, "bold"))
    return out


def summary():
    left, top, width, height = 24, 65, 280, 36
    cell_width = width / INTS_PER_BLOCK
    count = min(COLUMN_GROUPS.values())
    out = [text(left, 24, "sum_rows", 19, INK, "bold", "start"),
           text(left, 50, f"同一块的 {INTS_PER_BLOCK} 个 int", 18, MUTED, anchor="start"),
           text(346, 24, f"sum_cols：每组仅 {WAYS} 行，对应 {count} 块",
                19, INK, "bold", "start"),
           text(396, 47, "第 0 列按矩阵行 0、1、2、… 读取", 18, MUTED, anchor="start"),
           line(326, 12, 326, 212, LINE, 1.2)]
    for index in range(INTS_PER_BLOCK):
        x = left + index * cell_width
        first = index == 0
        fill, color = (FILL_RED, RED) if first else (FILL_GREEN, GREEN)
        out.append(rect(x, top, cell_width, height, fill, WHITE, rx=0, width=1))
        out.append(text(x + cell_width / 2, top + height / 2 + 6,
                        "M" if first else "H", 18, color, "bold", font=MONO))
    out.append(rect(left - 2, top - 2, width + 4, height + 4, "none", INK,
                    rx=3, width=1.6))
    out.append(text(left + width / 2, 132,
                    f"1 miss + {INTS_PER_BLOCK - 1} hit", 18, INK, "bold"))

    for index, group in enumerate(sorted(COLUMN_GROUPS)):
        rows = [row for row in range(ROWS)
                if ((BASE_ADDRESS + row * ROW_STRIDE) // BLOCK_BYTES) % SETS == group]
        retained, incoming = rows[:WAYS], rows[WAYS]
        shown = (retained[0], retained[1], None, retained[-1])
        y, cache_x, cell = 60 + index * 44, 396, 53
        out.append(text(cache_x - 12, y + 23, f"组 {group}", 18, BLUE, "bold", "end"))
        for position, row in enumerate(shown):
            x = cache_x + position * cell
            victim = position == 0
            out.append(rect(x, y, cell, 32, FILL_RED if victim else FILL_BLUE,
                            WHITE, rx=0, width=1))
            label = text(x + cell / 2, y + 23, "…" if row is None else f"行 {row}",
                         18, RED if victim else INK, "bold" if victim else "normal")
            if victim:
                label = label.replace(">", ' text-decoration="line-through">', 1)
            out.append(label)
        out.append(rect(cache_x, y, cell * len(shown), 32, "none", BLUE,
                        rx=3, width=1.6))
        out.append(rect(650, y, 76, 32, FILL_ORANGE, ORANGE, rx=3, width=1.6))
        out.append(text(688, y + 23, f"行 {incoming}", 18, ORANGE, "bold"))
        out.append(arrow(644, y + 16, 614, y + 16, ORANGE, 2.4))
        out.append(text(744, y + 23, f"逐出行 {retained[0]} 的块", 18,
                        RED, "bold", "start"))
    out.append(text(656, 160, "…继续读完第 0 列，再读第 1 列", 18, MUTED))
    out.append(text(656, 185, "W[0][1] 与 W[0][0] 同块，原块已逐出 → miss",
                    18, RED, "bold"))
    out.append(text(656, 211, "W[1][1] 同理，其余行也都 miss", 18, RED, "bold"))
    return out


def summary_rows():
    left, top, width, height = 40, 42, 920, 48
    block_width = width / 2
    cell_width = block_width / INTS_PER_BLOCK
    out = []
    for block in range(2):
        block_x = left + block * block_width
        out.append(text(block_x + block_width / 2, 24,
                        f"第 {block + 1} 块（{BLOCK_BYTES} 字节）", 19, INK, "bold"))
        for index in range(INTS_PER_BLOCK):
            x = block_x + index * cell_width
            first = index == 0
            fill, color = (FILL_RED, RED) if first else (FILL_GREEN, GREEN)
            out.append(rect(x, top, cell_width, height, fill, WHITE, rx=0, width=1))
            out.append(text(x + cell_width / 2, top + height / 2 + 6,
                            "M" if first else "H", 18, color, "bold", font=MONO))
        out.append(rect(block_x, top, block_width, height, "none", INK,
                        rx=0, width=2))
    out.append(text(W / 2, 130,
                    f"每块 {INTS_PER_BLOCK} 次读取：1 miss + {INTS_PER_BLOCK - 1} hit",
                    19, INK, "bold"))
    return out


if __name__ == "__main__":
    save("exercise-rowcol-address", W, 160, address())
    save("exercise-rowcol-row", W, 174, row_access())
    save("exercise-rowcol-column", W, 260, column_access())
    save("exercise-rowcol-summary", W, 220, summary())
    save("exercise-rowcol-rows-summary", W, 150, summary_rows())
