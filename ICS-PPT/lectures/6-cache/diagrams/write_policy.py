#!/usr/bin/env python3
"""Writes on the stack processor, L1, L2, main memory: one figure, five animations.

write-problem: a write that only L1 sees, so the copies differ and the value can be lost.
write-through, write-back: a write hit under each policy, with the number of writes every
level receives. write-allocate, no-write-allocate: a write miss under each policy, each
drawn with the write-hit policy it is usually paired with.

Every level holds one line (a valid bit, a dirty bit under write-back, a tag and four
words); main memory holds the block alone. The word x is the last one of the block, so
its cells line up and an arrow runs straight down them. One arrow carries one word, four
arrows carry the whole block. Blue is a current copy, orange a copy just written, a red
outline a stale copy. Every frame of every animation uses the same coordinates.
Run it to refresh ../assets/write-problem-*.svg, write-through-*.svg, write-back-*.svg,
write-allocate-*.svg and no-write-allocate-*.svg.
"""

from cdraw import *
from svgkit import elbow

W, H = 1000, 346
TOP = {"cpu": (4, 44), "l1": (86, 130), "l2": (172, 216), "mem": (258, 302)}
ROW_H = 44
BASE = 29                          # baseline of the text inside a row
NAMES = {"l1": "L1 缓存", "l2": "L2 缓存", "mem": "内存"}
NAME_X = 158                       # right edge of the level names
LINE_X = 172                       # left edge of a line with a dirty bit
V_W, D_W, TAG_W, WORD_W = 56, 56, 72, 92
WORD_X = LINE_X + V_W + D_W + TAG_W
RIGHT = WORD_X + 4 * WORD_W
HOT_X = WORD_X + 3.5 * WORD_W      # centre of the word x
BYPASS_X = RIGHT + 30
NOTE_X = RIGHT + 54                # left edge of the notes beside a level
COUNT_X = 880                      # centre of the column of write counts
CAPTION_Y = 334

OLD, NEW, STALE = "old", "new", "stale"


def held(value, state=OLD, dirty=None, whole=False):
    """A line, or the block in main memory, that holds x."""
    return dict(kind="held", value=value, state=state, dirty=dirty, whole=whole)


def empty(dirty=None):
    return dict(kind="empty", dirty=dirty)


def other(dirty=None):
    """The line now holds another block: x was replaced."""
    return dict(kind="other", dirty=dirty)


def word_cell(x, y, value, state, base_fill, base_stroke):
    """The cell of x; the stale outline is returned apart, to lie over the neighbours."""
    fill, stroke = (FILL_ORANGE, ORANGE) if state == NEW else (base_fill, base_stroke)
    out = [rect(x, y, WORD_W, ROW_H, fill, stroke, rx=0, width=1.4),
           text(x + WORD_W / 2, y + BASE, f"x = {value}", 19, INK, "bold", font=MONO)]
    top = [rect(x, y, WORD_W, ROW_H, "none", RED, rx=0, width=3)] if state == STALE else []
    top += [rect(x, y, WORD_W, ROW_H, "none", ORANGE, rx=0, width=2.6)] if state == NEW else []
    return out, top


def words(y, line, fill, stroke):
    """The four words of a line that holds x; `whole` paints the block just written."""
    out, top = [], []
    whole = line["whole"]
    for k in range(3):
        x = WORD_X + k * WORD_W
        out.append(rect(x, y, WORD_W, ROW_H, FILL_ORANGE if whole else fill,
                        ORANGE if whole else stroke, rx=0, width=1.4))
    shapes, over = word_cell(RIGHT - WORD_W, y, line["value"], line["state"], fill, stroke)
    return out + shapes, top + over


