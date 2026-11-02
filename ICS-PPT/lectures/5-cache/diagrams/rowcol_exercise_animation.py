#!/usr/bin/env python3
"""Eight checked snapshots of the complete column-major matrix-load stream.

Read the shared model JSON. Simulate fixed physical cache slots with true
per-set LRU, including every omitted access. Draw only the two selected sets
and representative matrix rows. Run this script to refresh the eight
exercise-rowcol-col-*.svg assets.
"""

import json
from math import hypot
from pathlib import Path

from svgkit import (BLUE, FILL_BLUE, FILL_GREY, FILL_ORANGE, FILL_RED,
                    GREEN, INK, LINE, MONO, MUTED, ORANGE, RED, WHITE,
                    line, path, rect, save, text)


MODEL = json.loads((Path(__file__).resolve().parent.parent / "examples"
                    / "rowcol_cache_model.json").read_text())
MATRIX, CACHE = MODEL["matrix"], MODEL["cache"]
N, ELEMENT_BYTES, BASE = MATRIX["n"], MATRIX["element_bytes"], MATRIX["base_address"]
SETS, WAYS, BLOCK_BYTES = CACHE["sets"], CACHE["ways"], CACHE["block_bytes"]
ROW_STRIDE = N * ELEMENT_BYTES
BLOCK_INTS = BLOCK_BYTES // ELEMENT_BYTES
GROUPS = tuple(sorted(int(group) for group in MODEL["column_analysis"]["column_zero_sets"]))
CHECKPOINTS = (1, 2, 3, 2 * WAYS, 2 * WAYS + 1, N, N + 1, N + 2)
W, H = 1100, 370
MX, MY, CELL_W, CELL_H = 84, 88, 52, 16.5
MATRIX_ROWS = (0, 1, 2, None, 2 * WAYS - 2, 2 * WAYS - 1, 2 * WAYS,
               None, N - 2 * WAYS, N - 2 * WAYS + 1, None, N - 2, N - 1)
MATRIX_COLS = (0, 1, None, BLOCK_INTS - 1)
CACHE_X = {group: 500 + index * 300 for index, group in enumerate(GROUPS)}
CACHE_Y, CACHE_W, CACHE_H = 86, 250, 18


def block_info(block):
    relative = block * BLOCK_BYTES - BASE
    row, byte_offset = divmod(relative, ROW_STRIDE)
    first_column = byte_offset // ELEMENT_BYTES
    return row, first_column, first_column + BLOCK_INTS - 1


def simulate():
    held = [[None] * WAYS for group in range(SETS)]
    used = [[-1] * WAYS for group in range(SETS)]
    checkpoints = set(CHECKPOINTS)
    snapshots = []
    count = misses = 0
    for column in range(N):
        for row in range(N):
            count += 1
            block = (BASE + row * ROW_STRIDE + column * ELEMENT_BYTES) // BLOCK_BYTES
            group = block % SETS
            slots = held[group]
            hit = block in slots
            evicted = None
            if hit:
                slot = slots.index(block)
            else:
                misses += 1
                slot = slots.index(None) if None in slots else min(range(WAYS), key=used[group].__getitem__)
                evicted = slots[slot]
                slots[slot] = block
            used[group][slot] = count
            if count in checkpoints:
                snapshots.append(dict(count=count, misses=misses, row=row, column=column,
                                      block=block, group=group, slot=slot, hit=hit,
                                      evicted=evicted,
                                      held={selected: tuple(held[selected]) for selected in GROUPS}))
    expected = MODEL["results"]["sum_cols"]
    assert (count, misses) == (expected["accesses"], expected["misses"])
    assert [snapshot["count"] for snapshot in snapshots] == list(CHECKPOINTS)
    assert all(not snapshot["hit"] and snapshot["count"] == snapshot["misses"] for snapshot in snapshots)
    assert [snapshot["row"] for snapshot in snapshots] == [0, 1, 2, 2 * WAYS - 1, 2 * WAYS, N - 1, 0, 1]
    assert [snapshot["column"] for snapshot in snapshots] == [0] * 6 + [1, 1]
    for index, first in ((5, N - 2 * WAYS),):
        for offset, group in enumerate(GROUPS):
            rows = {block_info(block)[0] for block in snapshots[index]["held"][group]}
            assert rows == set(range(first + offset, N, len(GROUPS)))
    assert block_info(snapshots[4]["evicted"])[0] == 0
    assert block_info(snapshots[5]["evicted"])[0] == N - 2 * WAYS - 1
    assert block_info(snapshots[6]["evicted"])[0] == N - 2 * WAYS
    assert block_info(snapshots[7]["evicted"])[0] == N - 2 * WAYS + 1
    assert snapshots[6]["block"] == snapshots[0]["block"]
    assert snapshots[7]["block"] == snapshots[1]["block"]
    return snapshots


