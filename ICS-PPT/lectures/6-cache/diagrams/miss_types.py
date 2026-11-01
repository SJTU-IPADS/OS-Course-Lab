#!/usr/bin/env python3
"""The three kinds of miss, one frame each, on a cache of four blocks (block b goes to
position b mod 4).

Each frame shows one reference sequence: a chip per reference (green = hit, red =
miss, with the kind of miss under it), the last reference in orange. Below it the
cache just before that last reference, with the block being requested pointing at
its position (framed red: a miss). The right panel gives the test that sorts the
miss: was the block referenced before, and how many other blocks have been
referenced since (fewer than 4: conflict; 4 or more: capacity). The classification
is computed here from the sequence, the same rule as simulate_misses in pages.py.
Run it to refresh ../assets/miss-types-1.svg ... miss-types-3.svg.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (BLUE, FILL_BLUE, FILL_GREEN, FILL_ORANGE, FILL_RED, GREEN, INK, MONO,
                    MUTED, ORANGE, RED, WHITE, arrow, brace, mono, rect, save, text)

W, H = 1000, 256
SLOTS = 4
SEQS = [("冷不命中", [0, 1, 2, 3]),
        ("冲突不命中", [0, 8, 0]),
        ("容量不命中", [0, 1, 2, 3, 4, 0])]
CHIP_X, CHIP_W, CHIP_STEP, CHIP_Y, CHIP_H = 130, 84, 96, 38, 56
SLOT_X, SLOT_W, SLOT_STEP, SLOT_Y, SLOT_H = 130, 110, 124, 168, 44


def classify(seq):
    """For each reference: ('命中' | '冷' | '冲突' | '容量', number of other blocks since)."""
    held = [None] * SLOTS
    last = {}
    out = []
    for idx, block in enumerate(seq):
        slot = block % SLOTS
        others = len(set(seq[last[block] + 1:idx]) - {block}) if block in last else None
        if held[slot] == block:
            kind = "命中"
        elif block not in last:
            kind = "冷"
        else:
            kind = "容量" if others >= SLOTS else "冲突"
        out.append((kind, others))
        held[slot] = block
        last[block] = idx
    return out


def held_before(seq, upto):
    held = [None] * SLOTS
    for block in seq[:upto]:
        held[block % SLOTS] = block
    return held


def frame(n):
    title, seq = SEQS[n - 1]
    res = classify(seq)
    last = len(seq) - 1
    block = seq[last]
    kind, others = res[last]
    out = [text(20, 28, title, 22, RED, "bold", "start")]
    # the reference sequence
    out.append(text(CHIP_X - 14, CHIP_Y + 32, "引用顺序", 16, MUTED, "bold", "end"))
    for idx, b in enumerate(seq):
        x = CHIP_X + idx * CHIP_STEP
        k = res[idx][0]
        now = idx == last
        fill, stroke = (FILL_GREEN, GREEN) if k == "命中" else (FILL_RED, RED)
        out.append(rect(x, CHIP_Y, CHIP_W, CHIP_H, FILL_ORANGE if now else fill,
                        ORANGE if now else stroke, rx=6, width=3 if now else 1.6))
        out.append(mono(x + CHIP_W / 2, CHIP_Y + 27, str(b), 21, INK, "bold", "middle"))
        label = "命中" if k == "命中" else f"{k}不命中"
        out.append(text(x + CHIP_W / 2, CHIP_Y + 47, label, 14, GREEN if k == "命中" else RED,
                        "bold"))
    # the other blocks referenced since the previous reference of the block
    if others is not None:
        prev = max(i for i in range(last) if seq[i] == block)
        x0 = CHIP_X + (prev + 1) * CHIP_STEP
        x1 = CHIP_X + (last - 1) * CHIP_STEP + CHIP_W
        out += brace(x0, x1, CHIP_Y + CHIP_H + 8, BLUE, f"其他块 {others} 个", 16)
    # the cache before the last reference
    held = held_before(seq, last)
    out.append(text(SLOT_X - 14, SLOT_Y + 30, "缓存", 16, MUTED, "bold", "end"))
    for k in range(SLOTS):
        x = SLOT_X + k * SLOT_STEP
        mine = k == block % SLOTS
        out.append(text(x + SLOT_W / 2, SLOT_Y - 10, f"位置 {k}", 15, MUTED, "bold"))
        out.append(rect(x, SLOT_Y, SLOT_W, SLOT_H, FILL_BLUE if held[k] is not None else WHITE,
                        INK, rx=3, width=1.6))
        out.append(mono(x + SLOT_W / 2, SLOT_Y + 30, "空" if held[k] is None else str(held[k]),
                        21 if held[k] is not None else 17, INK if held[k] is not None else MUTED,
                        "bold", "middle"))
        if mine:
            out.append(rect(x - 5, SLOT_Y - 5, SLOT_W + 10, SLOT_H + 10, "none", RED, rx=5,
                            width=3))
            cx = x + SLOT_W / 2
            out.append(arrow(cx, SLOT_Y + SLOT_H + 46, cx, SLOT_Y + SLOT_H + 10, ORANGE, 3))
            out.append(text(cx - 14 if k >= 2 else cx + 14, SLOT_Y + SLOT_H + 34,
                            f"请求块 {block}：{block} mod 4 = {k}", 17, ORANGE, "bold",
                            "end" if k >= 2 else "start"))
    # the test
    px = 730
    out.append(text(px, 158, "判定", 17, MUTED, "bold", "start"))
    lines = []
    if others is None:
        lines = [(f"块 {block} 之前没有被引用过", INK), ("冷不命中", RED)]
    else:
        lines = [(f"块 {block} 之前被引用过", INK),
                 (f"其他块 {others} 个，{'少于' if others < SLOTS else '不少于'} {SLOTS} 个", INK),
                 (kind + "不命中", RED)]
    for i, (s, c) in enumerate(lines):
        out.append(text(px, 158 + 28 * (i + 1), s, 18, c, "bold", "start"))
    return out


if __name__ == "__main__":
    for n in (1, 2, 3):
        save(f"miss-types-{n}", W, H, frame(n), left=-34)
