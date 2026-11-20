#!/usr/bin/env python3
"""prefix-tree: the cached prompts merged by common prefix; LRU eviction.

The tree is drawn left to right, its nodes segments of prompt_bar.py: a
child starts GAP right of its parent, so every node stands at the offset
its text has in a prompt, and the new request, a prompt above the tree,
stands over the nodes it matches. A child on the parent's row continues
it; a child on a lower row hangs from a line down the gap after the
parent. Under each node the time t it was last used. On the left the
names: 「新请求」 with its time, 「前缀树」 with what t means.
Rows: user A's two turns (system prompt, question 1, answer 1, question 2,
answer 2), the node added by the new request, user B's turn (question 3,
answer 3) under the shared system prompt. Times: user A turn 1 at t = 1,
user B at t = 2, user A turn 2 at t = 3, the new request at t = 4.
1: the new request (system prompt, question 1, answer 1, question 4) at
   t = 4. Its first three segments match the path from the root: the
   band of reused K, V under them in the request and in the tree, their
   times set to 4. Question 4 is computed and added as a new node under
   answer 1 (「新建」); the request labels the run 「复用」 and question 4
   「计算」.
2: the cache is full (「缓存已满」 under the tree's name). The leaves'
   times are drawn dark; the oldest leaf, user B's answer 3 (t = 2), is
   evicted (dashed, 「淘汰」). Question 3 has the same time but is an inner
   node and stays.
"""
from prompt_bar import *

H = 370
BAR_Y = 16                         # top of the new request
ROW_Y = {"A": 126, "N": 220, "B": 296}     # top of each row of the tree
NOW = 4

# node: (parent, row, time last used before the new request)
NODES = {
    "系统提示词": (None, "A", 3),
    "问题 1": ("系统提示词", "A", 3),
    "回答 1": ("问题 1", "A", 3),
    "问题 2": ("回答 1", "A", 3),
    "回答 2": ("问题 2", "A", 3),
    "问题 3": ("系统提示词", "B", 2),
    "回答 3": ("问题 3", "B", 2),
    "问题 4": ("回答 1", "N", NOW),
}
REQUEST = ["系统提示词", "问题 1", "回答 1", "问题 4"]
MATCHED = REQUEST[:3]
EVICT = "回答 3"


def left(node):
    parent = NODES[node][0]
    return X0 if parent is None else left(parent) + seg_width(parent) + GAP


def tlabel(x, y, rest, size=17, fill=MUTED, weight="normal", anchor="middle"):
    """An italic serif t followed by plain text, as one line."""
    return (f'<text x="{x:.1f}" y="{y:.1f}" font-family="{FONT}" font-size="{size}" '
            f'font-weight="{weight}" fill="{fill}" text-anchor="{anchor}" '
            f'xml:space="preserve"><tspan font-family="{MATH}" font-style="italic" '
            f'font-size="{size + 2}">t</tspan>{esc(rest)}</text>')


def leaves():
    parents = {p for p, _, _ in NODES.values()}
    return [n for n in NODES if n not in parents]


def edge(parent, child, dash=None):
    """Parent to child: straight on one row, else down the gap after the parent."""
    px = left(parent) + seg_width(parent)
    py = ROW_Y[NODES[parent][1]] + SEG_H / 2
    cy = ROW_Y[NODES[child][1]] + SEG_H / 2
    if cy == py:
        return [line(px, py, left(child), cy, MUTED, 1.8, dash)]
    trunk = px + GAP / 2
    return [elbow([(trunk, py), (trunk, cy), (left(child), cy)], MUTED, 1.8,
                  dash=dash, head=False)]


def frame(n):
    b = []
    # the new request, its reused run and its computed segment
    b += row_name(BAR_Y + SEG_H / 2, "新请求")
    b.append(tlabel(NAME_X, BAR_Y + SEG_H / 2 + 30, f" = {NOW}", 16, anchor="start"))
    els, spans = bar(X0, BAR_Y, REQUEST, reused={0, 1, 2})
    b += els
    lab_y = BAR_Y + SEG_H + BAND_H + 22
    b += under(spans[0][0], spans[2][1], lab_y, "复用")
    b += under(*spans[3], lab_y, "计算")

    # the tree
    top = ROW_Y["A"]
    b += row_name(top + SEG_H / 2, "前缀树")
    b.append(tlabel(NAME_X, top + SEG_H / 2 + 30, "：上次使用的时间", 16, anchor="start"))
    if n == 2:
        b.append(label(NAME_X, top + SEG_H / 2 + 56, "缓存已满", 17, INK, "bold", "start"))
    gone = n == 2
    for node, (parent, row, t) in NODES.items():
        if parent:
            b += edge(parent, node, dash="5 4" if gone and node == EVICT else None)
    tips = leaves()
    for node, (parent, row, t) in NODES.items():
        x, y = left(node), ROW_Y[row]
        w = seg_width(node)
        b += segment(x, y, node, evicted=gone and node == EVICT)[0]
        reused = node in MATCHED
        if reused:
            b += band(x, y + SEG_H, w)
            t = NOW
        ty = y + SEG_H + (BAND_H + 20 if row == "A" else 22)
        dark = n == 2 and node in tips
        b.append(tlabel(x + w / 2, ty, f" = {t}", 17, INK if dark else MUTED,
                        "bold" if dark else "normal"))
    q4 = "问题 4"
    b.append(label(left(q4) + seg_width(q4) + 14, ROW_Y["N"] + SEG_H / 2 + 6, "新建",
                   17, INK, "bold", "start"))
    if gone:
        x = left(EVICT) + seg_width(EVICT) + 14
        b.append(label(x, ROW_Y["B"] + SEG_H / 2 + 6, "淘汰", 17, MUTED, "bold", "start"))
    save(f"prefix-tree-{n}", b, H)


for n in (1, 2):
    frame(n)