def matrix(snapshot):
    out = [text(MX + 2 * CELL_W, 64, f"W[{N}][{N}]（前 {BLOCK_INTS} 列）", 17, INK, "bold")]
    for index, column in enumerate(MATRIX_COLS):
        out.append(text(MX + (index + 0.5) * CELL_W, MY - 8,
                        "…" if column is None else f"j={column}", 16, MUTED))
    out.append(text(MX + 4 * CELL_W + 16, MY - 8, "…", 17, MUTED))
    active_y = None
    highlights = []
    grid = []
    for position, row in enumerate(MATRIX_ROWS):
        y = MY + position * CELL_H
        if row is None:
            out.append(text(MX + 2 * CELL_W, y + 13, "⋮", 18, MUTED))
            continue
        out.append(text(MX - 10, y + 12.5, f"i={row}", 14, MUTED, anchor="end"))
        for index, column in enumerate(MATRIX_COLS):
            x = MX + index * CELL_W
            current = row == snapshot["row"] and column == snapshot["column"]
            visited = column is not None and column * N + row + 1 <= snapshot["count"]
            fill = FILL_ORANGE if current else FILL_RED if visited else FILL_BLUE
            out.append(rect(x, y, CELL_W, CELL_H, fill,
                            "none", rx=0, width=0))
            grid.append(rect(x, y, CELL_W, CELL_H, "none", BLUE, rx=0, width=0.8))
            if current:
                highlights.append(rect(x, y, CELL_W, CELL_H, "none", ORANGE,
                                       rx=0, width=1.6))
            if column is None:
                out.append(text(x + CELL_W / 2, y + 12.5, "…", 14, MUTED))
            elif visited:
                out.append(text(x + CELL_W / 2, y + 12.5, "M", 14, RED, "bold", font=MONO))
        if row == snapshot["row"]:
            active_y = y + CELL_H / 2
            highlights.append(rect(MX - 2, y, 4 * CELL_W + 4, CELL_H,
                                   "none", ORANGE, rx=2, width=2))
    assert active_y is not None
    out += grid
    out.append(text(MX + 2 * CELL_W, 338,
                    f"每行列 0–{BLOCK_INTS - 1} 同属一块", 17, BLUE, "bold"))
    return out, active_y, highlights


def cache(snapshot):
    out = []
    highlights = []
    grid = []
    for group in GROUPS:
        x = CACHE_X[group]
        out.append(text(x + CACHE_W / 2, 70, f"组 {group}（{WAYS} 行）", 18, INK, "bold"))
        out.append(rect(x - 8, CACHE_Y - 6, CACHE_W + 16, WAYS * CACHE_H + 12,
                        "none", LINE, rx=5, width=1.2, dash="5 3"))
        for slot, block in enumerate(snapshot["held"][group]):
            y = CACHE_Y + slot * CACHE_H
            current = group == snapshot["group"] and slot == snapshot["slot"]
            out.append(rect(x, y, CACHE_W, CACHE_H, WHITE if block is None else FILL_BLUE,
                            "none", rx=0, width=0))
            out.append(rect(x, y, 22, CACHE_H, FILL_GREY, "none", rx=0, width=0))
            out.append(text(x + 11, y + 13.5, str(slot), 14, MUTED, font=MONO))
            grid.append(rect(x, y, CACHE_W, CACHE_H, "none",
                             LINE if block is None else INK, rx=0, width=1))
            grid.append(line(x + 22, y, x + 22, y + CACHE_H, LINE, 0.8))
            grid.append(line(x + 82, y, x + 82, y + CACHE_H, LINE, 0.8))
            if block is None:
                out.append(text(x + 48, y + 13, "空", 14, MUTED))
            else:
                row, first, last = block_info(block)
                assert (first, last) == (0, BLOCK_INTS - 1)
                out.append(text(x + 52, y + 13, f"行{row}", 15, INK, "bold"))
                cell_w = (CACHE_W - 82) / BLOCK_INTS
                for column in range(BLOCK_INTS):
                    hot = current and column == snapshot["column"]
                    out.append(rect(x + 82 + column * cell_w, y, cell_w, CACHE_H,
                                    FILL_ORANGE if hot else FILL_BLUE,
                                    "none", rx=0, width=0))
                    if column:
                        grid.append(line(x + 82 + column * cell_w, y,
                                         x + 82 + column * cell_w, y + CACHE_H,
                                         WHITE, 0.7))
            if current:
                highlights.append(rect(x - 3, y, CACHE_W + 6, CACHE_H,
                                       "none", RED, rx=3, width=2.2))
    out += grid
    return out, highlights