def cache_row(key, line):
    y = TOP[key][0]
    dirty = line["dirty"]
    x = LINE_X if dirty is not None else LINE_X + D_W
    kind = line["kind"]
    live = kind != "empty"
    fill, stroke = (FILL_BLUE, INK) if live else (WHITE, LINE)
    out = [rect(x, y, V_W, ROW_H, fill, stroke, rx=0, width=1.4),
           text(x + V_W / 2, y + BASE - 1, f"V={int(live)}", 17, INK if live else MUTED, font=MONO)]
    x += V_W
    if dirty is not None:
        out.append(rect(x, y, D_W, ROW_H, fill, stroke, rx=0, width=1.4))
        if live:
            out.append(text(x + D_W / 2, y + BASE - 1, f"D={dirty}", 17, RED if dirty else INK,
                            "bold" if dirty else "normal", font=MONO))
        x += D_W
    tag_on = kind == "held"
    out.append(rect(x, y, TAG_W, ROW_H, FILL_PURPLE if tag_on else fill,
                    PURPLE if tag_on else stroke, rx=0, width=1.4))
    if live:
        out.append(text(x + TAG_W / 2, y + BASE, "标记", 18, PURPLE if tag_on else MUTED, "bold"))
    top = []
    if kind == "held":
        shapes, top = words(y, line, FILL_BLUE, INK)
        out += shapes
    elif kind == "other":
        out.append(rect(WORD_X, y, 4 * WORD_W, ROW_H, FILL_GREY, MUTED, rx=0, width=1.4))
        out.append(text(WORD_X + 2 * WORD_W, y + BASE, "另一个块", 18, MUTED, "bold"))
    else:
        for k in range(4):
            out.append(rect(WORD_X + k * WORD_W, y, WORD_W, ROW_H, WHITE, LINE, rx=0, width=1.4))
    left = LINE_X if dirty is not None else LINE_X + D_W
    if live:
        out.append(rect(left, y, RIGHT - left, ROW_H, "none", INK, rx=0, width=2))
    return out, top


def mem_row(line):
    y = TOP["mem"][0]
    shapes, top = words(y, line, FILL_GREEN, GREEN)
    return shapes + [rect(WORD_X, y, RIGHT - WORD_X, ROW_H, "none", GREEN, rx=0, width=2)], top


def ends(a, b):
    """The y of an arrow from level a to the next level b, in either direction."""
    if TOP[a][0] < TOP[b][0]:
        return TOP[a][1] + 2, TOP[b][0] - 3
    return TOP[a][0] - 2, TOP[b][1] + 3


def word_flow(a, b, colour, label="", x=HOT_X):
    y0, y1 = ends(a, b)
    out = [arrow(x, y0, x, y1, colour, 3)]
    if label:
        out.append(text(x - 14, (y0 + y1) / 2 + 7, label, 18, colour, "bold", "end"))
    return out


def block_flow(a, b, colour, label=""):
    y0, y1 = ends(a, b)
    out = [arrow(WORD_X + (k + 0.5) * WORD_W, y0, WORD_X + (k + 0.5) * WORD_W, y1, colour, 3)
           for k in range(4)]
    if label:
        out.append(text(WORD_X + 0.5 * WORD_W - 14, (y0 + y1) / 2 + 7, label, 18, colour, "bold",
                        "end"))
    return out


def bypass_flow(label):
    """From the processor past L1 into the word x of L2."""
    y0 = (TOP["cpu"][0] + TOP["cpu"][1]) / 2
    y1 = (TOP["l2"][0] + TOP["l2"][1]) / 2
    out = [elbow([(RIGHT + 2, y0), (BYPASS_X, y0), (BYPASS_X, y1), (RIGHT + 4, y1)], ORANGE, 3)]
    out.append(text(BYPASS_X + 14, (TOP["l1"][0] + TOP["l1"][1]) / 2 + 7, label, 18, ORANGE, "bold",
                    "start"))
    return out


