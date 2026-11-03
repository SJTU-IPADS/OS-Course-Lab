#!/usr/bin/env python3
"""Writes between the CPU, one cache, and memory in five fixed-layout animations.

Registers and the cache line are inside the CPU boundary. Memory is outside it.
One arrow transfers a word and four arrows transfer the whole block. Fill every
cell before drawing grid lines, then draw current and stale outlines last.
"""

from math import hypot

from cdraw import *
from svgkit import path


W, H = 1000, 346
ROW_H, BASE = 44, 29
CPU_X, CPU_Y, CPU_W, CPU_H = 46, 10, 908, 192
REG_Y, REG_H = 28, 44
CACHE_Y, MEM_Y = 124, 250
LINE_X = 172
V_W, D_W, TAG_W, WORD_W = 56, 56, 72, 92
WORD_X = LINE_X + V_W + D_W + TAG_W
RIGHT = WORD_X + 4 * WORD_W
HOT_X = WORD_X + 3.5 * WORD_W
BYPASS_X = RIGHT + 70
COUNT_X = 876
CAPTION_Y = 333
OLD, NEW, STALE = "old", "new", "stale"


def held(value, state=OLD, dirty=None, whole=False):
    return dict(kind="held", value=value, state=state, dirty=dirty, whole=whole)


def empty(dirty=None):
    return dict(kind="empty", dirty=dirty)


def other(dirty=None):
    return dict(kind="other", dirty=dirty)


def transfer(points, colour, width=2.8):
    """A fixed-size head shares its endpoint and direction with the shaft."""
    end_x, end_y = points[-1]
    prev_x, prev_y = points[-2]
    distance = hypot(end_x - prev_x, end_y - prev_y)
    unit_x, unit_y = (end_x - prev_x) / distance, (end_y - prev_y) / distance
    head_length, half_width = 9, 4
    base_x, base_y = end_x - head_length * unit_x, end_y - head_length * unit_y
    shaft = [*points[:-1], (base_x, base_y)]
    drawing = "M " + " L ".join(f"{x:.1f} {y:.1f}" for x, y in shaft)
    head = (f"M {end_x:.1f} {end_y:.1f} "
            f"L {base_x - half_width * unit_y:.1f} {base_y + half_width * unit_x:.1f} "
            f"L {base_x + half_width * unit_y:.1f} {base_y - half_width * unit_x:.1f} Z")
    return [path(drawing, colour, width, head=False),
            path(head, colour, 0, fill=colour, head=False)]


def grid_row(y, cells, *, outline, highlight=None):
    """Draw backgrounds, labels, neutral borders, and highlights in that order."""
    out, borders = [], []
    for x, width, label, fill, stroke, colour, font in cells:
        out.append(rect(x, y, width, ROW_H, fill, "none", rx=0, width=0))
        if label:
            out.append(text(x + width / 2, y + BASE, label, 18, colour, "bold", font=font))
        borders.append(rect(x, y, width, ROW_H, "none", stroke, rx=0, width=1.3))
    left = cells[0][0]
    right = cells[-1][0] + cells[-1][1]
    borders.append(rect(left, y, right - left, ROW_H, "none", outline, rx=0, width=2))
    if highlight:
        borders.append(rect(RIGHT - WORD_W, y, WORD_W, ROW_H, "none", highlight,
                            rx=0, width=2.8))
    return out + borders


def word_cells(y, line, base_fill, base_stroke):
    cells = []
    for index in range(4):
        current = index == 3
        fresh = line["whole"] or (current and line["state"] == NEW)
        fill = FILL_ORANGE if fresh else base_fill
        stroke = ORANGE if fresh else base_stroke
        label = f"x = {line['value']}" if current else ""
        cells.append((WORD_X + index * WORD_W, WORD_W, label, fill, stroke, INK, MONO))
    return cells


def cache_row(line):
    dirty, kind = line["dirty"], line["kind"]
    live = kind != "empty"
    fill, stroke = (FILL_BLUE, INK) if live else (WHITE, LINE)
    left = LINE_X if dirty is not None else LINE_X + D_W
    cells = [(left, V_W, f"V={int(live)}", fill, stroke, INK if live else MUTED, MONO)]
    x = left + V_W
    if dirty is not None:
        cells.append((x, D_W, f"D={dirty}" if live else "", fill, stroke,
                      RED if dirty else INK, MONO))
        x += D_W
    cells.append((x, TAG_W, "标记" if live else "", FILL_PURPLE if live else WHITE,
                  PURPLE if live else LINE, PURPLE, FONT))
    if kind == "held":
        cells += word_cells(CACHE_Y, line, FILL_BLUE, INK)
    elif kind == "empty":
        cells += [(WORD_X + index * WORD_W, WORD_W, "", WHITE, LINE, MUTED, MONO)
                  for index in range(4)]
    else:
        cells.append((WORD_X, 4 * WORD_W, "另一个块", FILL_GREY, MUTED, MUTED, FONT))
    highlight = {NEW: ORANGE, STALE: RED}.get(line.get("state"))
    return grid_row(CACHE_Y, cells, outline=INK if live else LINE, highlight=highlight)