def transfer_arrow(points):
    """Join a fixed-size head to the shaft using the final segment's direction."""
    end_x, end_y = points[-1]
    before_x, before_y = points[-2]
    length = hypot(end_x - before_x, end_y - before_y)
    head_length, half_width = 11, 4.5
    assert length > head_length
    dx, dy = (end_x - before_x) / length, (end_y - before_y) / length
    base_x, base_y = end_x - head_length * dx, end_y - head_length * dy
    shaft_points = [*points[:-1], (base_x, base_y)]
    shaft_path = "M " + " L ".join(f"{x:.2f} {y:.2f}" for x, y in shaft_points)
    shaft = path(shaft_path, ORANGE, 2.8, head=False).replace(
        "<path ", '<path stroke-linecap="round" stroke-linejoin="round" ', 1)
    wing_a = (base_x - half_width * dy, base_y + half_width * dx)
    wing_b = (base_x + half_width * dy, base_y - half_width * dx)
    head_path = (f"M {end_x:.2f} {end_y:.2f} "
                 f"L {wing_a[0]:.2f} {wing_a[1]:.2f} "
                 f"L {wing_b[0]:.2f} {wing_b[1]:.2f} Z")
    return [shaft, path(head_path, ORANGE, 0, fill=ORANGE, head=False)]


def transfer_route(snapshot, origin_y):
    start_x = MX + 4 * CELL_W + 7
    end_x = CACHE_X[snapshot["group"]] - 7
    end_y = CACHE_Y + (snapshot["slot"] + 0.5) * CACHE_H
    if snapshot["group"] == GROUPS[0]:
        points = [(start_x, origin_y), (end_x, end_y)]
        label = text((start_x + end_x) / 2, min(origin_y, end_y) - 12,
                     f"整块取入（{BLOCK_BYTES} B）", 17, ORANGE, "bold")
    else:
        source_turn_x = 444
        destination_turn_x = (CACHE_X[GROUPS[0]] + CACHE_W
                              + CACHE_X[GROUPS[1]]) / 2
        points = [(start_x, origin_y), (source_turn_x, origin_y),
                  (source_turn_x, 320), (destination_turn_x, 320),
                  (destination_turn_x, end_y), (end_x, end_y)]
        label = text((source_turn_x + destination_turn_x) / 2, 338,
                     f"整块取入（{BLOCK_BYTES} B）", 17, ORANGE, "bold")
    return points, label


def caption(index, snapshot):
    row, column, group = snapshot["row"], snapshot["column"], snapshot["group"]
    if index == 1:
        return (f"缓存共 {SETS} 组，每行 {ROW_STRIDE // BLOCK_BYTES} 块，"
                f"因此第 2 行开头 W[1][0] 映射到组 {group}，取入整块。")
    if index < 3:
        return f"W[{row}][{column}] 映射组 {group}，取入整块并使用空行。"
    if index == 3:
        return f"第 0 列已读到矩阵行 {2 * WAYS - 1}，两组各有 {WAYS} 块，两组内所有空行都已用完。"
    if index == 4:
        return f"W[{row}][{column}] 映射组 {group}，逐出最久未用的矩阵行 0 的块。"
    if index == 5:
        return f"第 0 列读完，两组各保留最后 {WAYS} 块，较早的块已逐出。"
    if index == 6:
        return "W[0][1] 与 W[0][0] 同块，原块已逐出，再次 miss。"
    return "W[1][1] 同理，再次 miss。后续同组块轮流替换，列遍历全部 miss。"


def frame(index, snapshot):
    out = [text(24, 22,
                f"读 {snapshot['count']}：W[{snapshot['row']}][{snapshot['column']}] → 组 {snapshot['group']}，miss",
                20, RED, "bold", "start"),
           text(W - 24, 22, f"已读 {snapshot['count']} 次，miss {snapshot['misses']} 次",
                20, INK, "bold", "end")]
    shapes, origin_y, matrix_highlights = matrix(snapshot)
    out += shapes
    cache_shapes, cache_highlights = cache(snapshot)
    out += cache_shapes
    route, label = transfer_route(snapshot, origin_y)
    out += transfer_arrow(route)
    out.append(label)
    if snapshot["evicted"] is not None:
        row, first, last = block_info(snapshot["evicted"])
        out.append(text(900, 338, f"逐出：行 {row}，列 {first}–{last} 的块", 18, RED, "bold"))
    out.append(text(W / 2, 362, caption(index, snapshot), 18, INK, "bold"))
    # Outlines overlay every cell fill, grid line, and transfer arrow.
    out += matrix_highlights + cache_highlights
    return out


if __name__ == "__main__":
    for index, snapshot in enumerate(simulate()):
        save(f"exercise-rowcol-col-{index + 1}", W, H, frame(index, snapshot))