def frame(f):
    out = box(LINE_X, TOP["cpu"][0], RIGHT - LINE_X, TOP["cpu"][1] - TOP["cpu"][0], "处理器", WHITE, INK, 19)
    over = []
    for key in ("l1", "l2", "mem"):
        out.append(text(NAME_X, TOP[key][0] + BASE, NAMES[key], 20, INK, "bold", "end"))
        shapes, top = mem_row(f[key]) if key == "mem" else cache_row(key, f[key])
        out += shapes
        over += top
    out += over
    for flow in f.get("flows", []):
        out += flow
    for key, label, colour in f.get("notes", []):
        out.append(text(NOTE_X, (TOP[key][0] + TOP[key][1]) / 2 + 7, label, 19, colour, "bold", "start"))
    counts = f.get("counts")
    if counts:
        out.append(text(COUNT_X, TOP["l1"][0] - 18, "被写入的次数", 17, MUTED, "bold"))
        colour = f.get("count_colour", INK)
        for key, count in zip(("l1", "l2", "mem"), counts):
            hot = key in f.get("count_hot", ())
            out.append(text(COUNT_X, TOP[key][0] + BASE + 1, count, 22, colour if not hot else ORANGE,
                            "bold"))
    out.append(text(W / 2, CAPTION_Y, f["caption"], 20, f.get("caption_colour", INK), "bold"))
    return out


PROBLEM = [
    dict(l1=held(1), l2=held(1), mem=held(1),
         caption="① x 的块在 L1、L2 与内存中各有一份副本，值都是 1"),
    dict(l1=held(1), l2=held(1), mem=held(1),
         flows=[word_flow("cpu", "l1", ORANGE, "写 x = 2")],
         caption="② 处理器写 x = 2，该块在 L1 中"),
    dict(l1=held(2, NEW), l2=held(1, STALE), mem=held(1, STALE),
         notes=[("l2", "旧值", RED), ("mem", "旧值", RED)],
         caption="③ L1 更新自己的副本，L2 与内存中的 x 仍是 1，三份副本不一致"),
    dict(l1=other(), l2=held(1, STALE), mem=held(1, STALE),
         notes=[("l1", "x = 2 丢失", RED), ("l2", "旧值", RED), ("mem", "旧值", RED)],
         caption="④ L1 把这一行替换出去，新值 2 不在任何一层中"),
    dict(l1=held(1), l2=held(1), mem=held(1),
         flows=[word_flow("l1", "cpu", RED, "读到 x = 1"), block_flow("l2", "l1", GREEN, "取块")],
         notes=[("l1", "不命中", RED)],
         caption="⑤ 再读 x：L1 不命中，从 L2 取得 x = 1，写入的值丢失"),
]

THROUGH = [
    dict(l1=held(2, NEW), l2=held(1, STALE), mem=held(1, STALE),
         flows=[word_flow("cpu", "l1", ORANGE, "写 x = 2")],
         counts=("1 次", "0 次", "0 次"), count_hot=("l1",),
         caption="① 处理器写 x = 2，L1 写命中，更新 x"),
    dict(l1=held(2), l2=held(2, NEW), mem=held(1, STALE),
         flows=[word_flow("l1", "l2", ORANGE, "写 x = 2")],
         counts=("1 次", "1 次", "0 次"), count_hot=("l2",),
         caption="② L1 同时把这个字写到 L2"),
    dict(l1=held(2), l2=held(2), mem=held(2, NEW),
         flows=[word_flow("l2", "mem", ORANGE, "写 x = 2")],
         notes=[("cpu", "等待内存写完", RED)],
         counts=("1 次", "1 次", "1 次"), count_hot=("mem",),
         caption="③ L2 同时把这个字写到内存，三份副本一致，这次写才结束"),
    dict(l1=held(101, NEW), l2=held(101, NEW), mem=held(101, NEW),
         flows=[word_flow("cpu", "l1", RED, "再写 99 次，x 增至 101"), word_flow("l1", "l2", RED),
                word_flow("l2", "mem", RED)],
         notes=[("cpu", "每次都等内存", RED)],
         counts=("100 次", "100 次", "100 次"), count_colour=RED, caption_colour=RED,
         caption="④ 同一个字写 100 次，内存被写 100 次，每次写都要等内存写完"),
]