def mem_row(line):
    highlight = {NEW: ORANGE, STALE: RED}.get(line["state"])
    return grid_row(MEM_Y, word_cells(MEM_Y, line, FILL_GREEN, GREEN),
                    outline=GREEN, highlight=highlight)


def flow(source, target, colour, label="", *, x=HOT_X):
    bounds = {"reg": (REG_Y, REG_Y + REG_H),
              "cache": (CACHE_Y, CACHE_Y + ROW_H),
              "mem": (MEM_Y, MEM_Y + ROW_H)}
    source_top, source_bottom = bounds[source]
    target_top, target_bottom = bounds[target]
    downward = source_top < target_top
    start_y = source_bottom + 3 if downward else source_top - 3
    end_y = target_top - 4 if downward else target_bottom + 4
    out = transfer([(x, start_y), (x, end_y)], colour)
    if label:
        out.append(text(x - 14, (start_y + end_y) / 2 + 6, label, 18, colour, "bold", "end"))
    return out


def block_flow(source, target, colour, label):
    out = []
    for index in range(4):
        out += flow(source, target, colour, x=WORD_X + (index + 0.5) * WORD_W)
    middle_y = (CACHE_Y + ROW_H + MEM_Y) / 2
    out.append(text(WORD_X - 15, middle_y + 6, label, 18, colour, "bold", "end"))
    return out


def bypass_flow():
    middle_reg = REG_Y + REG_H / 2
    middle_mem = MEM_Y + ROW_H / 2
    out = transfer([(RIGHT + 3, middle_reg), (BYPASS_X, middle_reg),
                    (BYPASS_X, middle_mem), (RIGHT + 5, middle_mem)], ORANGE)
    out.append(text(BYPASS_X + 14, 219, "写 x = 2", 18, ORANGE, "bold", "start"))
    return out


def frame(snapshot):
    out = [rect(CPU_X, CPU_Y, CPU_W, CPU_H, WHITE, INK, rx=8, width=2),
           text(CPU_X + 53, CPU_Y + 34, "CPU", 20, INK, "bold")]
    out += box(WORD_X, REG_Y, 4 * WORD_W, REG_H, "寄存器", FILL_GREY, INK, 19)
    out.append(text(155, CACHE_Y + BASE, "缓存", 20, INK, "bold", "end"))
    out += cache_row(snapshot["cache"])
    out.append(text(155, MEM_Y + BASE, "内存", 20, GREEN, "bold", "end"))
    out += mem_row(snapshot["mem"])
    for shapes in snapshot.get("flows", []):
        out += shapes
    if snapshot.get("counts"):
        out.append(text(COUNT_X, CACHE_Y - 22, "被写入的次数", 17, MUTED, "bold"))
        for y, label in zip((CACHE_Y, MEM_Y), snapshot["counts"]):
            out.append(text(COUNT_X, y + BASE, label, 20, snapshot.get("count_colour", INK), "bold"))
    out.append(text(W / 2, CAPTION_Y, snapshot["caption"], 20,
                    snapshot.get("caption_colour", INK), "bold"))
    return out


PROBLEM = [
    dict(cache=held(1), mem=held(1),
         caption="① 缓存保存 x 所在块的副本，缓存与内存中的 x 都为 1"),
    dict(cache=held(1), mem=held(1),
         flows=[flow("reg", "cache", ORANGE, "写 x = 2")],
         caption="② CPU 写 x = 2，该块已在缓存中"),
    dict(cache=held(2, NEW), mem=held(1, STALE),
         caption="③ 只更新缓存时，缓存中的 x 为 2，内存中的 x 仍为 1"),
    dict(cache=other(), mem=held(1, STALE), caption_colour=RED,
         caption="④ 错误处理：替换时直接丢弃这一行，写入的新值 2 丢失"),
    dict(cache=held(1), mem=held(1),
         flows=[block_flow("mem", "cache", GREEN, "整块取入"),
                flow("cache", "reg", RED, "读到 x = 1")], caption_colour=RED,
         caption="⑤ 再读 x 发生 miss，从内存取回旧值 1，结果错误"),
]

