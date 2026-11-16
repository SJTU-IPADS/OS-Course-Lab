#!/usr/bin/env python3
"""Four warps on one SM take turns: the scheduler issues from a ready warp.

Each warp issues two instructions, the second a load from device memory, then
waits eight cycles for the data and issues two more. Left: the SM with its
warp scheduler and four warps, and the device memory beside it; a load is an
arrow to the memory, the data coming back is an arrow from it. Right: the
same story on a time axis, one row per warp, aligned with the warps on the
left. With four warps two cycles stay empty: eight cycles of waiting need four
other warps issuing two instructions each, and only three are there.
Frame f covers two cycles: ../assets/warp-switch-1.svg .. warp-switch-9.svg.
Run it to refresh them.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from svgkit import (FILL_GREEN, FILL_GREY, FILL_ORANGE, GREEN, INK, LINE, MUTED, ORANGE,
                    WHITE, arrow, mono, rect, save, text)

W, H = 1120, 372
X0, U = 436, 36            # the left edge of cycle 0, the width of one cycle
SLOTS = 18
WAIT = 2 + 8               # a warp issues in 2 cycles, then waits 8
TOP, ROWH = 92, 40
SM_X, SM_W = 8, 244        # the SM box
DRAM_X, DRAM_W = 316, 78   # the device memory

# The cycles in which warp w issues: 2w, 2w + 1, then again after its load.
ISSUE = {w: [2 * w, 2 * w + 1, 2 * w + 10, 2 * w + 11] for w in range(4)}
LOAD = {w: 2 * w + 1 for w in range(4)}         # the cycle of its ld
BACK = {w: 2 * w + 10 for w in range(4)}        # the cycle its data is back
FRAMES = 9                                      # frame f covers cycles 2f - 2, 2f - 1

NOTES = [
    "周期 0–1：调度器选中 Warp 0，发射两条指令，第二条 ld 向显存发出读请求，Warp 0 开始等待",
    "周期 2–3：Warp 0 在等待，调度器选中 Warp 1，它也向显存发出读请求",
    "周期 4–5：调度器选中 Warp 2，Warp 0 与 Warp 1 仍在等待",
    "周期 6–7：调度器选中 Warp 3，4 个 Warp 都在等待显存的数据",
    "周期 8–9：没有就绪的 Warp，调度器没有指令可发",
    "周期 10–11：显存的数据返回 Warp 0，它重新就绪，调度器选中它继续发射",
    "周期 12–13：数据返回 Warp 1，调度器选中 Warp 1",
    "周期 14–15：数据返回 Warp 2，调度器选中 Warp 2",
    "周期 16–17：数据返回 Warp 3，调度器选中 Warp 3，4 个 Warp 都已完成这一轮",
]


def row_y(w):
    return TOP + w * ROWH


def state(w, f):
    """What warp w is doing in frame f: issue, load, wait, back, or ready."""
    a, b = 2 * f - 2, 2 * f - 1
    if a in ISSUE[w] or b in ISSUE[w]:
        return "issue"
    if LOAD[w] < a and b < BACK[w]:
        return "wait"
    return "ready"


def panel(f):
    """The SM with the scheduler and four warps, the memory, the arrows."""
    a = 2 * f - 2
    picked = next((w for w in range(4) if a in ISSUE[w]), None)
    out = [rect(SM_X, 4, SM_W, TOP + 4 * ROWH + 3, WHITE, INK, rx=6, width=1.6),
           text(SM_X + 12, 24, "SM", 15, INK, "bold", "start")]
    # the scheduler, level with the scheduler row of the time axis
    out.append(rect(SM_X + 12, 32, SM_W - 24, 30, FILL_ORANGE if picked is not None else WHITE,
                    ORANGE if picked is not None else LINE, rx=4, width=1.4))
    out.append(text(SM_X + SM_W / 2, 52, "Warp 调度器" + (f"：选中 W{picked}" if picked is not None
                                                      else "：无就绪 Warp"),
                    15, INK, "bold"))
    for w in range(4):
        y = row_y(w)
        s = state(w, f)
        fill, stroke, dash, label = {
            "issue": (FILL_ORANGE, ORANGE, None, "发射"),
            "wait": (FILL_GREY, MUTED, "4 3", "等待显存的数据"),
            "ready": (WHITE, LINE, None, "就绪"),
        }[s]
        out.append(rect(SM_X + 12, y + 3, SM_W - 24, ROWH - 8, fill, stroke, rx=4, width=1.4,
                        dash=dash))
        out.append(text(SM_X + 22, y + 26, f"Warp {w}", 15, INK, "bold", "start"))
        out.append(text(SM_X + SM_W - 20, y + 26, label, 14, INK if s == "issue" else MUTED,
                        anchor="end"))
    # the device memory
    mem_top, mem_bot = row_y(0) + 2, row_y(3) + ROWH - 6
    out.append(rect(DRAM_X, mem_top, DRAM_W, mem_bot - mem_top, FILL_GREY, MUTED, rx=6, width=1.6))
    out.append(text(DRAM_X + DRAM_W / 2, (mem_top + mem_bot) / 2 + 6, "显存", 18, INK, "bold"))
    # the loads and their data
    ax0, ax1 = SM_X + SM_W - 12, DRAM_X
    for w in range(4):
        yc = row_y(w) + ROWH / 2 - 2
        a, b = 2 * f - 2, 2 * f - 1
        if a <= LOAD[w] <= b:
            out.append(arrow(ax0 + 2, yc - 5, ax1 - 2, yc - 5, ORANGE, 2.4))
        elif LOAD[w] < a and b < BACK[w]:
            out.append(arrow(ax0 + 2, yc - 5, ax1 - 2, yc - 5, MUTED, 1.4, dash="4 3"))
        if a <= BACK[w] <= b:
            out.append(arrow(ax1 - 2, yc + 5, ax0 + 2, yc + 5, GREEN, 2.4))
    return out


def timeline(f):
    out = []
    a, b = 2 * f - 2, 2 * f - 1
    out.append(rect(X0 + a * U, 28, 2 * U, TOP + 4 * ROWH - 28 + 2, "#f4f8fa", "none", rx=3,
                    width=0))
    # the scheduler row
    for c in range(b + 1):
        who = [w for w in ISSUE if c in ISSUE[w]]
        x = X0 + c * U
        out.append(rect(x + 2, 32, U - 4, 30, FILL_ORANGE if who else WHITE,
                        ORANGE if who else LINE, rx=3, width=1.2))
        out.append(mono(x + U / 2, 52, f"W{who[0]}" if who else "空", 13,
                        INK if who else MUTED, "bold" if who else "normal", "middle"))
    for c in range(b + 1, SLOTS):
        out.append(rect(X0 + c * U + 2, 32, U - 4, 30, WHITE, LINE, rx=3, width=0.8))
    # one row per warp
    for w in range(4):
        y = row_y(w)
        out.append(rect(X0, y + 6, SLOTS * U, ROWH - 8, WHITE, LINE, rx=3, width=0.8))
        for c in ISSUE[w]:
            if c <= b:
                out.append(rect(X0 + c * U + 2, y + 8, U - 4, ROWH - 12, FILL_ORANGE, ORANGE,
                                rx=2, width=1.2))
        if LOAD[w] <= b:
            out.append(mono(X0 + LOAD[w] * U + U / 2, y + 27, "ld", 12, INK, "bold", "middle"))
            start = LOAD[w] + 1
            end = min(b + 1, BACK[w])
            if end > start:
                out.append(rect(X0 + start * U + 2, y + 8, (end - start) * U - 4, ROWH - 12,
                                FILL_GREY, MUTED, rx=2, width=1, dash="4 3"))
                if end - start >= 6:
                    out.append(text(X0 + (start + end) / 2 * U, y + 26, "等待显存的数据", 13,
                                    MUTED))
    # the time axis
    axis = TOP + 4 * ROWH + 18
    out.append(arrow(X0, axis, X0 + SLOTS * U + 20, axis, MUTED, 1.6))
    for c in range(0, SLOTS, 2):
        out.append(mono(X0 + c * U, axis + 20, str(c), 13, MUTED, anchor="middle"))
    out.append(text(X0 + SLOTS * U + 20, axis + 20, "时钟周期", 14, MUTED, anchor="end"))
    return out


def legend(y):
    out, x = [], X0 - 150
    items = ((FILL_ORANGE, ORANGE, None, "发射一条指令（ld：读显存）"),
             (FILL_GREY, MUTED, "4 3", "等待访存"),
             (WHITE, LINE, None, "就绪，未被选中"))
    for fill, stroke, dash, label in items:
        out.append(rect(x, y - 13, 26, 16, fill, stroke, rx=2, width=1.2, dash=dash))
        out.append(text(x + 34, y, label, 14, MUTED, anchor="start"))
        x += 34 + len(label) * 14 + 26
    out.append(arrow(x, y - 5, x + 30, y - 5, ORANGE, 2.2))
    out.append(text(x + 38, y, "读请求", 14, MUTED, anchor="start"))
    x += 38 + 3 * 14 + 22
    out.append(arrow(x, y - 5, x + 30, y - 5, GREEN, 2.2))
    out.append(text(x + 38, y, "数据返回", 14, MUTED, anchor="start"))
    return out


def frame(f):
    out = panel(f) + timeline(f)
    out.append(text(W / 2, 330, NOTES[f - 1], 17, INK, "bold"))
    out += legend(358)
    return out


if __name__ == "__main__":
    for f in range(1, FRAMES + 1):
        save(f"warp-switch-{f}", W, H, frame(f), left=-4)