BACK = [
    dict(l1=held(2, NEW, dirty=1), l2=held(1, STALE, dirty=0), mem=held(1, STALE),
         flows=[word_flow("cpu", "l1", ORANGE, "写 x = 2")],
         counts=("1 次", "0 次", "0 次"), count_hot=("l1",),
         caption="① 处理器写 x = 2，L1 写命中：只更新 L1，脏位 D 置 1"),
    dict(l1=held(101, NEW, dirty=1), l2=held(1, STALE, dirty=0), mem=held(1, STALE),
         flows=[word_flow("cpu", "l1", ORANGE, "再写 99 次，x 增至 101")],
         counts=("100 次", "0 次", "0 次"), count_hot=("l1",),
         caption="② 同一个字再写 99 次：每次只更新 L1，不写下一层"),
    dict(l1=other(dirty=0), l2=held(101, NEW, dirty=1, whole=True), mem=held(1, STALE),
         flows=[block_flow("l1", "l2", ORANGE, "整块")],
         counts=("100 次", "1 次（整块）", "0 次"), count_hot=("l2",),
         caption="③ L1 替换这一行：D = 1，整块写到 L2，L2 这一行的 D 置 1"),
    dict(l1=other(dirty=0), l2=other(dirty=0), mem=held(101, NEW, whole=True),
         flows=[block_flow("l2", "mem", ORANGE, "整块")],
         counts=("100 次", "1 次（整块）", "1 次（整块）"), count_hot=("mem",),
         caption="④ L2 替换这一行：D = 1，整块写到内存。写 100 次，内存只被写 1 次"),
]

ALLOCATE = [
    dict(l1=empty(dirty=0), l2=held(1, dirty=0), mem=held(1),
         flows=[word_flow("cpu", "l1", ORANGE, "写 x = 2")],
         notes=[("l1", "不命中", RED)],
         caption="① 处理器写 x = 2，L1 中没有该块，写不命中"),
    dict(l1=held(1, dirty=0), l2=held(1, dirty=0), mem=held(1),
         flows=[block_flow("l2", "l1", GREEN, "取块")],
         caption="② 把该块从 L2 取入 L1"),
    dict(l1=held(2, NEW, dirty=1), l2=held(1, STALE, dirty=0), mem=held(1, STALE),
         flows=[word_flow("cpu", "l1", ORANGE, "写 x = 2")],
         caption="③ 按写命中处理。图中的写命中策略是写回：更新 L1 的 x，D 置 1"),
    dict(l1=held(2, dirty=1), l2=held(1, STALE, dirty=0), mem=held(1, STALE),
         flows=[word_flow("l1", "cpu", GREEN, "读写这个块", x=HOT_X - WORD_W), word_flow("cpu", "l1", GREEN)],
         notes=[("l1", "命中", GREEN)],
         caption="④ 之后读写这个块中的字，L1 都命中"),
]

AROUND = [
    dict(l1=empty(), l2=held(1), mem=held(1),
         flows=[word_flow("cpu", "l1", ORANGE, "写 x = 2")],
         notes=[("l1", "不命中", RED)],
         caption="① 处理器写 x = 2，L1 中没有该块，写不命中"),
    dict(l1=empty(), l2=held(2, NEW), mem=held(1, STALE),
         flows=[bypass_flow("写 x = 2")],
         caption="② 不取块，把这个字直接写到 L2，L1 不变"),
    dict(l1=empty(), l2=held(2), mem=held(2, NEW),
         flows=[word_flow("l2", "mem", ORANGE, "写 x = 2")],
         caption="③ 图中的写命中策略是写直达：L2 同时把这个字写到内存"),
    dict(l1=empty(), l2=held(2), mem=held(2),
         flows=[word_flow("cpu", "l1", ORANGE, "读 x"), block_flow("l2", "l1", GREEN, "取块")],
         notes=[("l1", "不命中", RED)],
         caption="④ 之后读 x：L1 不命中，这时才从 L2 取块"),
]


if __name__ == "__main__":
    for name, frames in (("write-problem", PROBLEM), ("write-through", THROUGH),
                         ("write-back", BACK), ("write-allocate", ALLOCATE),
                         ("no-write-allocate", AROUND)):
        for n, f in enumerate(frames, 1):
            save(f"{name}-{n}", W, H, frame(f))