THROUGH = [
    dict(cache=held(1), mem=held(1),
         flows=[flow("reg", "cache", ORANGE, "写 x = 2")], counts=("0 次", "0 次"),
         caption="① CPU 写 x = 2，该块已在缓存中，写 hit"),
    dict(cache=held(2, NEW), mem=held(2, NEW),
         flows=[flow("reg", "cache", ORANGE, "写 x = 2"),
                flow("cache", "mem", ORANGE, "写入这个字")], counts=("1 次", "1 次"),
         caption="② 更新缓存，同时把这个字写到内存"),
    dict(cache=held(2), mem=held(2), counts=("1 次", "1 次"),
         caption="③ 此次写完成，缓存与内存中的 x 都为 2"),
    dict(cache=held(101, NEW), mem=held(101, NEW),
         flows=[flow("reg", "cache", ORANGE, "再写 99 次"), flow("cache", "mem", ORANGE)],
         counts=("100 次", "100 次"), count_colour=ORANGE,
         caption="④ 同一个字写 100 次，缓存与内存各被写入 100 次"),
]

BACK = [
    dict(cache=held(2, NEW, dirty=1), mem=held(1, STALE),
         flows=[flow("reg", "cache", ORANGE, "写 x = 2")], counts=("1 次", "0 次"),
         caption="① 写 hit 时只更新缓存，D 置 1，记录尚未写回的修改"),
    dict(cache=held(101, NEW, dirty=1), mem=held(1, STALE),
         flows=[flow("reg", "cache", ORANGE, "再写 99 次")], counts=("100 次", "0 次"),
         caption="② 同一个字再写 99 次，每次只更新缓存"),
    dict(cache=held(101, dirty=1), mem=held(101, NEW, whole=True),
         flows=[block_flow("cache", "mem", ORANGE, "整块写回")],
         counts=("100 次", "1 次（整块）"),
         caption="③ 替换前检查 D = 1，把整个块写回内存"),
    dict(cache=other(dirty=0), mem=held(101), counts=("100 次", "1 次（整块）"),
         caption="④ 写回完成后替换这一行，100 次缓存写只产生 1 次整块写回"),
]

ALLOCATE = [
    dict(cache=empty(dirty=0), mem=held(1),
         flows=[flow("reg", "cache", ORANGE, "写 x = 2")],
         caption="① CPU 写 x = 2，缓存中没有该块，写 miss"),
    dict(cache=held(1, dirty=0), mem=held(1),
         flows=[block_flow("mem", "cache", GREEN, "整块取入")],
         caption="② 从内存取入整个块，放入缓存"),
    dict(cache=held(2, NEW, dirty=1), mem=held(1, STALE),
         flows=[flow("reg", "cache", ORANGE, "写 x = 2")],
         caption="③ 按写 hit 处理，图中采用写回：更新缓存，D 置 1"),
    dict(cache=held(2, dirty=1), mem=held(1, STALE),
         flows=[flow("cache", "reg", GREEN, "读 / 写", x=HOT_X - 24),
                flow("reg", "cache", GREEN, x=HOT_X + 24)],
         caption="④ 只要该块仍在缓存中，后续对块内字的读写都 hit"),
]

AROUND = [
    dict(cache=empty(), mem=held(1),
         flows=[flow("reg", "cache", ORANGE, "写 x = 2")],
         caption="① CPU 写 x = 2，缓存中没有该块，写 miss"),
    dict(cache=empty(), mem=held(2, NEW), flows=[bypass_flow()],
         caption="② 不取入该块，把这个字直接写到内存"),
    dict(cache=empty(), mem=held(2),
         caption="③ 写完成后缓存仍无此块，这次写省去了一次整块取入"),
    dict(cache=held(2), mem=held(2),
         flows=[flow("reg", "cache", ORANGE, "读 x"),
                block_flow("mem", "cache", GREEN, "整块取入")],
         caption="④ 随后读 x 仍 miss，此时才从内存取入整个块"),
]


if __name__ == "__main__":
    for name, snapshots in (("write-problem", PROBLEM), ("write-through", THROUGH),
                            ("write-back", BACK), ("write-allocate", ALLOCATE),
                            ("no-write-allocate", AROUND)):
        for index, snapshot in enumerate(snapshots, 1):
            save(f"{name}-{index}", W, H, frame(snapshot))
